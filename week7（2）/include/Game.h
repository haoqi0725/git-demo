#ifndef GAME_H
#define GAME_H

#include <enet/enet.h> // 确保头文件包含


#include "raylib.h"
#include <vector>
#include <string>

#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include "PowerUp.h"
#include "GameState.h" // 确保你有这个枚举定义文件
#include "NetworkProtocol.h"

#define MAX_PARTICLES 200

// 粒子结构体
struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float life;
    float maxLife;
    bool active;
};

// 排行榜条目结构体
struct ScoreEntry {
    std::string name;
    int score;
    std::string date;
};

// 道具配置
struct PaddleExtendConfig {
    float extra_width;
    float duration;
    float drop_rate;
};

struct MultiBallConfig {
    int extra_balls;
    float duration;
    float drop_rate;
};

struct SlowBallConfig {
    float speed_factor;
    float duration;
    float drop_rate;
};

struct PowerUpsConfigSet {
    PaddleExtendConfig paddle_extend;
    MultiBallConfig multi_ball;
    SlowBallConfig slow_ball;
};

// 游戏全局配置结构体
struct GameConfig {
    float ballRadius;
    float ballSpeedX;
    float ballSpeedY;
    float gravity;
    float maxSpeed;
    float bounceForce;
    
    float paddleWidth;
    float paddleHeight;
    float paddleSpeed;
    float paddleYOffset;
    float paddleBoostSpeed;
    
    int brickRows;
    int brickCols;
    float brickWidth;
    float brickHeight;
    float brickStartX;
    float brickStartY;
    float brickPaddingX;
    float brickPaddingY;
    
    int initialLives;
    int scorePerBrick;
    float timeMultiplierDecay;

    float paddleExtraWidthBuff = 0.0f;
    float globalEffectTimer = 0.0f;
    bool fireBallActive = false;
    
    PowerUpsConfigSet powerups;
};

class Game {
public:
    Game();
    ~Game();

    void Init(bool isServer);
    void Update();
    void Draw();
    void Shutdown();

    // 状态转换与重置
    void ChangeState(GameState newState);
    void ResetGame(bool fullReset = true);
    
    // 供 Effect 类获取实例的接口
    Paddle& GetPaddle() { return paddle; }
    std::vector<Ball>& GetBalls() { return balls; }

    // ================= 道具效果 API =================
    // 只有这五个，不要重复声明！
    void AddMultiBall(int count);
    void SlowDownBalls(float factor, float duration);
    void ActivateFireball(float duration);     // 新增：激活穿透火球
    void ActivateGiantBall(float duration, float scaleMultiplier); // 新增：激活巨大化球
    void SpawnTrailParticle(Vector2 pos);      // 新增：生成火球拖尾粒子
    // ================================================
   bool StartNetwork(bool hostMode); // true=启动服务器, false=作为客户端连接
    void HandleNetworkEvents();       // 用于在 Update() 中处理网络包
private:
    // 窗口与游戏状态
    int SW;
    int SH;
    std::string windowTitle;
    GameState state;
    
    // 玩家数据
    int score;
    int lives;
    int currentLevel;
    
    // 实体容器
    GameConfig config;
    std::vector<Ball> balls;
    Paddle paddle;
    Paddle paddle2;
    std::vector<Brick> bricks;
    std::vector<PowerUp> powerUps;
    std::vector<Particle> particles;
    
    // 全局控制计时器
    float powerUpDropRate;
    float slowEffectRemainingTime;
    
    // 调试与作弊模式
    bool debugMode;
    bool godMode;
    
    // 排行榜相关
    std::string playerName;
    int nameInputCursor;
    bool isEnteringName;
    std::vector<ScoreEntry> leaderboard;

    // === 新增：持久化道具状态 ===
   float savedExtraWidth = 0.0f;
    bool savedFireball = false;
    bool savedGiantBall = false; 
    float savedGiantMultiplier = 1.0f;
    bool savedSlowBall = false; // 新增：保存减速状态
    int savedMultiBallCount = 0; // 新增：保存多球状态

    bool ballLaunched = false; // 新增：是否已经发射

    // 网络成员
    ENetHost* netHost;
    ENetPeer* netPeer;
    bool isHost;
    bool simulatePacketLoss = false;
    int clientScore = 0;      // ← 加这行
    int clientLives = 3;
    // ===== 插值用 =====
       Vector2 renderBallPos;
    Vector2 targetBallPos;
    
    // PPT标准线性插值
    GameStateUpdate lastSnapshot;
    GameStateUpdate currentSnapshot;
    bool hasSnapshot = false;
    double lastSnapshotTime = 0.0;
    double currentSnapshotTime = 0.0;

    // === 内部私有方法 ===
    void LoadConfig(const std::string& path);
    void LoadLeaderboard();
    void SaveLeaderboard();
    void AddScoreToLeaderboard();
    
    void BuildBricks();
    void UpdateBalls(float dt);
    
    void SpawnPowerUp(float x, float y, PowerUpType type);
    void UpdatePowerUps(float dt);
    void HandlePowerUpCollisions();
    
    void SpawnParticles(Rectangle rect, Color color);
    void SpawnPowerUpGlow(Vector2 pos, Color color);
    void UpdateParticles(float dt);
    void DrawParticles();
    
    void DrawMenu();
    void DrawPlaying();
    void DrawPaused();
    void DrawGameOver();
    void DrawWin();
    void DrawLeaderboard();
    void DrawNameInput();
    void DrawDebugInfo();
    void DrawLevelClear();
};

#endif