#include "../include/Game.h"
#include "../include/nlohmann/json.hpp"
#include <cstdlib>  // for std::abs, std::rand
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <ctime>
#include <algorithm>
#include <cmath>

using json = nlohmann::json;

// ========== 构造函数 ==========
Game::Game() 
    : SW(800), SH(600), windowTitle("ARKANOID 2D")
    , state(GameState::MENU), score(0), lives(5)
    , paddle(350, 550, 100, 20, 300.0f)
    , paddle2(350, 550, 100, 20, 300.0f)
    , powerUpDropRate(0.3f), slowEffectRemainingTime(0.0f)
    // 排行榜（在 clientPredictionTimer 之前声明）
    , playerName(""), nameInputCursor(0), isEnteringName(false)
    // 调试模式
    , debugMode(true), godMode(false), currentLevel(1)
    , gameStarted(false)
    // 网络相关成员
    , networkUpdateTimer(0.0f)
    , isNetworkGame(false)
    , localRole(NetworkRole::NONE)
    , clientPredictionTimer(0.0f)        // 移到后面
{
}

Game::~Game() {
}

// ========== 加载配置文件 ==========
void Game::LoadConfig(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Warning: Cannot open config file: " << path << std::endl;
            SW = 800; SH = 600; windowTitle = "ARKANOID 2D";
            config.ballRadius = 10; config.ballSpeedX = 3; config.ballSpeedY = -4;
            config.gravity = 0.08; config.maxSpeed = 15; config.bounceForce = 0.5;
            config.paddleWidth = 100; config.paddleHeight = 20; config.paddleSpeed = 8;
            config.paddleYOffset = 50; config.paddleBoostSpeed = 28;
            config.brickRows = 4; config.brickCols = 9;
            config.brickWidth = 75; config.brickHeight = 25;
            config.brickStartX = 25; config.brickStartY = 80;
            config.brickPaddingX = 10; config.brickPaddingY = 8;
            config.initialLives = 5; config.scorePerBrick = 10;
            config.timeMultiplierDecay = 0.05;
            config.powerups.paddle_extend = {40.0f, 5.0f, 0.3f};
            config.powerups.multi_ball = {2, 0.0f, 0.2f};
            config.powerups.slow_ball = {0.7f, 5.0f, 0.25f};
            config.powerups.fire_ball = {5.0f, 0.2f};
            return;
        }

        json configJson = json::parse(file);
        
        SW = configJson.value("window", json::object()).value("width", 800);
        SH = configJson.value("window", json::object()).value("height", 600);
        windowTitle = configJson.value("window", json::object()).value("title", "ARKANOID 2D");

        auto ballCfg = configJson.value("ball", json::object());
        config.ballRadius = ballCfg.value("radius", 10.0f);
        config.ballSpeedX = ballCfg.value("speedX", 3.0f);
        config.ballSpeedY = ballCfg.value("speedY", -4.0f);
        config.gravity = ballCfg.value("gravity", 0.08f);
        config.maxSpeed = ballCfg.value("maxSpeed", 15.0f);
        config.bounceForce = ballCfg.value("bounceForce", 0.5f);

        auto paddleCfg = configJson.value("paddle", json::object());
        config.paddleWidth = paddleCfg.value("width", 100);
        config.paddleHeight = paddleCfg.value("height", 20);
        config.paddleSpeed = paddleCfg.value("speed", 8);
        config.paddleYOffset = paddleCfg.value("yOffset", 50);
        config.paddleBoostSpeed = paddleCfg.value("boostSpeed", 28);

        auto bricksCfg = configJson.value("bricks", json::object());
        config.brickRows = bricksCfg.value("rows", 4);
        config.brickCols = bricksCfg.value("cols", 9);
        config.brickWidth = bricksCfg.value("width", 75.0f);
        config.brickHeight = bricksCfg.value("height", 25.0f);
        config.brickStartX = bricksCfg.value("startX", 25.0f);
        config.brickStartY = bricksCfg.value("startY", 80.0f);
        config.brickPaddingX = bricksCfg.value("paddingX", 10.0f);
        config.brickPaddingY = bricksCfg.value("paddingY", 8.0f);

        auto gameCfg = configJson.value("game", json::object());
        config.initialLives = gameCfg.value("initialLives", 3);
        config.scorePerBrick = gameCfg.value("scorePerBrick", 10);
        config.timeMultiplierDecay = gameCfg.value("timeMultiplierDecay", 0.05f);

        auto powerCfg = configJson.value("powerups", json::object());
        auto pe = powerCfg.value("paddle_extend", json::object());
        config.powerups.paddle_extend.extra_width = pe.value("extra_width", 40.0f);
        config.powerups.paddle_extend.duration = pe.value("duration", 5.0f);
        config.powerups.paddle_extend.drop_rate = pe.value("drop_rate", 0.3f);
        auto mb = powerCfg.value("multi_ball", json::object());
        config.powerups.multi_ball.extra_balls = mb.value("extra_balls", 2);
        config.powerups.multi_ball.duration = mb.value("duration", 0.0f);
        config.powerups.multi_ball.drop_rate = mb.value("drop_rate", 0.2f);
        auto sb = powerCfg.value("slow_ball", json::object());
        config.powerups.slow_ball.speed_factor = sb.value("speed_factor", 0.7f);
        config.powerups.slow_ball.duration = sb.value("duration", 5.0f);
        config.powerups.slow_ball.drop_rate = sb.value("drop_rate", 0.25f);
        auto fb = powerCfg.value("fire_ball", json::object());
        config.powerups.fire_ball.duration = fb.value("duration", 5.0f);
        config.powerups.fire_ball.drop_rate = fb.value("drop_rate", 0.2f);
        
        paddle.SetSpeed(config.paddleSpeed);   
        paddle.SetWidth(config.paddleWidth);   
        // 重新计算位置，确保挡板贴近屏幕底部
        paddle.SetPosition((SW - config.paddleWidth) / 2.0f, SH - config.paddleYOffset);

        powerUpDropRate = config.powerups.paddle_extend.drop_rate;

        std::cout << "Config loaded: " << SW << "x" << SH << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
    }
}

// ========== 排行榜 ==========
void Game::LoadLeaderboard() {
    leaderboard.clear();
    std::ifstream file("leaderboard.txt");
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            size_t pos1 = line.find('|'), pos2 = line.find('|', pos1 + 1);
            if (pos1 != std::string::npos && pos2 != std::string::npos) {
                ScoreEntry se;
                se.name = line.substr(0, pos1);
                se.score = std::stoi(line.substr(pos1 + 1, pos2 - pos1 - 1));
                se.date = line.substr(pos2 + 1);
                leaderboard.push_back(se);
            }
        }
    }
    std::sort(leaderboard.begin(), leaderboard.end(),
              [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });
}

void Game::SaveLeaderboard() {
    std::ofstream file("leaderboard.txt");
    for (const auto& entry : leaderboard) {
        file << entry.name << "|" << entry.score << "|" << entry.date << "\n";
    }
}

void Game::AddScoreToLeaderboard() {
    time_t now = time(nullptr);
    std::string date = ctime(&now);
    date.pop_back();
    ScoreEntry newEntry;
    newEntry.name = playerName.empty() ? "Anonymous" : playerName;
    newEntry.score = score;
    newEntry.date = date;
    leaderboard.push_back(newEntry);
    std::sort(leaderboard.begin(), leaderboard.end(),
              [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });
    if (leaderboard.size() > 10) leaderboard.resize(10);
    SaveLeaderboard();
}

