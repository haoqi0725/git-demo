#pragma once

#include "raylib.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include "GameState.h"
#include <vector>
#include <string>
#include <algorithm>

/**
 * @brief 打砖块游戏主类
 * 
 * 管理游戏的核心逻辑，包括状态机、碰撞检测、配置加载和排行榜功能。
 * 支持多关卡切换和调试模式。
 */
class Game {
public:
    /**
     * @brief 构造函数
     * 
     * 初始化游戏对象的默认值
     */
    Game();
    
    /**
     * @brief 析构函数
     * 
     * 释放游戏资源
     */
    ~Game();
    
    /**
     * @brief 初始化游戏
     * 
     * 加载配置文件、排行榜，创建游戏对象
     */
    void Init();
    
    /**
     * @brief 更新游戏逻辑
     * 
     * 每帧调用，处理输入、碰撞检测、状态转换
     */
    void Update();
    
    /**
     * @brief 绘制游戏画面
     * 
     * 根据当前状态绘制相应的界面
     */
    void Draw();
    
    /**
     * @brief 关闭游戏
     * 
     * 保存排行榜，释放资源
     */
    void Shutdown();
    
private:
    // 窗口尺寸
    int SW;                     ///< 屏幕宽度
    int SH;                     ///< 屏幕高度
    std::string windowTitle;    ///< 窗口标题
    
    // 游戏状态
    GameState state;            ///< 当前游戏状态
    int score;                  ///< 当前分数
    int lives;                  ///< 剩余生命
    
    // 游戏对象
    Ball ball;                  ///< 球对象
    Paddle paddle;              ///< 挡板对象
    std::vector<Brick> bricks;  ///< 砖块数组
    
    // 排行榜
    struct ScoreEntry {
        std::string name;       ///< 玩家名称
        int score;              ///< 分数
        std::string date;       ///< 日期
    };
    std::vector<ScoreEntry> leaderboard;  ///< 排行榜列表
    std::string playerName;     ///< 当前玩家名称
    int nameInputCursor;        ///< 输入光标位置
    bool isEnteringName;        ///< 是否正在输入名字
    
    // 调试模式
    bool debugMode;             ///< 调试模式开关
    bool godMode;               ///< 无敌模式（无限生命）
    
    // 关卡系统
    int currentLevel;           ///< 当前关卡
    std::vector<std::string> levels;  ///< 关卡配置文件列表
    
    // 配置参数
    struct {
        // Ball 配置
        float ballRadius;       ///< 球半径
        float ballSpeedX;       ///< 球水平速度
        float ballSpeedY;       ///< 球垂直速度
        float gravity;          ///< 重力
        float maxSpeed;         ///< 最大速度
        float bounceForce;      ///< 反弹力度
        
        // Paddle 配置
        int paddleWidth;        ///< 挡板宽度
        int paddleHeight;       ///< 挡板高度
        int paddleSpeed;        ///< 挡板移动速度
        int paddleYOffset;      ///< 挡板距离底部距离
        int paddleBoostSpeed;   ///< 加速速度
        
        // Bricks 配置
        int brickRows;          ///< 砖块行数
        int brickCols;          ///< 砖块列数
        float brickWidth;       ///< 砖块宽度
        float brickHeight;      ///< 砖块高度
        float brickStartX;      ///< 砖块起始X坐标
        float brickStartY;      ///< 砖块起始Y坐标
        float brickPaddingX;    ///< 砖块水平间距
        float brickPaddingY;    ///< 砖块垂直间距
        
        // Game 配置
        int initialLives;       ///< 初始生命值
        int scorePerBrick;      ///< 每个砖块分数
        float timeMultiplierDecay;  ///< 时间衰减系数
        
    } config;
    
    /**
     * @brief 加载配置文件
     * @param path 配置文件路径
     */
    void LoadConfig(const std::string& path);
    
    /**
     * @brief 加载排行榜
     */
    void LoadLeaderboard();
    
    /**
     * @brief 保存排行榜
     */
    void SaveLeaderboard();
    
    /**
     * @brief 添加分数到排行榜
     */
    void AddScoreToLeaderboard();
    
    /**
     * @brief 构建砖块
     */
    void BuildBricks();
    
    /**
     * @brief 重置游戏
     */
    void ResetGame();
    
    /**
     * @brief 切换关卡
     * @param level 关卡编号（从1开始）
     */
    void SwitchLevel(int level);
    
    /**
     * @brief 状态转换
     * @param newState 新状态
     */
    void ChangeState(GameState newState);
    
    // 状态处理
    void HandleMenuState();     ///< 菜单状态处理
    void HandlePlayingState();  ///< 游戏状态处理
    void HandlePausedState();   ///< 暂停状态处理
    void HandleGameOverState(); ///< 游戏结束状态处理
    void HandleWinState();      ///< 胜利状态处理
    void HandleLeaderboardState(); ///< 排行榜状态处理
    
    // 绘制方法
    void DrawMenu();            ///< 绘制菜单
    void DrawPlaying();         ///< 绘制游戏画面
    void DrawPaused();          ///< 绘制暂停画面
    void DrawGameOver();        ///< 绘制游戏结束
    void DrawWin();             ///< 绘制胜利画面
    void DrawLeaderboard();     ///< 绘制排行榜
    void DrawNameInput();       ///< 绘制姓名输入框
    void DrawDebugInfo();       ///< 绘制调试信息
};
