// Game.h
#ifndef GAME_H
#define GAME_H

#include <nlohmann/json.hpp>
using json = nlohmann::json;  // 添加这一行
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
#include "BatchRenderer.h" 
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
struct LoadResult {
    int levelNumber;
    bool success;
    std::string message;
};
enum class LoadState { 
    IDLE,    // 空闲状态
    LOADING, // 正在加载
    DONE     // 加载完成
};

class ParticlePool {
public:
    static constexpr int MAX_PARTICLES = 500;
    
    ParticlePool();
    
    void Spawn(Vector2 pos, Vector2 vel, Color color);
    void Update(float dt);
    void Draw() const;
    void Clear();
    
    // 获取粒子数量（用于调试）
    int GetActiveCount() const;
    
private:
    Particle particles[MAX_PARTICLES];
    int freeList[MAX_PARTICLES];  // 空闲索引栈
    int freeCount;
};

class Game {
public:
    Game();
    ~Game();
    
    void Init();
    void Update();
    void Draw();
    void Shutdown();
    void RunBenchmark(int frames = 600);
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
    
    // 网络相关方法
    void StartAsHost();
    void StartAsClient(const std::string& ip);
    Paddle& GetPaddle2() { return paddle2; }

        // ========== 数据持久化（存档/读档） ==========
    void SaveGame();                // 保存当前游戏状态
    bool LoadGame();                // 加载存档，返回是否成功
    bool SaveExists() const;        // 检查存档文件是否存在
    void ContinueFromSave();        // 从存档继续游戏（由菜单调用）
    void ResetToNewGame();          // 重置为新游戏（清除存档）
    
private:

    static constexpr float NETWORK_UPDATE_RATE = 1.0f / 30.0f;
    
    // ========== 空间划分相关常量 ==========
    static constexpr int GRID_COLS = 8;   // 网格列数
    static constexpr int GRID_ROWS = 6;   // 网格行数

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
    
    // ========== 空间划分相关成员 ==========
    std::vector<Brick*> grid[GRID_COLS][GRID_ROWS];  // 网格指针数组
    float cellWidth;   // 网格宽度
    float cellHeight;  // 网格高度
    
    // 客户端预测用的砖块副本（仅用于视觉）
    std::vector<Brick> predictedBricks;
    bool usePredictedBricks = false;
    
    // 道具与粒子
    std::vector<PowerUp> powerUps;
    ParticlePool particlePool;
    float powerUpDropRate;
    float slowEffectRemainingTime;
    
    BatchRenderer batchRenderer;  // 批绘制器

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
    bool isNetworkGame;
    NetworkRole localRole;
    float clientPredictionTimer;
    int clientFrameNumber;           // 客户端当前帧号
    int lastConfirmedFrame;          // 主机最后确认的帧号
    
    // 异步加载相关
    LoadState loadState = LoadState::IDLE;
    std::future<bool> loadFuture;
    std::string loadingLevelPath;
    int pendingLevelNumber;
    ThreadSafeQueue<LoadResult> loadResultQueue;
    
    bool benchmarkMode = false;  // 基准测试模式标志

    // 异步加载控制变量
    bool isLoadingExclusive = false;  // 独占加载标志，禁止输入
    float loadStartTime = 0.0f;       // 加载开始时间
    int frameCounter = 0;              // 帧计数器（用于性能调试）
    // 空间划分相关
    int spatialGridUpdateCounter = 0;   // ✅ 新增：专门用于空间网格更新
    // 纹理相关
    Texture2D currentBackground = { 0, 0, 0, 0, 0 };
    std::string backgroundPath;
    bool backgroundLoading = false;
    
    // 配置参数结构体
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
    
    // ========== 私有方法声明 ==========
    
    // 基础方法
    void LoadConfig(const std::string& path);
    void LoadLeaderboard();
    void SaveLeaderboard();
    void AddScoreToLeaderboard();
    void BuildBricks();
    void BuildBricksDefault(); 
    void BuildBricksFromLayout(const json& levelJson);
    void ResetGame();
    void ChangeState(GameState newState);
    
    // ========== 空间划分方法 ==========
    void BuildSpatialGrid();                       // 构建空间网格
    std::vector<Brick*> GetNearbyBricks(const Ball& ball);  // 获取球附近的砖块
    
    // 状态处理函数
    void HandleMenuState();
    void HandleLevelSelectState();
    void HandlePlayingState(float dt);  
    void HandlePausedState();
    void HandleGameOverState();
    void HandleWinState();
    void HandleLeaderboardState();
    
    // 绘制函数
    void DrawMenu();
    void DrawLevelSelect();
    void DrawPlaying();
    void DrawPaused();
    void DrawGameOver();
    void DrawWin();
    void DrawLeaderboard();
    void DrawNameInput();
    void DrawDebugInfo();
    
    // 关卡逻辑
    void StartLevel(int level);
    void LoadBaseConfig(const std::string& path);
    void LoadLevelConfig(const std::string& path);
    
    // 道具与粒子
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
    void UpdateClientCollisionPrediction(float dt);
    void SendClientInputToHost();
    void ReceiveAndSmoothHostState();
    
    // 网络方法
    void SetupNetworkMode();
    void UpdateNetwork();
    void SendGameStateToPeer();
    void ReceiveAndApplyPeerState();
    
    // 异步加载相关
    void CheckAsyncLoading();
    void ApplyLoadedLevel();
    bool LoadLevelAssetsAsync(const std::string& levelPath);

    void ApplySaveData(const json& save);   // 将json数据应用到游戏（内部使用）
    // 初始化
    void InitBalls();
    void InitPaddle();
    void InitParticles();
    
    void UpdateBenchmark(float fixedDt);
    
    // 异步纹理加载
    std::vector<std::future<Texture2D>> asyncTextures;

    void ResyncBalls(const GameStateSnapshot& state);  // 重新同步球
    float CalculatePredictionError(const GameStateSnapshot& state);  // 计算预测误差

    // ========== 关卡编辑器相关 ==========
    bool editorMode = false;           // 是否处于编辑模式
    int editorGridCols = 8;            // 编辑网格列数
    int editorGridRows = 5;            // 编辑网格行数
    float editorCellWidth;             // 网格单元宽度
    float editorCellHeight;            // 网格单元高度
    int selectedBrickType = 1;         // 当前选择的砖块类型（血量）
     // 错误处理相关
    bool showConfigError = false;
    float configErrorTimer = 0.0f;
    std::string configErrorMessage;
    
    // 编辑模式辅助方法
    void HandleEditorMode();
    void DrawEditorUI();
    void AddBrickAtMouse();
    void RemoveBrickAtMouse();
    void SaveCurrentLayout();
    void LoadCustomLevel(const std::string& path);
     // ========== 错误处理方法声明 ==========
    void ShowConfigError(const std::string& message);
    bool ValidateConfigFile(const std::string& path);  // 可选
};

#endif