// ========== 初始化 ==========
void Game::Init() {
    LoadBaseConfig("config.json");   // 加载基础配置（包含 powerups 等）
    if (currentLevel >= 1) {
        std::string levelFile = "level_" + std::to_string(currentLevel) + ".json";
        LoadLevelConfig(levelFile);  // 只覆盖关卡特定配置（球速、砖块等）
    }
    LoadLeaderboard();
     // ========== 加载背景纹理 ==========
    backgroundPath = "assets/background_level" + std::to_string(currentLevel) + ".png";
    
    // 先检查缓存
    if (TextureCache::Instance().HasTexture(backgroundPath)) {
        currentBackground = TextureCache::Instance().GetTexture(backgroundPath);
        backgroundLoading = false;
        std::cout << "[Game] Using cached texture: " << backgroundPath << std::endl;
    } else {
        // 尝试直接加载（如果文件存在）
        Texture2D tex = LoadTexture(backgroundPath.c_str());
        if (tex.id != 0) {
            TextureCache::Instance().AddTexture(backgroundPath, tex);
            currentBackground = tex;
            backgroundLoading = false;
            std::cout << "[Game] Texture loaded: " << backgroundPath 
                      << " (" << tex.width << "x" << tex.height << ")" << std::endl;
        } else {
            // 文件不存在，使用降级方案
            currentBackground = { };
            backgroundLoading = false;
            std::cout << "[Game] No texture found, using solid color background" << std::endl;
        }
    }

    balls.clear();
    balls.emplace_back(Vector2{(float)SW/2, (float)SH/2},
                       Vector2{config.ballSpeedX, config.ballSpeedY},
                       config.ballRadius);
    
     balls[0].SetAttached(true);  // 球附着在挡板上
    gameStarted = false;         // 等待空格发射

    paddle = Paddle((SW - config.paddleWidth) / 2,
                    SH - config.paddleYOffset,
                    config.paddleWidth,
                    config.paddleHeight,
                    config.paddleSpeed);
    BuildBricks();
    
    powerUps.clear();
    particles.clear();
    
    particles.resize(MAX_PARTICLES);
    for (auto& p : particles) {
        p.active = false;
    }

    lives = config.initialLives;

    networkManager.Initialize();
    networkUpdateTimer = 0.0f;
}
void Game::LoadBaseConfig(const std::string& path) {
    // 加载完整的 config.json（和原来的 LoadConfig 一样）
    LoadConfig(path);  // 重命名原来的 LoadConfig
}

void Game::LoadLevelConfig(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Warning: Cannot open level file: " << path << std::endl;
            return;
        }

        json levelJson = json::parse(file);
        
        // 只更新关卡相关的配置
        auto ballCfg = levelJson.value("ball", json::object());
        if (!ballCfg.empty()) {
            config.ballRadius = ballCfg.value("radius", config.ballRadius);
            config.ballSpeedX = ballCfg.value("speedX", config.ballSpeedX);
            config.ballSpeedY = ballCfg.value("speedY", config.ballSpeedY);
            config.gravity = ballCfg.value("gravity", config.gravity);
            config.maxSpeed = ballCfg.value("maxSpeed", config.maxSpeed);
            config.bounceForce = ballCfg.value("bounceForce", config.bounceForce);
        }

        auto paddleCfg = levelJson.value("paddle", json::object());
        if (!paddleCfg.empty()) {
            config.paddleWidth = paddleCfg.value("width", config.paddleWidth);
            config.paddleHeight = paddleCfg.value("height", config.paddleHeight);
            config.paddleSpeed = paddleCfg.value("speed", config.paddleSpeed);
            config.paddleYOffset = paddleCfg.value("yOffset", config.paddleYOffset);
            config.paddleBoostSpeed = paddleCfg.value("boostSpeed", config.paddleBoostSpeed);
        }

        auto bricksCfg = levelJson.value("bricks", json::object());
        if (!bricksCfg.empty()) {
            config.brickRows = bricksCfg.value("rows", config.brickRows);
            config.brickCols = bricksCfg.value("cols", config.brickCols);
            config.brickWidth = bricksCfg.value("width", config.brickWidth);
            config.brickHeight = bricksCfg.value("height", config.brickHeight);
            config.brickStartX = bricksCfg.value("startX", config.brickStartX);
            config.brickStartY = bricksCfg.value("startY", config.brickStartY);
            config.brickPaddingX = bricksCfg.value("paddingX", config.brickPaddingX);
            config.brickPaddingY = bricksCfg.value("paddingY", config.brickPaddingY);
        }

        auto gameCfg = levelJson.value("game", json::object());
        if (!gameCfg.empty()) {
            config.initialLives = gameCfg.value("initialLives", config.initialLives);
            config.scorePerBrick = gameCfg.value("scorePerBrick", config.scorePerBrick);
            config.timeMultiplierDecay = gameCfg.value("timeMultiplierDecay", config.timeMultiplierDecay);
        }

        // 注意：powerups 只从基础配置加载，关卡配置不覆盖道具设置
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading level config: " << e.what() << std::endl;
    }
}

// ========== 构建砖块 ==========

void Game::BuildBricks() {
    bricks.clear();
    
    Color colors[] = {RED, ORANGE, YELLOW, GREEN, BLUE};
    int points[] = {50, 40, 30, 20, 10};

    int cols = config.brickCols;
    float padding = 5.0f; 
    float startX = config.brickStartX;
    
    // 第一关特殊处理：增加左右边距
    if (currentLevel == 1) startX += 40.0f; 

    int rows = (currentLevel == 1) ? 4 : 5;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            // 确定血量
            int hp = 1;
            bool isInvincible = false;

            if (currentLevel == 1) {
                hp = (r < 2) ? 2 : 1;
            } 
            else if (currentLevel == 2) {
                if (r == 0) hp = 3;
                else if (r < 3) hp = 2;
                else hp = 1;
            } 
            else if (currentLevel == 3) {
                if (r == 0) hp = 3;
                else if (r == 1) hp = 2;
                else if (r == 2) {
                    hp = 2;
                    // 特定位置放置无敌砖块
                    if (c == 2 || c == 7) { 
                        isInvincible = true;
                    }
                }
                else hp = 1;
            }

            // 计算位置
            float x = startX + c * (config.brickWidth + padding);
            float y = config.brickStartY + r * (config.brickHeight + padding);
            
            // 第二关错位布局
            if (currentLevel == 2 && r % 2 == 0) {
                x += config.brickWidth / 2.0f;
            }

            // 创建砖块
            if (isInvincible) {
                bricks.emplace_back(x, y, config.brickWidth, config.brickHeight, 
                                  GOLD, 999, 999, currentLevel);
            } else {
                Color brickColor = colors[r % 5];
                int brickPoints = points[r % 5];
                bricks.emplace_back(x, y, config.brickWidth, config.brickHeight, 
                                  brickColor, brickPoints, hp, currentLevel);
            }
        }
    }
}

// ========== 重置游戏 ==========
void Game::ResetGame() {
    std::string levelFile = "level_" + std::to_string(currentLevel) + ".json";
    LoadLevelConfig(levelFile);
    score = 0;
    lives = config.initialLives;
    BuildBricks();
    
    balls.clear();
    
    balls.emplace_back(Vector2{(float)SW/2, (float)SH/2},
                       Vector2{config.ballSpeedX, config.ballSpeedY},
                       config.ballRadius);
    
    paddle = Paddle((SW - config.paddleWidth) / 2,
                    SH - config.paddleYOffset,
                    config.paddleWidth,
                    config.paddleHeight,
                    config.paddleSpeed);
    
    isEnteringName = false;
    playerName = "";
    
    powerUps.clear();
    particles.clear();
    
    particles.resize(MAX_PARTICLES);
    for (auto& p : particles) {
        p.active = false;
    }
    gameStarted = false; 
}

