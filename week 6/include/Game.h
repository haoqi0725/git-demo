#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include "GameState.h"
#include "PowerUp.h"
#include <vector>
#include <string>
#include <algorithm>
#include <memory>

// 粒子结构体
struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float life;
    float maxLife;
    bool active;
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
    void SpawnPowerUp(float x, float y, PowerUpType type);
    
    // 粒子系统接口
    void SpawnParticles(Rectangle rect, Color color);
    void SpawnPowerUpGlow(Vector2 pos, Color color);
    
private:
    static const int MAX_PARTICLES = 500;
    
    // 窗口
    int SW, SH;
    std::string windowTitle;
    
    // 游戏状态
    GameState state;
    int score, lives;
    
    // 游戏对象
    std::vector<Ball> balls;
    Paddle paddle;
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
    
    // 状态处理
    void HandleMenuState();
    void HandlePlayingState();
    void HandlePausedState();
    void HandleGameOverState();
    void HandleWinState();
    void HandleLeaderboardState();
    
    // 绘制
    void DrawMenu();
    void DrawPlaying();
    void DrawPaused();
    void DrawGameOver();
    void DrawWin();
    void DrawLeaderboard();
    void DrawNameInput();
    void DrawDebugInfo();
    
    // 道具与粒子更新
    void UpdatePowerUps(float dt);
    void HandlePowerUpCollisions();
    void UpdateParticles(float dt);
    void DrawParticles();
    void UpdateBalls(float dt);
};

#endif
