#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include "GameState.h"
#include "PowerUp.h"
#include "NetworkManager.h"
#include "InterpolationManager.h"
#include "TextureCache.h"
#include "ThreadSafeQueue.h"
#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <future>
#include <thread>
#include <chrono>


// 粒子结构体
struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float life;
    float maxLife;
    bool active;
};
enum class LoadState { 
    IDLE,    // 空闲状态
    LOADING, // 正在加载
    DONE     // 加载完成
};
class Game {
public:
    Game();
    ~Game();
    
    void Init();
    void Update();
    void Draw();
    void Shutdown();
    
    // 道具系统接口
    Paddle& GetPaddle() { return paddle; }
    std::vector<Ball>& GetBalls() { return balls; }
    void AddMultiBall(int count);
    void SlowDownBalls(float factor, float duration);
    void EnableFireBall(float duration);
    void SpawnPowerUp(float x, float y, PowerUpType type);
    // 启动异步关卡加载
    void StartAsyncLevelLoad(int levelNumber);
    
    // 获取当前加载状态（用于UI显示）
    LoadState GetLoadState() const { return loadState; }

    auto& GetConfig() { return config; }
    const auto& GetConfig() const { return config; }
    
    // 粒子系统接口
    void SpawnParticles(Rectangle rect, Color color);
    void SpawnPowerUpGlow(Vector2 pos, Color color);
    

     // 网络相关方法（public，让main或其他地方调用）
    void StartAsHost();
    void StartAsClient(const std::string& ip);
     Paddle& GetPaddle2() { return paddle2; }
private:

    static const int MAX_PARTICLES = 500;
     // 网络常量
    static constexpr float NETWORK_UPDATE_RATE = 1.0f / 30.0f;  // 30Hz

    // 窗口
    int SW, SH;
    std::string windowTitle;
    
    // 游戏状态
    GameState state;
    int score, lives;
    
    // 游戏对象
    std::vector<Ball> balls;
    Paddle paddle;
    Paddle paddle2;  
    std::vector<Brick> bricks;
    
    // 道具与粒子
    std::vector<PowerUp> powerUps;
    std::vector<Particle> particles;
    float powerUpDropRate;
    float slowEffectRemainingTime;
    
    // 排行榜
    struct ScoreEntry {
        std::string name;
        int score;
        std::string date;
    };
    std::vector<ScoreEntry> leaderboard;
    std::string playerName;
    int nameInputCursor;
    bool isEnteringName;
    
    // 调试模式
    bool debugMode;
    bool godMode;
    int currentLevel;
    bool gameStarted; 
    
     // 网络相关成员
    NetworkManager networkManager;
    InterpolationManager interpolationManager;
    float networkUpdateTimer;

    // 网络模式标志
    bool isNetworkGame;
    NetworkRole localRole;
     // 客户端预测相关
    float clientPredictionTimer;

    LoadState loadState = LoadState::IDLE;  // 加载状态
    std::future<bool> loadFuture;           // 异步任务句柄
    std::string loadingLevelPath;             // 当前正在加载的关卡路径
    int pendingLevelNumber;                   // 待切换的关卡编号
    // 配置参数
    struct {
        float ballRadius, ballSpeedX, ballSpeedY;
        float gravity, maxSpeed, bounceForce;
        int paddleWidth, paddleHeight, paddleSpeed, paddleYOffset, paddleBoostSpeed;
        int brickRows, brickCols;
        float brickWidth, brickHeight, brickStartX, brickStartY;
        float brickPaddingX, brickPaddingY;
        int initialLives, scorePerBrick;
        float timeMultiplierDecay;
        struct {
            struct { float extra_width, duration, drop_rate; } paddle_extend;
            struct { int extra_balls; float duration, drop_rate; } multi_ball;
            struct { float speed_factor, duration, drop_rate; } slow_ball;
            struct { float duration, drop_rate; } fire_ball;
        } powerups;
    } config;
    
   // 内部方法
    void LoadConfig(const std::string& path);
    void LoadLeaderboard();
    void SaveLeaderboard();
    void AddScoreToLeaderboard();
    void BuildBricks();
    void ResetGame();
    void ChangeState(GameState newState);
    
    // 异步加载关卡资源（在后台线程执行）
    bool LoadLevelAssetsAsync(const std::string& levelPath);
    
    // 检查异步加载状态（每帧调用）
    void CheckAsyncLoading();
    // 应用加载完成的关卡数据
    void ApplyLoadedLevel();

    // 状态处理函数（注意：HandlePlayingState 需要 float dt 参数）
    void HandleMenuState();
    void HandleLevelSelectState();
    void HandlePlayingState(float dt);  
    void HandlePausedState();
    void HandleGameOverState();
    void HandleWinState();
    void HandleLeaderboardState();
    // 绘制
    void DrawMenu();
    void DrawLevelSelect();
    void DrawPlaying();
    void DrawPaused();
    void DrawGameOver();
    void DrawWin();
    void DrawLeaderboard();
    void DrawNameInput();
    void DrawDebugInfo();

    // ========== 关卡逻辑 ==========
    void StartLevel(int level);
    void LoadBaseConfig(const std::string& path);   // 加载基础配置
    void LoadLevelConfig(const std::string& path);  // 加载关卡特定配置
    
    // 道具与粒子更新
    void UpdatePowerUps(float dt);
    void HandlePowerUpCollisions();
    void UpdateParticles(float dt);
    void DrawParticles();
    
      // 游戏逻辑辅助函数
    void UpdateBallsPhysics(float dt);
    void HandleBallBrickCollisions();
    void TrySpawnPowerUp(Rectangle brickRect);
    void LaunchBall();
    void CheckBallLoss();
    void CheckLevelCompletion();
    
     // 输入处理辅助函数                         
    void HandleLocalInput(float dt);
    void HandleHostInput(float dt);
    void HandleClientInput(float dt);
    
    // 客户端预测相关
    void UpdateClientPrediction(float dt);
    void SendClientInputToHost();
    void ReceiveAndSmoothHostState();  

    // 网络方法（private）
    void SetupNetworkMode();
    void UpdateNetwork();
    void SendGameStateToPeer();
    void ReceiveAndApplyPeerState();

    // 初始化相关
    void InitBalls();
    void InitPaddle();
    void InitParticles();
     // 存储异步加载完成的消息
    struct LoadResult {
        int levelNumber;
        bool success;
        std::string message;
    };
    ThreadSafeQueue<LoadResult> loadResultQueue;

    // 存储异步纹理加载的 future
    std::vector<std::future<Texture2D>> asyncTextures;
    
    // 背景纹理
    Texture2D currentBackground = { 0, 0, 0, 0, 0 };  
    std::string backgroundPath;
    
    // 标记背景是否正在加载
    bool backgroundLoading = false;
};

#endif