// ========== 状态转换 ==========
void Game::ChangeState(GameState newState) {
    state = newState;
}

// ========== 道具效果 ==========
void Game::AddMultiBall(int count) {
    std::cout << "AddMultiBall: BEFORE size=" << balls.size() << ", adding " << count << std::endl;
    if (balls.empty()) {
        std::cout << "AddMultiBall: balls empty, returning!" << std::endl;
        return;
    }
    
    // 从挡板中间上方生成新球
    float baseX = paddle.GetRect().x + paddle.GetRect().width / 2;
    float baseY = paddle.GetRect().y - 50;
    float speed = std::abs(config.ballSpeedY);  // ✅ 就改这一行
    
    for (int i = 0; i < count; ++i) {
        float angle = GetRandomValue(-30, 30) * DEG2RAD;
        Vector2 vel = { sinf(angle) * speed, -speed };
        balls.emplace_back(Vector2{baseX, baseY}, vel, config.ballRadius);
        balls.back().SetAttached(false);
        std::cout << "    New ball speed: (" << balls.back().GetSpeed().x << ", " << balls.back().GetSpeed().y << ")" << std::endl;
        std::cout << "  Created ball " << i+1 << " at (" << baseX << ", " << baseY << ")" << std::endl;
    }
    
    std::cout << "AddMultiBall: AFTER size=" << balls.size() << std::endl;
}

void Game::SlowDownBalls(float factor, float duration) {
    if (slowEffectRemainingTime <= 0.0f) {
        for (auto& ball : balls) {
            Vector2 spd = ball.GetSpeed();
            ball.SetSpeed({spd.x * factor, spd.y * factor});
        }
    }
    slowEffectRemainingTime = duration;
}
void Game::EnableFireBall(float duration) {
    // 遍历所有的球，给每一个球都开启火球状态
    for (auto& ball : balls) {
        // 设置为 true 并刷新持续时间
        ball.SetFire(duration); 
    }
    std::cout << "All balls are now fireballs for " << duration << " seconds." << std::endl;
}
// ========== 道具系统 ==========
void Game::SpawnPowerUp(float x, float y, PowerUpType type) {
    powerUps.emplace_back(x, y, type);
}

void Game::UpdatePowerUps(float dt) {
    for (auto& p : powerUps) {
        p.Update(dt);
    }
    powerUps.erase(std::remove_if(powerUps.begin(), powerUps.end(),
        [](const PowerUp& p) { return !p.IsActive(); }), powerUps.end());
}

void Game::HandlePowerUpCollisions() {
    Rectangle paddleRect = paddle.GetRect();
    for (auto& p : powerUps) {
        if (p.IsActive() && CheckCollisionRecs(p.GetRect(), paddleRect)) {
            p.Apply(*this);
            p.Deactivate();
            SpawnPowerUpGlow({p.GetRect().x + p.GetRect().width/2, p.GetRect().y + p.GetRect().height/2}, GREEN);
        }
    }
}

// ========== 粒子系统 ==========
void Game::SpawnParticles(Rectangle rect, Color color) {
    int spawned = 0;
    for (auto& p : particles) {
        if (!p.active && spawned < 12) {
            p.active = true;
            p.position = { rect.x + GetRandomValue(0, (int)rect.width),
                           rect.y + GetRandomValue(0, (int)rect.height) };
            p.velocity = { (GetRandomValue(-50, 50) / 10.0f),
                           (GetRandomValue(-50, 50) / 10.0f) };
            p.color = color;
            p.life = 0.6f;
            p.maxLife = 0.6f;
            spawned++;
        }
    }
}

void Game::SpawnPowerUpGlow(Vector2 pos, Color color) {
    int spawned = 0;
    for (auto& p : particles) {
        if (!p.active && spawned < 8) {
            p.active = true;
            p.position = pos;
            p.velocity = { (GetRandomValue(-40, 40) / 10.0f),
                           (GetRandomValue(-40, 40) / 10.0f) };
            p.color = color;
            p.life = 0.4f;
            p.maxLife = 0.4f;
            spawned++;
        }
    }
}

void Game::UpdateParticles(float dt) {
    for (auto& p : particles) {
        if (!p.active) continue;
        
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.life -= dt;
        
        if (p.position.x - 3 <= 0) { p.position.x = 3; p.velocity.x = -p.velocity.x* 0.5f; }
        if (p.position.x + 3 >= SW) { p.position.x = SW - 3; p.velocity.x = -p.velocity.x* 0.5f; }
        if (p.position.y - 3 <= 0) { p.position.y = 3; p.velocity.y = -p.velocity.y* 0.5f; }
        if (p.position.y + 3 >= SH) { p.active = false; continue; }
        
        if (p.life <= 0) {
            p.active = false;
        }
    }
}

void Game::DrawParticles() {
    for (const auto& p : particles) {
        if (!p.active) continue;
        unsigned char alpha = (unsigned char)(255 * (p.life / p.maxLife));
        DrawCircleV(p.position, 3, Fade(p.color, alpha / 255.0f));
    }
}


// ========== 主更新循环 ==========
void Game::Update() {
    // 全局快捷键：查看排行榜
    if (state != GameState::LEADERBOARD && IsKeyPressed(KEY_L)) {
        LoadLeaderboard();
        ChangeState(GameState::LEADERBOARD);
        return;
    }
    
    float dt = GetFrameTime();
    
    CheckAsyncLoading();

    // 更新减速效果计时器（保持原有逻辑）
    if (slowEffectRemainingTime > 0.0f) {
        slowEffectRemainingTime -= dt;
        if (slowEffectRemainingTime <= 0.0f) {
            for (auto& ball : balls) {
                ball.SetSpeed(Vector2{config.ballSpeedX, config.ballSpeedY});
            }
        }
    }
    
    switch (state) {
        case GameState::MENU:
            HandleMenuState();
            break;

        case GameState::LEVEL_SELECT: 
            HandleLevelSelectState();
            break; 
            
        case GameState::PLAYING:
            HandlePlayingState(dt);
            break;
            
        case GameState::PAUSED:
            HandlePausedState();
            break;
            
        case GameState::GAMEOVER:
            HandleGameOverState();
            break;
            
        case GameState::VICTORY:
            HandleWinState();
            break;
            
        case GameState::LEADERBOARD:
            HandleLeaderboardState();
            break;
    }
    // 网络服务
    networkManager.Service();
    
    if (state == GameState::PLAYING) {
        UpdateNetwork();
    }
}

// ========== 状态处理函数 ==========
void Game::HandleMenuState() {
    if (IsKeyPressed(KEY_SPACE)) {
        ChangeState(GameState::LEVEL_SELECT);
    }
    if (IsKeyPressed(KEY_H)) {
        // 作为主机开始
        StartAsHost();
        ChangeState(GameState::PLAYING);
    }
    if (IsKeyPressed(KEY_C)) {
        // 作为客户端连接
        StartAsClient("127.0.0.1"); // 本地测试
        ChangeState(GameState::PLAYING);
    
    }
}

void Game::HandleLevelSelectState() {
    if (IsKeyPressed(KEY_ONE)) StartLevel(1);
    if (IsKeyPressed(KEY_TWO)) StartLevel(2);
    if (IsKeyPressed(KEY_THREE)) StartLevel(3);
    
    if (IsKeyPressed(KEY_ESCAPE)) {
        ChangeState(GameState::MENU);
    }
}

void Game::HandlePlayingState(float dt) {
    // 暂停键（加载中也可以暂停）
    if (IsKeyPressed(KEY_P)) {
        ChangeState(GameState::PAUSED);
        return;
    }
    
    // 测试异步加载 - 按 T 键触发（仅在空闲时）
    if (IsKeyPressed(KEY_T) && loadState == LoadState::IDLE) {
        int nextLevel = (currentLevel % 3) + 1;  // 循环1-3关
        StartAsyncLevelLoad(nextLevel);
    }
    
    // 🚫 加载中：跳过物理和碰撞逻辑，只允许移动挡板
    if (loadState == LoadState::LOADING) {
        // 保留挡板移动
        if (isNetworkGame) {
            if (localRole == NetworkRole::HOST) {
                HandleHostInput(dt);
            } else {
                HandleClientInput(dt);
            }
        } else {
            HandleLocalInput(dt);
        }
        // 更新道具和粒子（保持视觉连续性）
        UpdatePowerUps(dt);
        UpdateParticles(dt);
        return;
    }
    
    // --- 以下为正常游戏逻辑 ---
    
    // 1. 重置砖块帧标记
    for (auto& brick : bricks) {
        brick.SetHitThisFrame(false);
    }
    
    // 2. 玩家输入控制
    if (isNetworkGame) {
        if (localRole == NetworkRole::HOST) {
            // 主机控制底部挡板
            HandleHostInput(dt);
            // 主机负责物理计算
            UpdateBallsPhysics(dt);
            HandleBallBrickCollisions();
        } else {
            // 客户端控制底部挡板，发送输入给主机
            HandleClientInput(dt);
            // 客户端也进行物理预测
            UpdateClientPrediction(dt);
        }
    } else {
        // 单人模式：正常控制
        HandleLocalInput(dt);
        // 单人模式物理计算
        UpdateBallsPhysics(dt);
        HandleBallBrickCollisions();
    }

    // 3. 更新游戏对象和球状态
    UpdatePowerUps(dt);
    HandlePowerUpCollisions();
    UpdateParticles(dt);

    // 4. 火球计时器递减
    for (auto& ball : balls) {
        ball.Update(dt);
    }
    
    // 5. 发射逻辑
    if (!gameStarted && IsKeyPressed(KEY_SPACE)) {
        LaunchBall();
    }
    
    // 6. 调试快捷键
    if (IsKeyPressed(KEY_K)) {
        powerUps.clear();
    }
    
    if (IsKeyPressed(KEY_END)) {
        isEnteringName = true;
        ChangeState(GameState::GAMEOVER);
        return;
    }
    
    // 7. 死亡检测与过关判定
    CheckBallLoss();
    
    if (state == GameState::GAMEOVER) return;
    
    CheckLevelCompletion();
}

void Game::HandleLocalInput(float dt) {
    if (IsKeyDown(KEY_LEFT)) paddle.MoveLeft(dt);
    if (IsKeyDown(KEY_RIGHT)) paddle.MoveRight(dt);
    paddle.ClampToScreen(SW);
}

void Game::HandleHostInput(float dt) {
    HandleLocalInput(dt);
}

void Game::HandleClientInput(float dt) {
    HandleLocalInput(dt);
}
void Game::UpdateClientPrediction(float dt) {
    // 客户端预测：根据本地状态更新球的位置
    for (auto& ball : balls) {
        if (ball.IsAttached()) {
            // 附着状态：跟随客户端挡板
            Rectangle paddleRect = paddle.GetRect();
            ball.SetPosition({
                paddleRect.x + paddleRect.width / 2,
                paddleRect.y - ball.GetRadius()
            });
        } else {
            // 非附着状态：根据速度自行推演位置
            ball.Move(dt);
            // 简单的边缘反弹预测
            ball.BounceEdge(SW, SH);
        }
    }
}
void Game::HandleBallBrickCollisions() {
    for (auto& ball : balls) {
        if (ball.IsAttached()) continue;
        
        // 火球和普通球使用不同的碰撞处理
        if (ball.IsFireball()) {
            // ========== 火球模式：穿透并摧毁 ==========
            for (auto& brick : bricks) {
                if (!brick.IsActive() || brick.HitThisFrame()) continue;
                
                Rectangle rect = brick.GetRect();
                auto side = ball.CheckBrickCollision(rect);
                
                if (side == CollisionSide::NONE) continue;
                
                // 跳过无敌砖块（火球也不能摧毁）
                if (brick.GetHealth() >= 999) {
                    brick.SetHitThisFrame(true);
                    // 无敌砖块反弹火球
                    if (side == CollisionSide::LEFT || side == CollisionSide::RIGHT) {
                        ball.ReverseX();
                    } else {
                        ball.ReverseY();
                    }
                   continue;
                }
                
                // 火球摧毁普通砖块
                brick.Destroy();
                score += config.scorePerBrick;
                SpawnParticles(rect, brick.GetColor());
                
                // 道具掉落
                TrySpawnPowerUp(rect);
                
                brick.SetHitThisFrame(true);
                // 火球不反弹，继续穿透下一个砖块
            }
        } else {
            // ========== 普通球模式：单次碰撞并反弹 ==========
           
            for (auto& brick : bricks) {
                if (!brick.IsActive() || brick.HitThisFrame()) continue;
                
                Rectangle rect = brick.GetRect();
                auto side = ball.CheckBrickCollision(rect);
                
                if (side == CollisionSide::NONE) continue;
                
                // 处理无敌砖块
                if (brick.GetHealth() >= 999) {
                    // 反弹球
                    if (side == CollisionSide::LEFT || side == CollisionSide::RIGHT) {
                        ball.ReverseX();
                        ball.SetPosition({
                            side == CollisionSide::LEFT ? 
                            rect.x - ball.GetRadius() : 
                            rect.x + rect.width + ball.GetRadius(),
                            ball.GetPosition().y
                        });
                    } else {
                        ball.ReverseY();
                        ball.SetPosition({
                            ball.GetPosition().x,
                            side == CollisionSide::TOP ? 
                            rect.y - ball.GetRadius() : 
                            rect.y + rect.height + ball.GetRadius()
                        });
                    }
                    brick.SetHitThisFrame(true);
                    break;
                }
                
                // 普通砖块扣血
                bool destroyed = brick.Hit();
                score += config.scorePerBrick;
                
                if (destroyed) {
                    SpawnParticles(rect, brick.GetColor());
                    TrySpawnPowerUp(rect);
                }
                
                // 反弹球
                if (side == CollisionSide::LEFT || side == CollisionSide::RIGHT) {
                    ball.ReverseX();
                    ball.SetPosition({
                        side == CollisionSide::LEFT ? 
                        rect.x - ball.GetRadius() : 
                        rect.x + rect.width + ball.GetRadius(),
                        ball.GetPosition().y
                    });
                } else {
                    ball.ReverseY();
                    ball.SetPosition({
                        ball.GetPosition().x,
                        side == CollisionSide::TOP ? 
                        rect.y - ball.GetRadius() : 
                        rect.y + rect.height + ball.GetRadius()
                    });
                }
                
                brick.SetHitThisFrame(true);
                break; // 普通球只处理一个碰撞
            }
        }
    }
}

void Game::UpdateBallsPhysics(float dt) {
    for (auto& ball : balls) {
        if (ball.IsAttached()) {
            Rectangle paddleRect = paddle.GetRect();
            ball.SetPosition({
                paddleRect.x + paddleRect.width / 2,
                paddleRect.y - ball.GetRadius()
            });
            continue;
        }
        
        ball.Move(dt);
        ball.BounceEdge(SW, SH);
    }
    
    // 球与两个挡板的碰撞检测
    for (auto& ball : balls) {
        ball.CheckPaddleCollision(paddle.GetRect());   // 底部挡板（主机控制）
        
        if (isNetworkGame) {
            ball.CheckPaddleCollision(paddle2.GetRect());  // 顶部挡板（客户端控制）
        }
    }
}


// 新增辅助函数：统一处理道具掉落
void Game::TrySpawnPowerUp(Rectangle brickRect) {
    static float lastPowerUpDrop = 0.0f;
    float currentTime = GetTime();
    
    // 冷却时间防止道具泛滥
    if (currentTime - lastPowerUpDrop < 0.3f) return;
    
    // 随机选择道具类型
    PowerUpType type = static_cast<PowerUpType>(GetRandomValue(0, 3));
    float dropRate = 0.0f;
    
    // 根据道具类型获取对应的掉率
    switch (type) {
        case PowerUpType::PADDLE_EXTEND:
            dropRate = config.powerups.paddle_extend.drop_rate;
            break;
        case PowerUpType::MULTI_BALL:
            dropRate = config.powerups.multi_ball.drop_rate;
            break;
        case PowerUpType::SLOW_BALL:
            dropRate = config.powerups.slow_ball.drop_rate;
            break;
        case PowerUpType::FIRE_BALL:
            dropRate = config.powerups.fire_ball.drop_rate;
            break;
    }
    
    // 检查是否掉落
    if (GetRandomValue(0, 99) < (int)(dropRate * 100)) {
        SpawnPowerUp(
            brickRect.x + brickRect.width / 2, 
            brickRect.y + brickRect.height / 2, 
            type
        );
        lastPowerUpDrop = currentTime;
    }
}

void Game::LaunchBall() {
    gameStarted = true;
    float launchSpeedX = 0.0f;
    float paddleInfluence = 250.0f;

    if (IsKeyDown(KEY_LEFT)) {
        launchSpeedX = -paddleInfluence;
    } else if (IsKeyDown(KEY_RIGHT)) {
        launchSpeedX = paddleInfluence;
    }

    for (auto& ball : balls) {
        ball.SetAttached(false);
        ball.SetSpeed({ launchSpeedX, config.ballSpeedY });
        
        float centerX = paddle.GetPosition().x + paddle.GetWidth() / 2.0f;
        float centerY = paddle.GetPosition().y - ball.GetRadius();
        ball.SetPosition({ centerX, centerY });
    }
}

void Game::CheckBallLoss() {
    // 移除掉出屏幕的球
    for (auto it = balls.begin(); it != balls.end(); ) {
        if (it->GetPosition().y + it->GetRadius() >= SH) {
            it = balls.erase(it);
        } else {
            ++it;
        }
    }
    
    // 没有球了
    if (balls.empty()) {
        lives--;
        if (lives <= 0 && !godMode) {
            isEnteringName = true;
            ChangeState(GameState::GAMEOVER);
        } else {
            // 生成新球并重置为附着状态
            balls.emplace_back(Vector2{(float)SW/2, (float)SH/2},
                               Vector2{config.ballSpeedX, config.ballSpeedY},
                               config.ballRadius);
            balls.back().SetAttached(true);
            gameStarted = false;
        }
    }
}

void Game::CheckLevelCompletion() {
    // 如果正在异步加载中，不要重复触发
    if (loadState == LoadState::LOADING) return;
    
    // 计算活跃砖块数量（排除无敌砖块）
    int activeBricks = 0;
    for (auto& brick : bricks) {
        if (brick.IsActive() && brick.GetHealth() < 10) activeBricks++;
    }
    
    // 所有普通砖块都被清除
    if (activeBricks == 0) {
        if (currentLevel >= 3) {
            // 通关所有关卡
            isEnteringName = true;
            ChangeState(GameState::GAMEOVER);
            return;
        }
        
        // ✅ 启动异步加载下一关（只启动一次）
        StartAsyncLevelLoad(currentLevel + 1);
    }
}



void Game::HandlePausedState() {
    if (IsKeyPressed(KEY_P)) {
        ChangeState(GameState::PLAYING);
    }
}

void Game::HandleGameOverState() {
    if (isEnteringName) {
        // 输入名字状态
        if (IsKeyPressed(KEY_ENTER)) {
            AddScoreToLeaderboard();
            isEnteringName = false;
            
            // 重置游戏
            score = 0;
            lives = config.initialLives;
            currentLevel = 1;
            paddle.ResetWidth();
            
            ChangeState(GameState::MENU);
        }
    } else {
        // 正常游戏结束状态
        if (IsKeyPressed(KEY_R)) {
            score = 0;
            lives = config.initialLives;
            currentLevel = 1;
            paddle.ResetWidth();
            ChangeState(GameState::MENU);
        }
    }
}

void Game::HandleWinState() {
    if (isEnteringName) {
        if (IsKeyPressed(KEY_ENTER)) {
            AddScoreToLeaderboard();
            isEnteringName = false;
            ChangeState(GameState::MENU);
        }
    } else {
        if (IsKeyPressed(KEY_R)) {
            score = 0;
            lives = config.initialLives;
            currentLevel = 1;
            paddle.ResetWidth();
            ChangeState(GameState::MENU);
        }
    }
}

void Game::HandleLeaderboardState() {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_R)) {
        ChangeState(GameState::MENU);
    }
}
void Game::UpdateNetwork() {
    if (!isNetworkGame) return;
    
    networkUpdateTimer += GetFrameTime();
    
    if (networkUpdateTimer >= NETWORK_UPDATE_RATE) {
        networkUpdateTimer = 0.0f;
        
        if (localRole == NetworkRole::HOST) {
            // 主机：发送游戏状态给客户端
            SendGameStateToPeer();
            
            // 接收客户端的输入
            PlayerInput input;
            if (networkManager.ReceivePlayerInput(input)) {
                // 更新客户端挡板位置
                paddle2.SetPosition(input.paddleX, paddle2.GetPosition().y);
                
                // 如果客户端按了空格，也发射球
                if (input.spacePressed && !gameStarted) {
                    LaunchBall();
                }
            }
        } else if (localRole == NetworkRole::CLIENT) {
            // 客户端：发送输入给主机
            SendClientInputToHost();
            
            // 接收主机状态并进行平滑修正
            ReceiveAndSmoothHostState();
        }
    }
}

void Game::SendClientInputToHost() {
    PlayerInput input;
    input.paddleX = paddle.GetPosition().x;
    input.paddleY = paddle.GetPosition().y;
    input.leftPressed = IsKeyDown(KEY_LEFT);
    input.rightPressed = IsKeyDown(KEY_RIGHT);
    input.spacePressed = IsKeyPressed(KEY_SPACE);
    input.timestamp = GetTime();
    networkManager.SendPlayerInput(input);
}

void Game::ReceiveAndSmoothHostState() {
    GameStateSnapshot state;
    if (networkManager.ReceiveGameState(state)) {
        // 添加快照到插值管理器
        interpolationManager.AddSnapshot(state);
        
        // 获取当前渲染时间（减去延迟补偿）
        double renderTime = GetTime() - 0.1; // 100ms 延迟补偿
        InterpolatedState interpolated = interpolationManager.GetInterpolatedState(renderTime);
        
        // 平滑修正球的状态
        if (!balls.empty()) {
            Ball& ball = balls[0];
            Vector2 currentPos = ball.GetPosition();
            Vector2 targetPos = {interpolated.ballX, interpolated.ballY};
            
            // 线性插值平滑（lerp 因子 0.3 提供平滑过渡）
            float lerpFactor = 0.3f;
            Vector2 smoothedPos = {
                currentPos.x + (targetPos.x - currentPos.x) * lerpFactor,
                currentPos.y + (targetPos.y - currentPos.y) * lerpFactor
            };
            
            ball.SetPosition(smoothedPos);
            
            // 更新速度为权威服务器的值
            ball.SetSpeed({state.ballSpeedX, state.ballSpeedY});
            
            // 同步火球状态
            if (state.isFireball && !ball.IsFireball()) {
                ball.SetFire(config.powerups.fire_ball.duration);
            } else if (!state.isFireball && ball.IsFireball()) {
                 ball.ClearFire(); 
            }
        }
        
        // 更新游戏状态
        score = state.score;
        lives = state.lives;
        currentLevel = state.currentLevel;
        
        // 更新主机挡板位置（客户端显示用）
        paddle2.SetPosition(state.paddle1X, paddle2.GetPosition().y);
    }
}
void Game::SendGameStateToPeer() {
    GameStateSnapshot state;
    
    if (!balls.empty()) {
        state.ballX = balls[0].GetPosition().x;
        state.ballY = balls[0].GetPosition().y;
        state.ballSpeedX = balls[0].GetSpeed().x;
        state.ballSpeedY = balls[0].GetSpeed().y;
        state.isFireball = balls[0].IsFireball();
    }
    
    state.paddle1X = paddle.GetPosition().x;
    state.paddle1Y = paddle.GetPosition().y;
    state.paddle2X = paddle2.GetPosition().x;
    state.paddle2Y = paddle2.GetPosition().y;
    state.score = score;
    state.lives = lives;
    state.currentLevel = currentLevel;
    state.timestamp = GetTime();
    
    networkManager.SendGameState(state);
}

void Game::StartAsHost() {
    if (networkManager.StartHost()) {
        TraceLog(LOG_INFO, "Game started as HOST - Waiting for client...");
        
        // 完全重置游戏
        score = 0;
        lives = config.initialLives;
        currentLevel = 1;
        isNetworkGame = true;
        localRole = NetworkRole::HOST;
        
        // 确保基础配置已加载
        LoadBaseConfig("config.json");
        std::string levelFile = "level_" + std::to_string(currentLevel) + ".json";
        LoadLevelConfig(levelFile);
        powerUpDropRate = config.powerups.paddle_extend.drop_rate;

        // 创建球
        balls.clear();
        balls.emplace_back(
            Vector2{(float)SW/2, (float)SH/2},
            Vector2{config.ballSpeedX, config.ballSpeedY},
            config.ballRadius
        );
        balls[0].SetAttached(true);
        
        // 初始化主机挡板（底部）
        paddle = Paddle(
            (SW - config.paddleWidth) / 2,
            SH - config.paddleYOffset,
            config.paddleWidth,
            config.paddleHeight,
            config.paddleSpeed
        );
        
        // 初始化客户端挡板（顶部）
        paddle2 = Paddle(
            (SW - config.paddleWidth) / 2,
            config.paddleYOffset,  // 放在顶部
            config.paddleWidth,
            config.paddleHeight,
            config.paddleSpeed
        );
        
        // 重新构建砖块
        BuildBricks();
        
        // 清除道具和粒子
        powerUps.clear();
        particles.clear();
        particles.resize(MAX_PARTICLES);
        for (auto& p : particles) {
            p.active = false;
        }
        
        // 重置状态
        gameStarted = false;
        isEnteringName = false;
        playerName = "";
        slowEffectRemainingTime = 0.0f;
        
        state = GameState::PLAYING;
        
        TraceLog(LOG_INFO, "Host game initialized. Waiting for client connection and SPACE to start.");
    }
}
void Game::StartAsClient(const std::string& ip) {
    if (networkManager.ConnectToHost(ip)) {
        TraceLog(LOG_INFO, "Game started as CLIENT - Connected to host");
        
        // 客户端初始化
        score = 0;
        lives = config.initialLives;
        currentLevel = 1;
        isNetworkGame = true;
        localRole = NetworkRole::CLIENT;
        
        // 加载配置 
        LoadBaseConfig("config.json");
        std::string levelFile = "level_" + std::to_string(currentLevel) + ".json";
        LoadLevelConfig(levelFile);
        powerUpDropRate = config.powerups.paddle_extend.drop_rate;
        
        // 创建球（客户端不进行物理计算，仅用于显示）
        balls.clear();
        balls.emplace_back(
            Vector2{(float)SW/2, (float)SH/2},
            Vector2{config.ballSpeedX, config.ballSpeedY},
            config.ballRadius
        );
        balls[0].SetAttached(true);
        
        // 初始化客户端挡板（底部 - 客户端控制底部挡板）
        paddle = Paddle(
            (SW - config.paddleWidth) / 2,
            SH - config.paddleYOffset,
            config.paddleWidth,
            config.paddleHeight,
            config.paddleSpeed
        );
        
        // 初始化主机挡板显示（顶部）
        paddle2 = Paddle(
            (SW - config.paddleWidth) / 2,
            config.paddleYOffset,
            config.paddleWidth,
            config.paddleHeight,
            config.paddleSpeed
        );
        
        // 构建砖块（与主机相同）
        BuildBricks();
        
        // 清除道具和粒子
        powerUps.clear();
        particles.clear();
        particles.resize(MAX_PARTICLES);
        for (auto& p : particles) {
            p.active = false;
        }
        
        // 重置状态
        gameStarted = false;
        isEnteringName = false;
        playerName = "";
        slowEffectRemainingTime = 0.0f;
        
        state = GameState::PLAYING;
        
        TraceLog(LOG_INFO, "Client game initialized. Waiting for host to start.");
    }
}

// ========== 绘制 ==========
void Game::Draw() {
    if (currentBackground.id != 0) {
        // 有纹理：绘制纹理背景
        DrawTexture(currentBackground, 0, 0, WHITE);
    } else {
        // 降级方案：纯色背景
        Color bg;
        if (currentLevel == 1) {
            bg = (Color){ 10, 25, 10, 255 };
        } else if (currentLevel == 2) {
            bg = (Color){ 25, 15, 10, 255 };
        } else {
            bg = (Color){ 5, 10, 25, 255 };
        }
        ClearBackground(bg);
    }
    switch (state) {
        case GameState::MENU: DrawMenu(); break;
       case GameState::LEVEL_SELECT: DrawLevelSelect(); break;
        case GameState::PLAYING: DrawPlaying(); break;
        case GameState::PAUSED: DrawPaused(); break;
        case GameState::GAMEOVER: DrawGameOver(); break;
        case GameState::VICTORY: DrawWin(); break;
        case GameState::LEADERBOARD: DrawLeaderboard(); break;
    }
}
void Game::DrawMenu() {
    DrawText("ARKANOID 2D", SW/2 - 150, 150, 40, WHITE);
    DrawText("PRESS SPACE - SOLO MODE", SW/2 - 130, 230, 24, RAYWHITE);
    DrawText("PRESS H - HOST CO-OP MODE", SW/2 - 130, 270, 24, YELLOW);
    DrawText("PRESS C - JOIN CO-OP MODE", SW/2 - 130, 310, 24, ORANGE);
    DrawText("L: LEADERBOARD | P: PAUSE", SW/2 - 130, 360, 20, LIGHTGRAY);
}

void Game::DrawPlaying() {
    DrawRectangle(0, 0, SW, 5, DARKGRAY);
    DrawRectangle(0, 0, 5, SH, DARKGRAY);
    DrawRectangle(SW-5, 0, 5, SH, DARKGRAY);

    // 绘制球
    for (auto& ball : balls) ball.Draw();
    
    // 绘制两个挡板
    paddle.Draw();
    if (isNetworkGame) {
        // 给客户端的挡板不同颜色以区分
        DrawRectangleRec(paddle2.GetRect(), RED);
        DrawRectangleLinesEx(paddle2.GetRect(), 2, DARKBLUE);
    }
    
    // 绘制砖块
    for (auto& b : bricks) if (b.IsActive()) b.Draw();
    
    // 绘制道具和粒子
    for (auto& p : powerUps) p.Draw();
    DrawParticles();
    
    // 绘制UI
    DrawText(TextFormat("SCORE: %d", score), 10, 10, 20, WHITE);
    DrawText(TextFormat("BALLS: %d", (int)balls.size()), 10, 35, 20, YELLOW);
    DrawText(TextFormat("LIVES: %d", lives), SW - 100, 10, 20, RED);
    
    // 网络状态提示
    if (isNetworkGame) {
        const char* roleText = (localRole == NetworkRole::HOST) ? "HOST" : "CLIENT";
        DrawText(roleText, SW/2 - 30, SH - 30, 20, GREEN);
    }
    
    if (debugMode) DrawDebugInfo();

      if (loadState == LoadState::LOADING) {
        // 半透明遮罩
        DrawRectangle(0, 0, SW, SH, ColorAlpha(BLACK, 0.75f));
        
        // 加载动画 - 旋转的圆环
        float centerX = SW / 2.0f;
        float centerY = SH / 2.0f;
        float radius = 40.0f;
        
        // 动画角度（每帧旋转）
        static float rotation = 0.0f;
        rotation += GetFrameTime() * 360.0f;  // 每秒转一圈
        if (rotation >= 360.0f) rotation -= 360.0f;
        
        // 绘制旋转圆环
        for (int i = 0; i < 8; i++) {
            float angle = rotation + i * 45.0f;
            float alpha = 1.0f - (i / 8.0f) * 0.7f;
            Color color = ColorAlpha(WHITE, alpha);
            
            float x = centerX + cosf(angle * DEG2RAD) * radius;
            float y = centerY + sinf(angle * DEG2RAD) * radius;
            DrawCircle(x, y, 5, color);
        }
        
        // 加载文字
        static float textTimer = 0.0f;
        textTimer += GetFrameTime();
        int dotCount = ((int)(textTimer * 2) % 4);
        std::string dots(dotCount, '.');
        
        DrawText(TextFormat("LOADING%s", dots.c_str()), 
                centerX - 80, centerY - 60, 30, WHITE);
        
        // 提示文字
        DrawText(TextFormat("Loading Level %d...", pendingLevelNumber),
                centerX - 100, centerY + 50, 18, LIGHTGRAY);
    }
}

void Game::DrawPaused() {
    DrawPlaying();
    DrawRectangle(0, 0, SW, SH, ColorAlpha(BLACK, 0.7f));
    DrawText("PAUSED", SW/2 - 80, SH/2 - 20, 48, YELLOW);
    DrawText("PRESS P TO RESUME", SW/2 - 120, SH/2 + 40, 24, WHITE);
}

void Game::DrawGameOver() {
    DrawRectangle(0, 0, SW, SH, ColorAlpha(BLACK, 0.8f));
    DrawText("GAME OVER", SW/2 - 100, SH/2 - 60, 48, RED);
    DrawText(TextFormat("YOUR SCORE: %d", score), SW/2 - 100, SH/2, 28, WHITE);
    if (isEnteringName) DrawNameInput();
    else DrawText("PRESS R TO MENU", SW/2 - 90, SH/2 + 60, 22, GRAY);
}

void Game::DrawWin() {
    DrawRectangle(0, 0, SW, SH, ColorAlpha(BLACK, 0.8f));
    DrawText("YOU WIN!", SW/2 - 80, SH/2 - 60, 48, GOLD);
    DrawText(TextFormat("YOUR SCORE: %d", score), SW/2 - 100, SH/2, 28, WHITE);
    if (isEnteringName) DrawNameInput();
    else DrawText("PRESS R TO MENU", SW/2 - 90, SH/2 + 60, 22, GRAY);
}

void Game::DrawLeaderboard() {
    DrawRectangle(0, 0, SW, SH, ColorAlpha(BLACK, 0.9f));
    DrawText("=== LEADERBOARD ===", SW/2 - 120, 50, 30, GOLD);
    DrawText("RANK", 150, 120, 20, WHITE);
    DrawText("NAME", 300, 120, 20, WHITE);
    DrawText("SCORE", 550, 120, 20, WHITE);
    DrawLine(100, 145, SW - 100, 145, GRAY);
    int y = 170;
    for (size_t i = 0; i < leaderboard.size() && i < 10; ++i) {
        Color color = (i == 0) ? GOLD : ((i == 1) ? LIGHTGRAY : ((i == 2) ? ORANGE : WHITE));
        DrawText(TextFormat("%d", i + 1), 160, y, 20, color);
        DrawText(leaderboard[i].name.c_str(), 300, y, 20, color);
        DrawText(TextFormat("%d", leaderboard[i].score), 550, y, 20, color);
        DrawText(leaderboard[i].date.c_str(), 650, y, 18, GRAY);
        y += 35;
    }
    DrawText("PRESS ESC OR R TO RETURN", SW/2 - 140, SH - 50, 20, GRAY);
}

void Game::DrawNameInput() {
    DrawRectangle(SW/2 - 150, SH/2 + 30, 300, 40, DARKGRAY);
    DrawRectangleLines(SW/2 - 150, SH/2 + 30, 300, 40, WHITE);
    std::string displayName = playerName.empty() ? "ENTER YOUR NAME" : playerName;
    DrawText(displayName.c_str(), SW/2 - 140, SH/2 + 40, 20, playerName.empty() ? GRAY : WHITE);
    if ((int)(GetTime() * 2) % 2 == 0) {
        int textWidth = MeasureText(displayName.c_str(), 20);
        DrawRectangle(SW/2 - 140 + textWidth, SH/2 + 40, 2, 20, WHITE);
    }
    DrawText("PRESS ENTER TO SAVE", SW/2 - 110, SH/2 + 80, 18, GRAY);
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 126 && playerName.length() < 20)
            playerName += (char)key;
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && !playerName.empty())
        playerName.pop_back();
}

void Game::DrawDebugInfo() {
    DrawText(TextFormat("PUs:%d", (int)powerUps.size()), 200, 10, 16, YELLOW);
    DrawText(TextFormat("Parts:%d", (int)particles.size()), 300, 10, 16, YELLOW);
    DrawText(TextFormat("Cache:%zu", TextureCache::Instance().Size()), 
             400, 10, 16, YELLOW);
    // ========== 新增：显示队列状态 ==========
    DrawText(TextFormat("Queue:%zu", loadResultQueue.Size()), 
             500, 10, 16, YELLOW);        
    DrawText("T: Async Load Test", 400, 30, 14, YELLOW);
}
// ========== 新增：异步加载实现 ==========

// 1. 启动异步关卡加载
void Game::StartAsyncLevelLoad(int levelNumber) {
    if (loadState == LoadState::LOADING) {
        std::cout << "Already loading, please wait..." << std::endl;
        return;
    }
    
    std::cout << "Starting async load for level " << levelNumber << std::endl;
    
    loadState = LoadState::LOADING;
    pendingLevelNumber = levelNumber;
    loadingLevelPath = "level_" + std::to_string(levelNumber) + ".json";
    
    // 使用 std::async 在新线程中执行加载
    loadFuture = std::async(std::launch::async, 
                           &Game::LoadLevelAssetsAsync, 
                           this, 
                           loadingLevelPath);
}

// 2. 后台线程执行的加载任务
bool Game::LoadLevelAssetsAsync(const std::string& levelPath) {
    std::cout << "[Worker Thread] Loading: " << levelPath << std::endl;
    std::cout << "[Worker Thread] Thread ID: " << std::this_thread::get_id() << std::endl;
    
    // 模拟耗时操作
    for (int i = 0; i < 20; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "[Worker Thread] Loading progress: " << (i + 1) * 5 << "%" << std::endl;
    }
    
    // 实际加载关卡配置文件
    try {
        std::ifstream file(levelPath);
        if (!file.is_open()) {
            // ========== 将失败结果推入队列 ==========
            LoadResult result;
            result.levelNumber = pendingLevelNumber;
            result.success = false;
            result.message = "Failed to open file";
            loadResultQueue.Push(result);
            // ======================================
            return false;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)), 
                           std::istreambuf_iterator<char>());
        
        if (content.empty()) {
            LoadResult result;
            result.levelNumber = pendingLevelNumber;
            result.success = false;
            result.message = "Empty file";
            loadResultQueue.Push(result);
            return false;
        }
        
        // ========== 将成功结果推入队列 ==========
        LoadResult result;
        result.levelNumber = pendingLevelNumber;
        result.success = true;
        result.message = "Loaded " + std::to_string(content.length()) + " bytes";
        loadResultQueue.Push(result);
        // ======================================
        
        std::cout << "[Worker Thread] Level file loaded successfully!" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        LoadResult result;
        result.levelNumber = pendingLevelNumber;
        result.success = false;
        result.message = e.what();
        loadResultQueue.Push(result);
        return false;
    }
}

// 3. 每帧检查异步加载状态
void Game::CheckAsyncLoading() {
      // ========== 处理队列中的加载结果 ==========
    LoadResult result;
    while (loadResultQueue.TryPop(result)) {
        std::cout << "[Main Thread] Queue message: Level " 
                  << result.levelNumber << " - " 
                  << (result.success ? "SUCCESS" : "FAILED")
                  << " - " << result.message << std::endl;
    }

    if (loadState != LoadState::LOADING) return;
    
    if (!loadFuture.valid()) {
        std::cerr << "[Error] Invalid future!" << std::endl;
        loadState = LoadState::IDLE;
        return;
    }
    
    // 非阻塞检查：wait_for(0) 立即返回
    auto status = loadFuture.wait_for(std::chrono::seconds(0));
    
    if (status == std::future_status::ready) {
        std::cout << "[Main Thread] Async loading completed!" << std::endl;
        
        try {
            bool result = loadFuture.get();
            
            if (result) {
                std::cout << "[Main Thread] Loading successful, applying level..." << std::endl;
                ApplyLoadedLevel();  // ✅ 只在这里调用
                loadState = LoadState::DONE;  // ✅ 然后立即设为 IDLE（在 ApplyLoadedLevel 中）
            } else {
                std::cerr << "[Main Thread] Loading failed!" << std::endl;
                loadState = LoadState::IDLE;
            }
        } catch (const std::exception& e) {
            std::cerr << "[Main Thread] Exception: " << e.what() << std::endl;
            loadState = LoadState::IDLE;
        }
    }
}

// 4. 应用加载完成的关卡数据
void Game::ApplyLoadedLevel() {
    std::cout << "[Main Thread] Applying level " << pendingLevelNumber << std::endl;
    
    // 切换到新关卡
    currentLevel = pendingLevelNumber;
    
    // 加载关卡配置
    std::string levelPath = "level_" + std::to_string(currentLevel) + ".json";
    LoadLevelConfig(levelPath);
    
    // 保持挡板宽度（道具效果可能会延续）
    float currentPaddleWidth = paddle.GetWidth();
    
    // 重建砖块
    BuildBricks();
    
    // 恢复或重置挡板宽度
    if (currentPaddleWidth > config.paddleWidth) {
        paddle.SetWidth(currentPaddleWidth);
    } else {
        paddle.SetWidth(config.paddleWidth);
    }
    
    // 清除所有道具（新关卡不应有道具）
    powerUps.clear();
    
    // 重置球的位置和状态
    balls.clear();
    balls.emplace_back(
        Vector2{(float)SW/2, (float)SH/2},
        Vector2{config.ballSpeedX, config.ballSpeedY},
        config.ballRadius
    );
    balls[0].SetAttached(true);
    
    // 重置游戏状态
    gameStarted = false;
    slowEffectRemainingTime = 0.0f;
    
    // 清除粒子效果
    for (auto& p : particles) {
        p.active = false;
    }
    
    std::cout << "[Main Thread] Level " << currentLevel << " applied! Press SPACE to launch ball." << std::endl;
    
    // ✅ 加载完成，回到空闲状态
    loadState = LoadState::IDLE;
}

void Game::Shutdown() {
    // 1. 保存排行榜数据
    SaveLeaderboard();
    
    // 2. 清理线程安全队列
    std::cout << "[ThreadSafeQueue] Clearing queue (" 
              << loadResultQueue.Size() << " items remaining)" << std::endl;
    loadResultQueue.Clear();
    loadResultQueue.Finish();  // 唤醒所有等待的线程
    
    // 3. 清理纹理缓存
    std::cout << "[TextureCache] Clearing " 
              << TextureCache::Instance().Size() << " textures..." << std::endl;
    TextureCache::Instance().Clear();
    
    // 4. 断开网络连接
    networkManager.Shutdown();
    
    std::cout << "[Game] Shutdown complete." << std::endl;
}



void Game::StartLevel(int level) {
    currentLevel = level;
    score = 0;           // 可选：是否每关重置分数
    lives = config.initialLives;
    
    // 1. 清理当前所有对象
    balls.clear();
    powerUps.clear();
    particles.clear();
    
    // 2. 初始化砖块 (BuildBricks 内部应根据 currentLevel 生成不同布局)
    BuildBricks(); 
    
    // 3. 创建初始球并设置为附着状态
    Ball startingBall({ 0, 0 }, { 0, 0 }, config.ballRadius);
    startingBall.SetAttached(true);
    balls.push_back(startingBall);
    
    // 4. 重置挡板位置
    paddle.SetPosition(SW / 2 - paddle.GetWidth() / 2, SH - config.paddleYOffset);
    
    // 5. 切换状态并标记游戏未开始（等待空格）
    gameStarted = false;
    state = GameState::PLAYING;
}

void Game::DrawLevelSelect() {
    DrawRectangle(0, 0, SW, SH, { 20, 20, 20, 200 }); // 半透明背景
    DrawText("SELECT YOUR CHALLENGE", SW/2 - 180, 150, 30, RAYWHITE);
    
    DrawText("[ 1 ] - LEVEL 1 (Beginner)", SW/2 - 120, 250, 20, GREEN);
    DrawText("[ 2 ] - LEVEL 2 (Intermediate)", SW/2 - 120, 300, 20, ORANGE);
    DrawText("[ 3 ] - LEVEL 3 (Expert)", SW/2 - 120, 350, 20, RED);
    
    DrawText("Press ESC to return", SW/2 - 80, 450, 15, GRAY);
}


void Game::InitBalls() {
    balls.clear();
    Ball newBall(
        Vector2{(float)SW/2, (float)SH/2},
        Vector2{config.ballSpeedX, config.ballSpeedY},
        config.ballRadius
    );
    newBall.SetAttached(true);
    balls.push_back(newBall);
}

void Game::InitPaddle() {
    paddle = Paddle(
        (SW - config.paddleWidth) / 2.0f,
        SH - config.paddleYOffset,
        config.paddleWidth,
        config.paddleHeight,
        config.paddleSpeed
    );
}

void Game::InitParticles() {
    particles.clear();
    particles.resize(MAX_PARTICLES);
    for (auto& p : particles) {
        p.active = false;
    }
}
