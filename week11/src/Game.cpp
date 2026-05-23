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
#include <numeric>  // 用于 std::count_if


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
    , clientFrameNumber(0)
    , lastConfirmedFrame(0)
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
            config.ballRadius = 10; config.ballSpeedX = 300; config.ballSpeedY = -400;
            config.gravity = 0.08; config.maxSpeed = 15; config.bounceForce = 0.5;
            config.paddleWidth = 100; config.paddleHeight = 20; config.paddleSpeed = 8;
            config.paddleYOffset = 80; config.paddleBoostSpeed = 28;
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
        config.paddleYOffset = paddleCfg.value("yOffset", 80);
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
        

        auto networkCfg = configJson.value("network", json::object());
        int networkPort = networkCfg.value("port", 12345);
        
        std::cout << "[Config] Network port configured: " << networkPort << std::endl;

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
    LoadConfig("config.json");    // 加载基础配置（包含 powerups 等）
    if (currentLevel >= 1) {
        std::string levelFile = "level_" + std::to_string(currentLevel) + ".json";
        LoadLevelConfig(levelFile);  // 只覆盖关卡特定配置（球速、砖块等）
    }
    LoadLeaderboard();
     bricks.reserve(200);      // 最大砖块数（10列×5行×4关=200）
    powerUps.reserve(50);     // 屏幕最多同时存在的道具
    balls.reserve(10);        // 多球道具最多10个球
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
    particlePool.Clear();

    lives = config.initialLives;

    networkManager.Initialize();
    networkUpdateTimer = 0.0f;
}
void Game::LoadBaseConfig(const std::string& path) {
    // 加载完整的 config.json（和原来的 LoadConfig 一样）
    LoadConfig(path);  // 重命名原来的 LoadConfig
}

// Game.cpp - 完善 LoadLevelConfig 函数

void Game::LoadLevelConfig(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            // 文件不存在：显示错误提示，使用默认配置
            std::string errorMsg = "Cannot open level file: " + path + "\nUsing default layout";
            ShowConfigError(errorMsg);
            std::cerr << "Warning: " << errorMsg << std::endl;
            
            // 使用默认砖块布局
            BuildBricksDefault();
            return;
        }

        json levelJson = json::parse(file);
        
        // 验证必需字段是否存在
        if (!levelJson.contains("bricks") || !levelJson["bricks"].contains("layout")) {
            std::string errorMsg = "Level file missing required fields (bricks/layout)\nUsing default layout";
            ShowConfigError(errorMsg);
            std::cerr << "Warning: " << errorMsg << std::endl;
            BuildBricksDefault();
            return;
        }
        
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
        
        // 正常加载砖块布局
        bool layoutLoaded = false;
        
        // 检查是否有 bricks 配置和 layout 字段
        if (levelJson.contains("bricks") && levelJson["bricks"].contains("layout")) {
            auto bricksCfg2 = levelJson["bricks"];
            auto layout = bricksCfg2["layout"];
            
            // 验证 layout 是数组且非空
            if (layout.is_array() && layout.size() > 0) {
                BuildBricksFromLayout(levelJson);  // 使用 JSON 布局
                layoutLoaded = true;
            }
        }
        
        if (!layoutLoaded) {
            // 降级到默认布局
            ShowConfigError("No valid layout found in " + path + "\nUsing default brick layout");
            BuildBricksDefault();
        }
        
        TraceLog(LOG_INFO, "Level config loaded: %s", path.c_str());
        
    } catch (const json::parse_error& e) {
        // JSON 解析错误
        std::string errorMsg = "JSON parse error in " + path + ":\n" + e.what() + "\nUsing default layout";
        ShowConfigError(errorMsg);
        std::cerr << "Error: " << errorMsg << std::endl;
        BuildBricksDefault();
        
    } catch (const std::exception& e) {
        // 其他异常
        std::string errorMsg = "Unexpected error loading " + path + ":\n" + e.what() + "\nUsing default layout";
        ShowConfigError(errorMsg);
        std::cerr << "Error: " << errorMsg << std::endl;
        BuildBricksDefault();
    }
}

// 新增：显示错误提示的方法
void Game::ShowConfigError(const std::string& message) {
    showConfigError = true;
    configErrorMessage = message;
    configErrorTimer = 5.0f;  // 显示5秒
}

// 新增：从 JSON 布局构建砖块（提取原 BuildBricks 中的 JSON 逻辑）
void Game::BuildBricksFromLayout(const json& levelJson) {
    bricks.clear();
    
    auto bricksCfg = levelJson["bricks"];
    int rows = bricksCfg.value("rows", config.brickRows);
    int cols = bricksCfg.value("cols", config.brickCols);
    float width = bricksCfg.value("width", config.brickWidth);
    float height = bricksCfg.value("height", config.brickHeight);
    float startX = bricksCfg.value("startX", config.brickStartX);
    float startY = bricksCfg.value("startY", config.brickStartY);
    float paddingX = bricksCfg.value("paddingX", config.brickPaddingX);
    float paddingY = bricksCfg.value("paddingY", config.brickPaddingY);
    
    auto layout = bricksCfg["layout"];
    json healthMap;
    if (bricksCfg.contains("health_map")) {
        healthMap = bricksCfg["health_map"];
    }
    
    // 关卡颜色主题
    Color levelColors[] = {
        (Color){200, 50, 50, 255},   // 红色系
        (Color){200, 130, 50, 255},  // 橙色系
        (Color){50, 150, 50, 255},   // 绿色系
        (Color){50, 100, 200, 255},  // 蓝色系
        (Color){180, 50, 180, 255}   // 紫色系
    };
    
    for (int r = 0; r < rows && r < (int)layout.size(); r++) {
        for (int c = 0; c < cols && c < (int)layout[r].size(); c++) {
            int type = layout[r][c];
            
            if (type == 0) continue;
            
            float x = startX + c * (width + paddingX);
            float y = startY + r * (height + paddingY);
            
            int hp = type;
            if (healthMap.contains(std::to_string(type))) {
                hp = healthMap[std::to_string(type)];
            }
            
            if (hp >= 999 || type == 999) {
                bricks.emplace_back(x, y, width, height, 
                                  GOLD, 999, 999, currentLevel);
                continue;
            }
            
            Color brickColor;
            int brickPoints = hp * 10;
            
            if (hp >= 4) {
                brickColor = levelColors[0];
            } else if (hp == 3) {
                brickColor = levelColors[1];
            } else if (hp == 2) {
                brickColor = levelColors[2];
            } else {
                brickColor = levelColors[3];
            }
            
            bricks.emplace_back(x, y, width, height, 
                              brickColor, brickPoints, hp, currentLevel);
        }
    }
    
    TraceLog(LOG_INFO, "Built %zu bricks from JSON layout (Level %d)", 
             bricks.size(), currentLevel);
}

// ========== 构建砖块 ==========
void Game::BuildBricks() {
    bricks.clear();
    
    // 加载关卡配置文件
    std::string levelFile = "level_" + std::to_string(currentLevel) + ".json";
    json levelJson;
    
    try {
        std::ifstream file(levelFile);
        if (!file.is_open()) {
            TraceLog(LOG_WARNING, "Cannot open level file: %s, using default layout", levelFile.c_str());
            BuildBricksDefault();  // 降级到原来的硬编码逻辑
            return;
        }
        levelJson = json::parse(file);
    } catch (const json::parse_error& e) {
        TraceLog(LOG_ERROR, "JSON parse error: %s, using default layout", e.what());
        BuildBricksDefault();
        return;
    }
    
    // 检查是否有 bricks 配置和 layout 字段
    if (!levelJson.contains("bricks") || !levelJson["bricks"].contains("layout")) {
        TraceLog(LOG_WARNING, "No layout field in level file, using default layout");
        BuildBricksDefault();
        return;
    }
    
    // 读取砖块配置
    auto bricksCfg = levelJson["bricks"];
    int rows = bricksCfg.value("rows", config.brickRows);
    int cols = bricksCfg.value("cols", config.brickCols);
    float width = bricksCfg.value("width", config.brickWidth);
    float height = bricksCfg.value("height", config.brickHeight);
    float startX = bricksCfg.value("startX", config.brickStartX);
    float startY = bricksCfg.value("startY", config.brickStartY);
    float paddingX = bricksCfg.value("paddingX", config.brickPaddingX);
    float paddingY = bricksCfg.value("paddingY", config.brickPaddingY);
    
    // 读取布局
    auto layout = bricksCfg["layout"];
    
    // 读取血量映射（可选，如果没有则默认 type = health）
    json healthMap;
    if (bricksCfg.contains("health_map")) {
        healthMap = bricksCfg["health_map"];
    }
    
    // 关卡颜色主题
    Color levelColors[] = {
        (Color){200, 50, 50, 255},   // 红色系
        (Color){200, 130, 50, 255},  // 橙色系
        (Color){50, 150, 50, 255},   // 绿色系
        (Color){50, 100, 200, 255},  // 蓝色系
        (Color){180, 50, 180, 255}   // 紫色系
    };
    
    for (int r = 0; r < rows && r < (int)layout.size(); r++) {
        for (int c = 0; c < cols && c < (int)layout[r].size(); c++) {
            int type = layout[r][c];
            
            // type == 0 表示没有砖块
            if (type == 0) continue;
            
            // 计算位置
            float x = startX + c * (width + paddingX);
            float y = startY + r * (height + paddingY);
            
            // 根据 type 获取血量
            int hp = type;
            if (healthMap.contains(std::to_string(type))) {
                hp = healthMap[std::to_string(type)];
            }
            
            // 无敌砖块（血量 >= 999 或 type == 999）
            if (hp >= 999 || type == 999) {
                bricks.emplace_back(x, y, width, height, 
                                  GOLD, 999, 999, currentLevel);
                continue;
            }
            
            // 普通砖块：根据血量和行数决定颜色
            Color brickColor;
            int brickPoints = hp * 10;
            
            // 根据血量设置颜色深浅
            if (hp >= 4) {
                brickColor = levelColors[0];
            } else if (hp == 3) {
                brickColor = levelColors[1];
            } else if (hp == 2) {
                brickColor = levelColors[2];
            } else {
                brickColor = levelColors[3];
            }
            
            bricks.emplace_back(x, y, width, height, 
                              brickColor, brickPoints, hp, currentLevel);
        }
    }
    
    TraceLog(LOG_INFO, "Built %zu bricks from JSON layout (Level %d)", 
             bricks.size(), currentLevel);
}

// 原来的硬编码逻辑作为降级方案
void Game::BuildBricksDefault() {
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
    
    powerUps.reserve(50);
    balls.reserve(10);

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
    particlePool.Clear();
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
    for (int i = 0; i < 12; i++) {
        Vector2 pos = {
            rect.x + GetRandomValue(0, (int)rect.width),
            rect.y + GetRandomValue(0, (int)rect.height)
        };
        Vector2 vel = {
            (GetRandomValue(-50, 50) / 10.0f),
            (GetRandomValue(-50, 50) / 10.0f)
        };
        particlePool.Spawn(pos, vel, color);
    }
}

void Game::SpawnPowerUpGlow(Vector2 pos, Color color) {
    for (int i = 0; i < 8; i++) {
        Vector2 vel = {
            (GetRandomValue(-40, 40) / 10.0f),
            (GetRandomValue(-40, 40) / 10.0f)
        };
        particlePool.Spawn(pos, vel, color);
    }
}

void Game::UpdateParticles(float dt) {
    particlePool.Update(dt);
}

void Game::DrawParticles() {
    particlePool.Draw();
}

// ========== 主更新循环 ==========
void Game::Update() {
    double frameStart = 0;
    if (!benchmarkMode) {
        frameStart = GetTime();
    }
    
    // 全局快捷键：查看排行榜
    if (state != GameState::LEADERBOARD && IsKeyPressed(KEY_L)) {
        LoadLeaderboard();
        ChangeState(GameState::LEADERBOARD);
        return;
    }
    
    // ========== 基准测试模式：使用固定时间步长 ==========
    float dt;
    if (benchmarkMode) {
        dt = 1.0f / 60.0f;  // 固定 16.67ms
    } else {
        dt = GetFrameTime();
    }
    
    CheckAsyncLoading();

    // 更新减速效果计时器
    if (slowEffectRemainingTime > 0.0f) {
        slowEffectRemainingTime -= dt;
        if (slowEffectRemainingTime <= 0.0f) {
            for (auto& ball : balls) {
                ball.SetSpeed(Vector2{config.ballSpeedX, config.ballSpeedY});
            }
        }
    }
    
    double collisionStart = 0;
    if (!benchmarkMode) collisionStart = GetTime();

    switch (state) {
        case GameState::MENU:
            if (IsKeyPressed(KEY_E)) {
                    editorMode = true;
                    state = GameState::PLAYING;  // 进入游戏场景但处于编辑模式
                    // 初始化空场景
                    bricks.clear();
                    editorCellWidth = (float)SW / editorGridCols;
                    editorCellHeight = (float)SH / editorGridRows;
                }
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
    
    double collisionTime = 0;
    if (!benchmarkMode) {
        collisionTime = GetTime() - collisionStart;
    }
    
    networkManager.Service();
    
    double physicsStart = 0;
    if (!benchmarkMode) physicsStart = GetTime();

    if (state == GameState::PLAYING) {
        UpdateNetwork();
    }

    double physicsTime = 0;
    double frameTime = 0;
    
    if (!benchmarkMode) {
        physicsTime = GetTime() - physicsStart;
        frameTime = GetTime() - frameStart;
    }
    
    // 基准测试模式下不输出日志
    if (!benchmarkMode) {
        static int frameCounter = 0;
        if (++frameCounter >= 60) {
            TraceLog(LOG_INFO, "Frame: %.2fms | Collision: %.3fms | Physics: %.3fms",
                     frameTime * 1000, collisionTime * 1000, physicsTime * 1000);
            frameCounter = 0;
        }
        
        if (frameTime > 0.0166) {
            TraceLog(LOG_WARNING, "Frame budget exceeded! %.2fms", frameTime * 1000);
        }

        static int budgetWarningCounter = 0;
        if (frameTime > 0.0167) {
            if (++budgetWarningCounter % 60 == 0) {
                TraceLog(LOG_WARNING, "Performance budget exceeded: %.2fms", frameTime * 1000);
            }
        }
    }
}
// ========== 状态处理函数 ==========
void Game::HandleMenuState() {
     // 检测存档，显示询问弹窗
    if (SaveExists() && !showLoadPrompt) {
        showLoadPrompt = true;
        return;  // 等待用户选择
    }
    
    // 如果正在显示弹窗，不处理其他按键
    if (showLoadPrompt) {
        // 按 Y 键继续游戏
        if (IsKeyPressed(KEY_Y)) {
            showLoadPrompt = false;
            ContinueFromSave();
            ChangeState(GameState::PLAYING);
            return;
        }
        // 按 N 键新游戏
        if (IsKeyPressed(KEY_N)) {
            showLoadPrompt = false;
            ResetToNewGame();
            ChangeState(GameState::LEVEL_SELECT);
            return;
        }
        // 按 ESC 关闭弹窗，留在菜单
        if (IsKeyPressed(KEY_ESCAPE)) {
            showLoadPrompt = false;
        }
        return;
    }
    // 新游戏
    if (IsKeyPressed(KEY_SPACE)) {
        ResetToNewGame();
        ChangeState(GameState::LEVEL_SELECT);
        return;
    }
    // 继续游戏（仅当存档存在时）
    if (IsKeyPressed(KEY_A) && SaveExists()) {
        ContinueFromSave();
        return;
    }
    
    // 主机模式
    if (IsKeyPressed(KEY_H)) {
        StartAsHost();
        ChangeState(GameState::PLAYING);
        return;
    }
    
    // 客户端模式
    if (IsKeyPressed(KEY_J)) {  // 改为 J 避免与 C 冲突
        StartAsClient("127.0.0.1");
        ChangeState(GameState::PLAYING);
        return;
    }
}

void Game::HandleLevelSelectState() {
    if (isLoadingExclusive || loadState != LoadState::IDLE) return;

    if (IsKeyPressed(KEY_ONE)) StartLevel(1);
    if (IsKeyPressed(KEY_TWO)) StartLevel(2);
    if (IsKeyPressed(KEY_THREE)) StartLevel(3);
    
    if (IsKeyPressed(KEY_R)) {
        ChangeState(GameState::MENU);
    }
}

void Game::HandlePlayingState(float dt) {
    HandleEditorMode();
    if (editorMode) {
        // 编辑模式下只更新UI，不更新游戏逻辑
        UpdatePowerUps(dt);
        UpdateParticles(dt);
        return;
    }
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
    
    // 加载中处理
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
    if (loadState == LoadState::DONE) {
        UpdatePowerUps(dt);
        UpdateParticles(dt);
    }

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
            UpdateClientCollisionPrediction(dt); 
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
    
     if (IsKeyPressed(KEY_S)) {
        SaveGame();
        TraceLog(LOG_INFO, "Game manually saved!");
    }

    if (IsKeyPressed(KEY_END)) {
        SaveGame(); 
        TraceLog(LOG_INFO, "Game saved before manual game over");
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
    if (++frameCounter % 60 == 0 || cellWidth == 0) {
        BuildSpatialGrid();  // 每60帧或首次重建网格
    }
    // 如果网格未构建或砖块数量变化，重建网格
    static int lastBrickCount = 0;
    int currentBrickCount = 0;
    for (auto& brick : bricks) {
        if (brick.IsActive()) currentBrickCount++;
    }
    
    if (lastBrickCount != currentBrickCount || cellWidth == 0) {
        BuildSpatialGrid();
        lastBrickCount = currentBrickCount;
    }
    
    for (auto& ball : balls) {
        if (ball.IsAttached()) continue;
        
        Vector2 pos = ball.GetPosition();
        int gx = (int)(pos.x / cellWidth);
        int gy = (int)(pos.y / cellHeight);
        
        // 限制网格范围
        gx = std::clamp(gx, 0, GRID_COLS - 1);
        gy = std::clamp(gy, 0, GRID_ROWS - 1);
        
        bool hitOccurred = false;
        
        // 检查周围 3x3 网格
        for (int dx = -1; dx <= 1 && !hitOccurred; dx++) {
            for (int dy = -1; dy <= 1 && !hitOccurred; dy++) {
                int nx = gx + dx;
                int ny = gy + dy;
                if (nx < 0 || nx >= GRID_COLS || ny < 0 || ny >= GRID_ROWS) continue;
                
                for (Brick* brickPtr : grid[nx][ny]) {
                    if (!brickPtr->IsActive() || brickPtr->HitThisFrame()) continue;
                    
                    Brick& brick = *brickPtr;
                    Rectangle rect = brick.GetRect();
                    auto side = ball.CheckBrickCollision(rect);
                    
                    if (side == CollisionSide::NONE) continue;
                    
                    // 火球模式
                    if (ball.IsFireball()) {
                        if (brick.GetHealth() >= 999) {
                            brick.SetHitThisFrame(true);
                            if (side == CollisionSide::LEFT || side == CollisionSide::RIGHT) {
                                ball.ReverseX();
                            } else {
                                ball.ReverseY();
                            }
                            continue;
                        }
                        brick.Destroy();
                        score += config.scorePerBrick;
                        SpawnParticles(rect, brick.GetColor());
                        TrySpawnPowerUp(rect);
                        brick.SetHitThisFrame(true);
                        // 火球继续穿透
                        continue;
                    }
                    
                    // 普通球模式
                    if (brick.GetHealth() >= 999) {
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
                        hitOccurred = true;
                        break;
                    }
                    
                    bool destroyed = brick.Hit();
                    score += config.scorePerBrick;
                    
                    TrySpawnPowerUp(rect);

                    if (destroyed) {
                        SpawnParticles(rect, brick.GetColor());
                    }
                    
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
                    hitOccurred = true;
                    break;
                }
            }
        }
    }
    
    // 重置帧标记
    for (auto& brick : bricks) {
        brick.SetHitThisFrame(false);
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
        // 通关当前关卡时自动保存
        SaveGame();
        std::cout << "[CheckLevelCompletion] Level " << currentLevel << " completed! Saved." << std::endl;
        
        if (currentLevel >= 3) {
            // 通关所有关卡
            isEnteringName = true;
            ChangeState(GameState::GAMEOVER);
            return;
        }
        
        // 启动异步加载下一关
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
        // 处理名字输入
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 126 && playerName.length() < 20)
                playerName += (char)key;
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !playerName.empty())
            playerName.pop_back();
        
        if (IsKeyPressed(KEY_ENTER)) {
            AddScoreToLeaderboard();
            isEnteringName = false;
            score = 0;
            lives = config.initialLives;
            currentLevel = 1;
            paddle.ResetWidth();
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

// Game.cpp - 修改 HandleWinState()
void Game::HandleWinState() {
    if (isEnteringName) {
        // 同样的名字输入处理代码
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 126 && playerName.length() < 20)
                playerName += (char)key;
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !playerName.empty())
            playerName.pop_back();
        
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
    if (IsKeyPressed(KEY_R) ) {
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

void Game::ReceiveAndSmoothHostState() {
    GameStateSnapshot state;
    if (!networkManager.ReceiveGameState(state)) return;
    
    // 更新主机确认的帧号（用于后续预测）
    // 注：当前简化版本不使用帧号回滚，只用于日志
    if (state.timestamp > lastConfirmedFrame) {
        lastConfirmedFrame = state.frameNumber;
    }
    
    // ========== 多球同步 ==========
    if (state.ballCount != (int)balls.size()) {
        // 球数量不一致，直接重新同步
        ResyncBalls(state);
    } else {
        // 同步每个球的位置和速度
        for (int i = 0; i < state.ballCount; i++) {
            Vector2 currentPos = balls[i].GetPosition();
            Vector2 targetPos = {state.ballX[i], state.ballY[i]};
            Vector2 targetSpeed = {state.ballSpeedX[i], state.ballSpeedY[i]};
            
            // 计算预测误差
            float errorX = targetPos.x - currentPos.x;
            float errorY = targetPos.y - currentPos.y;
            float errorMagnitude = sqrtf(errorX * errorX + errorY * errorY);
            
            // ========== 简化修正 ==========
            if (errorMagnitude > 100.0f) {
                // 误差过大：硬修正（直接跳转到正确位置）
                balls[i].SetPosition(targetPos);
                balls[i].SetSpeed(targetSpeed);
                balls[i].SetAttached(state.isAttached[i]);

                if (debugMode) {
                    TraceLog(LOG_WARNING, "Hard correction applied to ball %d, error: %.1f", 
                             i, errorMagnitude);
                }
            } else if (errorMagnitude > 5.0f) {
                // 中等误差：平滑修正（插值）
                float lerpFactor = 0.3f;
                Vector2 smoothedPos = {
                    currentPos.x + errorX * lerpFactor,
                    currentPos.y + errorY * lerpFactor
                };
                balls[i].SetPosition(smoothedPos);
                balls[i].SetSpeed(targetSpeed);
                balls[i].SetAttached(state.isAttached[i]);
            } else {
                // 小误差：只微调速度
                balls[i].SetSpeed(targetSpeed);
                balls[i].SetAttached(state.isAttached[i]);
            }
            
            // 同步火球状态
            if (state.isFireball[i] && !balls[i].IsFireball()) {
                balls[i].SetFire(config.powerups.fire_ball.duration);
            } else if (!state.isFireball[i] && balls[i].IsFireball()) {
                balls[i].ClearFire();
            }
        }
    }
    
    // ========== 同步其他状态 ==========
    // 客户端控制底部挡板（paddle），主机控制顶部挡板（paddle2）
    paddle2.SetPosition(state.paddle1X, paddle2.GetPosition().y);
    score = state.score;
    lives = state.lives;
    gameStarted = state.gameStarted;
    
    // 关卡切换
    if (state.currentLevel != currentLevel) {
        currentLevel = state.currentLevel;
        std::string levelFile = "level_" + std::to_string(currentLevel) + ".json";
        LoadLevelConfig(levelFile);
        BuildBricks();
    }
    
    // 可选：同步砖块状态（简化版不做，因为误差可接受）
}

void Game::SendGameStateToPeer() {
    GameStateSnapshot state;
    memset(&state, 0, sizeof(state));  // 清零所有字段
    
    // 基本信息
    state.paddle1X = paddle.GetPosition().x;
    state.paddle1Y = paddle.GetPosition().y;
    state.paddle2X = paddle2.GetPosition().x;
    state.paddle2Y = paddle2.GetPosition().y;
    state.score = score;
    state.lives = lives;
    state.currentLevel = currentLevel;
    state.timestamp = GetTime();
    state.gameStarted = gameStarted;
    
    // 统计活跃砖块数量
    int activeCount = 0;
    for (auto& brick : bricks) {
        if (brick.IsActive()) activeCount++;
    }
    state.activeBrickCount = activeCount;
    
    // ========== 多球信息 ==========
    state.ballCount = std::min((int)balls.size(), 10);
    for (int i = 0; i < state.ballCount; i++) {
        state.ballX[i] = balls[i].GetPosition().x;
        state.ballY[i] = balls[i].GetPosition().y;
        state.ballSpeedX[i] = balls[i].GetSpeed().x;
        state.ballSpeedY[i] = balls[i].GetSpeed().y;
        state.isFireball[i] = balls[i].IsFireball();
        state.isAttached[i] = balls[i].IsAttached();
    }
    
    networkManager.SendGameState(state);
}

void Game::StartAsHost() {
    if (networkManager.StartHost()) {
        TraceLog(LOG_INFO, "Game started as HOST on port %d", NetworkManager::DEFAULT_PORT);
        
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
        
        bricks.reserve(200);
        powerUps.reserve(50);
        balls.reserve(10);

        // 重新构建砖块
        BuildBricks();
        
        // 清除道具和粒子
        powerUps.clear();
       
        particlePool.Clear();
        
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
        TraceLog(LOG_INFO, "Game started as CLIENT - Connected to host at %s:%d", 
                 ip.c_str(), NetworkManager::DEFAULT_PORT);
        
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
        
         bricks.reserve(200);
        predictedBricks.reserve(200);
        powerUps.reserve(50);
        balls.reserve(10);
        // 构建砖块（与主机相同）
        BuildBricks();

         // ✅ 创建预测砖块副本
        predictedBricks = bricks;
        usePredictedBricks = true;
        
        // 清除道具和粒子
        powerUps.clear();
         particlePool.Clear();
        
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
    DrawText("PRESS SPACE - NEW GAME", SW/2 - 130, 230, 24, RAYWHITE);
    
    // 如果有存档，显示继续选项
    if (SaveExists()) {
        DrawText("PRESS A - CONTINUE FROM SAVE", SW/2 - 150, 270, 24, GREEN);
    }
    
    DrawText("PRESS H - HOST CO-OP MODE", SW/2 - 130, 310, 24, YELLOW);
    DrawText("PRESS C - JOIN CO-OP MODE", SW/2 - 130, 350, 24, ORANGE);
    
    DrawText("PRESS E - LEVEL EDITOR", SW/2 - 130, 390, 24, MAGENTA);
    DrawText("L: LEADERBOARD | P: PAUSE", SW/2 - 130, 430, 20, LIGHTGRAY);
}

void Game::DrawPlaying() {
    DrawRectangle(0, 0, SW, 5, DARKGRAY);
    DrawRectangle(0, 0, 5, SH, DARKGRAY);
    DrawRectangle(SW-5, 0, 5, SH, DARKGRAY);

     // 批绘制砖块
    batchRenderer.Begin();
    Brick::SetBatchRenderer(&batchRenderer);
    if (usePredictedBricks && localRole == NetworkRole::CLIENT) {
        for (auto& b : predictedBricks) {
            if (b.IsActive()) b.DrawBatch();
        }
    } else {
        for (auto& b : bricks) {
            if (b.IsActive()) b.DrawBatch();
        }
    }
    batchRenderer.End();
    Brick::SetBatchRenderer(nullptr);
    
    // 绘制两个挡板
    paddle.Draw();
    if (isNetworkGame) {
        // 给客户端的挡板不同颜色以区分
        DrawRectangleRec(paddle2.GetRect(), RED);
        DrawRectangleLinesEx(paddle2.GetRect(), 2, DARKBLUE);
    }
    
    for (auto& ball : balls) {
        ball.Draw();
    }

    // 绘制砖块
     if (usePredictedBricks && localRole == NetworkRole::CLIENT) {
        for (auto& b : predictedBricks) {
            if (b.IsActive()) b.Draw();
        }
    } else {
        for (auto& b : bricks) {
            if (b.IsActive()) b.Draw();
        }
    }
    
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
         // 客户端显示预测提示
        if (usePredictedBricks) {
            DrawText("PREDICTION MODE", SW - 150, SH - 30, 12, YELLOW);
        }
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
    // ========== ✅ 添加错误提示绘制 ==========
    if (showConfigError) {
        // 更新计时器
        configErrorTimer -= GetFrameTime();
        if (configErrorTimer <= 0.0f) {
            showConfigError = false;
        }
        
        // 计算提示框大小
        int textWidth = MeasureText(configErrorMessage.c_str(), 16);
        int boxWidth = textWidth + 40;
        int boxHeight = 70;
        int boxX = (SW - boxWidth) / 2;
        int boxY = SH - 90;
        
        // 半透明背景
        DrawRectangle(boxX, boxY, boxWidth, boxHeight, ColorAlpha(BLACK, 0.85f));
        DrawRectangleLines(boxX, boxY, boxWidth, boxHeight, RED);
        
        // 分行显示错误信息
        std::string msg = configErrorMessage;
        size_t pos = msg.find('\n');
        std::string line1 = msg.substr(0, pos);
        std::string line2 = (pos != std::string::npos) ? msg.substr(pos + 1) : "";
        
        DrawText(line1.c_str(), boxX + 20, boxY + 20, 16, YELLOW);
        if (!line2.empty()) {
            DrawText(line2.c_str(), boxX + 20, boxY + 45, 14, LIGHTGRAY);
        }
    }

    DrawEditorUI();
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
    DrawText("PRESS R TO RETURN", SW/2 - 140, SH - 50, 20, GRAY);
}

// Game.cpp - 简化 DrawNameInput()
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
    // 删除原来的 GetCharPressed 和 KEY_BACKSPACE 处理代码
}

void Game::DrawDebugInfo() {
    int lineY = SH - 80;  // 从底部开始
    int lineHeight = 18;
    
    // 半透明背景条，让文字更清晰
    DrawRectangle(0, lineY - 5, SW, 85, ColorAlpha(BLACK, 0.6f));
    
    DrawText(TextFormat("PUs:%d  Parts:%d  Cache:%zu  Queue:%zu  DrawCalls:%d  Batches:%d", 
        (int)powerUps.size(), 
        particlePool.GetActiveCount(), 
        TextureCache::Instance().Size(), 
        loadResultQueue.Size(),
        0,  // currentDrawCalls 需要实际获取
        batchRenderer.GetBatchCount()), 
        10, lineY, 14, YELLOW);
    
    lineY += lineHeight;
    DrawText(TextFormat("Particles: %d/%d  |  T: Async Load Test", 
        particlePool.GetActiveCount(), 
        ParticlePool::MAX_PARTICLES), 
        10, lineY, 14, LIGHTGRAY);
    
    // 帧率放右下角
    static float avgFrameTime = 0.0f;
    avgFrameTime = avgFrameTime * 0.95f + GetFrameTime() * 0.05f;
    
    Color timeColor = (avgFrameTime < 0.0167f) ? GREEN : 
                      (avgFrameTime < 0.0333f) ? YELLOW : RED;
    
    DrawText(TextFormat("Frame: %.2fms (%.1f FPS)", 
        avgFrameTime * 1000, 1.0f / avgFrameTime), 
        SW - 200, lineY, 14, timeColor);
}
// ========== 新增：异步加载实现 ==========

// 1. 启动异步关卡加载
void Game::StartAsyncLevelLoad(int levelNumber) {
    if (loadState == LoadState::LOADING) {
        std::cout << "Already loading, please wait..." << std::endl;
        return;
    }
    
    // 检查是否刚完成加载（防抖，避免重复触发）
    float currentTime = GetTime();
    if (currentTime - loadStartTime < 0.5f && loadStartTime > 0) {
        std::cout << "[Game] Too soon after last load, ignoring" << std::endl;
        return;
    }

    std::cout << "Starting async load for level " << levelNumber << std::endl;
    
    loadState = LoadState::LOADING;
    isLoadingExclusive = true;      // 禁止输入处理
    loadStartTime = currentTime;
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
    // 处理队列中的加载结果
    LoadResult result;
    while (loadResultQueue.TryPop(result)) {
        std::cout << "[Main Thread] Queue message: Level " 
                  << result.levelNumber << " - " 
                  << (result.success ? "SUCCESS" : "FAILED")
                  << " - " << result.message << std::endl;
        
        // ✅ 如果加载失败，直接清理状态
        if (!result.success) {
            loadState = LoadState::IDLE;
            isLoadingExclusive = false;
            std::cerr << "[Game] Level load failed: " << result.message << std::endl;
            return;
        }
    }

    if (loadState != LoadState::LOADING) return;
    
    if (!loadFuture.valid()) {
        std::cerr << "[Error] Invalid future!" << std::endl;
        loadState = LoadState::IDLE;
        isLoadingExclusive = false;
        return;
    }
    
    // 非阻塞检查
    auto status = loadFuture.wait_for(std::chrono::seconds(0));
    
    if (status == std::future_status::ready) {
        std::cout << "[Main Thread] Async loading completed!" << std::endl;
        
        try {
            bool result = loadFuture.get();
            
            if (result) {
                std::cout << "[Main Thread] Loading successful, applying level..." << std::endl;
                
                // ✅ 在应用新关卡前，保存当前道具效果（如果需要延续）
                // 例如：挡板加长效果、火球效果等
                
                ApplyLoadedLevel();  // 应用新关卡
                
                // ✅ 加载完成后，延迟一点再允许输入（避免误操作）
                loadStartTime = GetTime();  // 用于延迟恢复
                loadState = LoadState::DONE;  // 进入完成状态，等待延迟
                // 注意：不立即设为 IDLE，等待一小段时间
            } else {
                std::cerr << "[Main Thread] Loading failed!" << std::endl;
                loadState = LoadState::IDLE;
                isLoadingExclusive = false;
            }
        } catch (const std::exception& e) {
            std::cerr << "[Main Thread] Exception: " << e.what() << std::endl;
            loadState = LoadState::IDLE;
            isLoadingExclusive = false;
        }
    }
    
    // ✅ 处理加载完成后的延迟恢复
    if (loadState == LoadState::DONE) {
        float currentTime = GetTime();
        if (currentTime - loadStartTime > 0.3f) {  // 300ms 延迟
            loadState = LoadState::IDLE;
            isLoadingExclusive = false;
            std::cout << "[Game] Load complete, input enabled" << std::endl;
        }
    }
}

// 4. 应用加载完成的关卡数据
void Game::ApplyLoadedLevel() {
    std::cout << "[Main Thread] Applying level " << pendingLevelNumber << std::endl;
    
    // ✅ 禁止在应用过程中处理任何输入
    // isLoadingExclusive 已经是 true
    
    // 切换到新关卡
    currentLevel = pendingLevelNumber;
    
    // ✅ 保存挡板宽度（道具效果可能会延续）
    float currentPaddleWidth = paddle.GetWidth();
    float currentPaddle2Width = paddle2.GetWidth();
    
    // 加载关卡配置
    std::string levelPath = "level_" + std::to_string(currentLevel) + ".json";
    LoadLevelConfig(levelPath);
    
    // 重建砖块
    BuildBricks();

     powerUps.reserve(50);
    balls.reserve(10);
    // ✅ 同步预测砖块（如果是客户端模式）
    if (usePredictedBricks) {
        predictedBricks = bricks;
    }
    
    // 恢复或重置挡板宽度
    if (currentPaddleWidth > config.paddleWidth) {
        paddle.SetWidth(currentPaddleWidth);
    } else {
        paddle.SetWidth(config.paddleWidth);
    }
    
    if (isNetworkGame && currentPaddle2Width > config.paddleWidth) {
        paddle2.SetWidth(currentPaddle2Width);
    } else if (isNetworkGame) {
        paddle2.SetWidth(config.paddleWidth);
    }
    
    // ✅ 清理所有道具（新关卡重新开始）
    powerUps.clear();
    
    // ✅ 保存旧的球状态（用于可能的多球延续）
    bool hadFireball = false;
    float fireRemaining = 0.0f;
    if (!balls.empty() && balls[0].IsFireball()) {
        hadFireball = true;
        fireRemaining = balls[0].GetFireTimer();  // 需要添加 GetFireTimer 方法
    }
    
    // 重置球的位置和状态
    balls.clear();
    float paddleCenterX = paddle.GetPosition().x + paddle.GetWidth() / 2.0f;
    float ballY = paddle.GetPosition().y - config.ballRadius;
    
    balls.emplace_back(
        Vector2{paddleCenterX, ballY},  // ✅ 挡板正上方
        Vector2{config.ballSpeedX, config.ballSpeedY},
        config.ballRadius
    );
    balls[0].SetAttached(true);
    
    // ✅ 恢复火球效果（如果还有剩余时间）
    if (hadFireball && fireRemaining > 0) {
        balls[0].SetFire(fireRemaining);
        std::cout << "[Game] Restored fireball effect with " << fireRemaining << "s remaining" << std::endl;
    }
    
    // 重置游戏状态
    gameStarted = false;
    slowEffectRemainingTime = 0.0f;
    
    particlePool.Clear();
    // ✅ 重置挡板位置
    paddle.SetPosition((SW - paddle.GetWidth()) / 2.0f, SH - config.paddleYOffset);
    if (isNetworkGame) {
        paddle2.SetPosition((SW - paddle2.GetWidth()) / 2.0f, config.paddleYOffset);
    }
    
    // ✅ 刷新背景纹理
    backgroundPath = "assets/background_level" + std::to_string(currentLevel) + ".png";
    if (TextureCache::Instance().HasTexture(backgroundPath)) {
        currentBackground = TextureCache::Instance().GetTexture(backgroundPath);
    } else {
        // 异步加载新背景
        auto future = TextureCache::Instance().LoadTextureAsync(backgroundPath);
        // 可以选择等待或延迟加载
    }
    
    std::cout << "[Main Thread] Level " << currentLevel << " applied! Press SPACE to launch ball." << std::endl;
}

void Game::Shutdown() {
    // 等待异步加载完成
    if (loadState == LoadState::LOADING && loadFuture.valid()) {
        std::cout << "[Game] Waiting for async load to complete..." << std::endl;
        auto status = loadFuture.wait_for(std::chrono::seconds(2));
        if (status == std::future_status::timeout) {
            std::cerr << "[Game] Timeout waiting for async load!" << std::endl;
        }
    }
    
    // 保存排行榜
    SaveLeaderboard();
    
    // 清理线程安全队列
    std::cout << "[ThreadSafeQueue] Clearing queue (" 
              << loadResultQueue.Size() << " items remaining)" << std::endl;
    loadResultQueue.Clear();
    loadResultQueue.Finish();
    
    // 清理纹理缓存
    std::cout << "[TextureCache] Clearing " 
              << TextureCache::Instance().Size() << " textures..." << std::endl;
    TextureCache::Instance().Clear();
    
    // 断开网络连接
    networkManager.Shutdown();
    
    std::cout << "[Game] Shutdown complete." << std::endl;
}


void Game::StartLevel(int level) {
    currentLevel = level;
    score = 0;
    lives = config.initialLives;
    
    // 1. 清理
    balls.clear();
    powerUps.clear();
    particlePool.Clear();
    
    // 2. 重新加载关卡配置（确保使用新关卡的参数）
    std::string levelFile = "level_" + std::to_string(currentLevel) + ".json";
    LoadLevelConfig(levelFile);
    
    // 3. 重建砖块
    BuildBricks();
    
    powerUps.reserve(50);
    balls.reserve(10);

    // 4. 重置挡板（使用新关卡的宽度和Y偏移）
    paddle = Paddle(
        (SW - config.paddleWidth) / 2.0f,
        SH - config.paddleYOffset,
        config.paddleWidth,
        config.paddleHeight,
        config.paddleSpeed
    );
    
    // 5. 正确创建球 - 位置跟随挡板
    float ballX = paddle.GetPosition().x + paddle.GetWidth() / 2.0f;
    float ballY = paddle.GetPosition().y - config.ballRadius;
    
    balls.emplace_back(
        Vector2{ballX, ballY},
        Vector2{config.ballSpeedX, config.ballSpeedY},
        config.ballRadius
    );
    balls[0].SetAttached(true);
    
    // 6. 重置游戏状态
    gameStarted = false;
    slowEffectRemainingTime = 0.0f;
    paddle.ResetWidth();  // 清除残留的道具效果
    
    // 7. 切换状态
    state = GameState::PLAYING;
}

void Game::DrawLevelSelect() {
    DrawRectangle(0, 0, SW, SH, { 20, 20, 20, 200 }); // 半透明背景
    DrawText("SELECT YOUR CHALLENGE", SW/2 - 180, 150, 30, RAYWHITE);
    
    DrawText("[ 1 ] - LEVEL 1 (Beginner)", SW/2 - 120, 250, 20, GREEN);
    DrawText("[ 2 ] - LEVEL 2 (Intermediate)", SW/2 - 120, 300, 20, ORANGE);
    DrawText("[ 3 ] - LEVEL 3 (Expert)", SW/2 - 120, 350, 20, RED);
    
    DrawText("Press R to return", SW/2 - 80, 450, 15, GRAY);
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
     particlePool.Clear();
}
void Game::UpdateClientCollisionPrediction(float dt) {
     (void)dt;
    if (!usePredictedBricks) return;
    
    // 使用预测砖块副本进行碰撞检测
    for (auto& ball : balls) {
        if (ball.IsAttached()) continue;
        
        bool hitOccurred = false;
        
        for (auto& brick : predictedBricks) {
            if (!brick.IsActive() || brick.HitThisFrame()) continue;
            
            Rectangle rect = brick.GetRect();
            auto side = ball.CheckBrickCollision(rect);
            
            if (side == CollisionSide::NONE) continue;
            
            // 火球模式：穿透并标记摧毁
            if (ball.IsFireball()) {
                if (brick.GetHealth() < 999) {  // 非无敌砖块
                    brick.Destroy();
                    // 预测粒子效果
                    SpawnParticles(rect, brick.GetColor());
                }
                brick.SetHitThisFrame(true);
                // 火球不反弹，继续穿透
                continue;
            }
            
            // 普通球模式：单次碰撞后反弹
            if (brick.GetHealth() >= 999) {
                // 无敌砖块反弹
                if (side == CollisionSide::LEFT || side == CollisionSide::RIGHT) {
                    ball.ReverseX();
                } else {
                    ball.ReverseY();
                }
                brick.SetHitThisFrame(true);
                hitOccurred = true;
                break;
            }
            
            // 普通砖块：扣血或摧毁
            bool destroyed = brick.Hit();
            if (destroyed) {
                SpawnParticles(rect, brick.GetColor());
            }
            
            // 反弹球
            if (side == CollisionSide::LEFT || side == CollisionSide::RIGHT) {
                ball.ReverseX();
            } else {
                ball.ReverseY();
            }
            
            brick.SetHitThisFrame(true);
            hitOccurred = true;
            break;  // 普通球只处理一个碰撞
        }
        
        // 如果发生碰撞，需要重新调整球的位置避免卡进砖块
        if (hitOccurred) {
            // 简单的位置修正：确保球在砖块外部
            for (const auto& brick : predictedBricks) {
                if (!brick.IsActive()) continue;
                Rectangle rect = brick.GetRect();
                if (CheckCollisionCircleRec(ball.GetPosition(), ball.GetRadius(), rect)) {
                    // 将球推到砖块外（简单处理）
                    Vector2 pos = ball.GetPosition();
                    if (pos.x < rect.x) pos.x = rect.x - ball.GetRadius();
                    else if (pos.x > rect.x + rect.width) pos.x = rect.x + rect.width + ball.GetRadius();
                    if (pos.y < rect.y) pos.y = rect.y - ball.GetRadius();
                    else if (pos.y > rect.y + rect.height) pos.y = rect.y + rect.height + ball.GetRadius();
                    ball.SetPosition(pos);
                    break;
                }
            }
        }
    }
    
    // 重置帧标记
    for (auto& brick : predictedBricks) {
        brick.SetHitThisFrame(false);
    }
}

void Game::BuildSpatialGrid() {
    // 清空网格
    for (int i = 0; i < GRID_COLS; i++)
        for (int j = 0; j < GRID_ROWS; j++)
            grid[i][j].clear();
    
    cellWidth = SW / (float)GRID_COLS;
    cellHeight = SH / (float)GRID_ROWS;
    
    for (auto& brick : bricks) {
        if (!brick.IsActive()) continue;
        Rectangle rect = brick.GetRect();
        int gx = (int)(rect.x / cellWidth);
        int gy = (int)(rect.y / cellHeight);
        if (gx >= 0 && gx < GRID_COLS && gy >= 0 && gy < GRID_ROWS) {
            grid[gx][gy].push_back(&brick);
        }
    }
}


// ========== ParticlePool 实现 ==========
ParticlePool::ParticlePool() : freeCount(MAX_PARTICLES) {
    // 初始化空闲列表（倒序填充）
    for (int i = 0; i < MAX_PARTICLES; i++) {
        freeList[i] = MAX_PARTICLES - 1 - i;
    }
    
    // 初始化所有粒子为未激活状态
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].active = false;
        particles[i].life = 0.0f;
    }
}

void ParticlePool::Spawn(Vector2 pos, Vector2 vel, Color color) {
    if (freeCount > 0) {
        int idx = freeList[--freeCount];
        particles[idx] = {
            pos,        // position
            vel,        // velocity
            color,      // color
            1.0f,       // life
            1.0f,       // maxLife
            true        // active
        };
    }
}

void ParticlePool::Update(float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            // 更新位置
            particles[i].position.x += particles[i].velocity.x * dt;
            particles[i].position.y += particles[i].velocity.y * dt;
            
            // 更新生命
            particles[i].life -= dt;
            
            // 边界检查（可选）
            if (particles[i].position.x - 3 <= 0 || 
                particles[i].position.x + 3 >= GetScreenWidth()) {
                particles[i].velocity.x = -particles[i].velocity.x * 0.5f;
            }
            if (particles[i].position.y - 3 <= 0) {
                particles[i].velocity.y = -particles[i].velocity.y * 0.5f;
            }
            
            // 生命结束或超出底部边界
            if (particles[i].life <= 0 || particles[i].position.y + 3 >= GetScreenHeight()) {
                particles[i].active = false;
                freeList[freeCount++] = i;  // 回收索引
            }
        }
    }
}

void ParticlePool::Draw() const {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            unsigned char alpha = (unsigned char)(255 * (particles[i].life / particles[i].maxLife));
            DrawCircleV(particles[i].position, 3, Fade(particles[i].color, alpha / 255.0f));
        }
    }
}

void ParticlePool::Clear() {
    // 重置所有粒子
    freeCount = MAX_PARTICLES;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].active = false;
        freeList[i] = MAX_PARTICLES - 1 - i;
    }
}

int ParticlePool::GetActiveCount() const {
    return MAX_PARTICLES - freeCount;
}
void Game::UpdateBenchmark(float fixedDt) {
    // static int callCount = 0;
    // callCount++;
    
    // if (callCount <= 10) {
    //     std::cout << "[DEBUG] ===== UpdateBenchmark #" << callCount << " =====" << std::endl;
    //     std::cout << "[DEBUG] state=" << (int)state << " (0=MENU,1=LEVEL_SELECT,2=PLAYING)" << std::endl;
    //     std::cout << "[DEBUG] balls.size()=" << balls.size() << std::endl;
    //     std::cout << "[DEBUG] bricks.size()=" << bricks.size() << std::endl;
    //     std::cout << "[DEBUG] gameStarted=" << gameStarted << std::endl;
    //     std::cout << "[DEBUG] currentLevel=" << currentLevel << std::endl;
    // }
    
    // 更新减速效果
    if (slowEffectRemainingTime > 0.0f) {
        slowEffectRemainingTime -= fixedDt;
        if (slowEffectRemainingTime <= 0.0f) {
            for (auto& ball : balls) {
                ball.SetSpeed(Vector2{config.ballSpeedX, config.ballSpeedY});
            }
        }
    }
    
    // 物理更新
    if (state == GameState::PLAYING) {
        // 输入处理（简化：持续向右移动）
        HandleLocalInput(fixedDt);
        
        // 物理计算
        UpdateBallsPhysics(fixedDt);
        HandleBallBrickCollisions();
        
        // 道具更新
        UpdatePowerUps(fixedDt);
        HandlePowerUpCollisions();
        UpdateParticles(fixedDt);
        
        // 火球计时器
        for (auto& ball : balls) {
            ball.Update(fixedDt);
        }
        
        // 发射逻辑
        if (!gameStarted) {
            gameStarted = true;
            LaunchBall();
        }
        
        // 死亡检测
        CheckBallLoss();
        
        // 过关检测（简化：跳过异步加载）
        if (loadState != LoadState::LOADING) {
            int activeBricks = 0;
            for (auto& brick : bricks) {
                if (brick.IsActive() && brick.GetHealth() < 10) activeBricks++;
            }
            if (activeBricks == 0 && currentLevel < 3) {
                // 直接加载下一关，不使用异步
                currentLevel++;
                LoadLevelConfig("level_" + std::to_string(currentLevel) + ".json");
                BuildBricks();
                gameStarted = false;
            }
        }
    }
}void Game::RunBenchmark(int frames) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "      BENCHMARK MODE - Running " << frames << " frames" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // 保存原始设置
    bool originalDebugMode = debugMode;
    bool originalGodMode = godMode;
    bool originalBenchmarkMode = benchmarkMode;  // ✅ 保存
    
    // 设置基准测试模式
    debugMode = false;
    godMode = true;
    benchmarkMode = true;  // ✅ 设置成员变量
    
    // 确保游戏处于 Playing 状态
    state = GameState::PLAYING;
    gameStarted = true;
    
    // 固定时间步长 16.67ms
    const float FIXED_DT = 1.0f / 60.0f;
    
    std::vector<float> frameTimes;
    frameTimes.reserve(frames);
    
    // 预热运行 60 帧
    std::cout << "Warming up..." << std::endl;
    for (int i = 0; i < 60; i++) {
        UpdateBenchmark(FIXED_DT);
    }
    
    std::cout << "Running benchmark..." << std::endl;
    auto benchmarkStartTime = std::chrono::high_resolution_clock::now();
    
    for (int frame = 0; frame < frames; frame++) {
        auto frameStart = std::chrono::high_resolution_clock::now();
        
        UpdateBenchmark(FIXED_DT);
        
        auto frameEnd = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
        frameTimes.push_back(frameTime);
        
        if ((frame + 1) % 120 == 0) {
            std::cout << "Progress: " << (frame + 1) << "/" << frames 
                      << " frames (" << ((frame + 1) * 100 / frames) << "%)" << std::endl;
        }
        
        if (balls.empty()) {
            balls.emplace_back(Vector2{(float)SW/2, (float)SH/2},
                               Vector2{config.ballSpeedX, config.ballSpeedY},
                               config.ballRadius);
            balls.back().SetAttached(true);
            gameStarted = true;
        }
    }
    
    auto totalTime = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - benchmarkStartTime).count();
    
    // 统计分析
    float sumFrame = 0, minFrame = 1000.0f, maxFrame = 0;
    for (float t : frameTimes) {
        sumFrame += t;
        if (t < minFrame) minFrame = t;
        if (t > maxFrame) maxFrame = t;
    }
    float avgFrame = sumFrame / frames;
    
    std::vector<float> sortedFrameTimes = frameTimes;
    std::sort(sortedFrameTimes.begin(), sortedFrameTimes.end());
    int p95Index = (int)(frames * 0.95);
    int p99Index = (int)(frames * 0.99);
    float p95Frame = sortedFrameTimes[p95Index];
    float p99Frame = sortedFrameTimes[p99Index];
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "         BENCHMARK RESULTS" << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::cout << "\n--- Frame Timing ---" << std::endl;
    std::cout << "Total frames:     " << frames << std::endl;
    std::cout << "Total time:       " << totalTime << " seconds" << std::endl;
    std::cout << "Average FPS:      " << (1000.0f / avgFrame) << " fps" << std::endl;
    std::cout << "Average frame:    " << avgFrame << " ms" << std::endl;
    std::cout << "Min frame:        " << minFrame << " ms" << std::endl;
    std::cout << "Max frame:        " << maxFrame << " ms" << std::endl;
    std::cout << "P95 frame:        " << p95Frame << " ms" << std::endl;
    std::cout << "P99 frame:        " << p99Frame << " ms" << std::endl;
    
    float targetFrameTime = 1000.0f / 60.0f;
    if (avgFrame <= targetFrameTime) {
        std::cout << "\n✅ PASS: Average frame time within 60 FPS budget" << std::endl;
    } else {
        float exceed = (avgFrame - targetFrameTime) / targetFrameTime * 100;
        std::cout << "\n❌ FAIL: Average frame time exceeds budget by " << exceed << "%" << std::endl;
    }
    
    float frameVariance = 0;
    for (float t : frameTimes) {
        frameVariance += (t - avgFrame) * (t - avgFrame);
    }
    frameVariance /= frames;
    float frameStdDev = sqrt(frameVariance);
    std::cout << "Frame stability:  ±" << frameStdDev << " ms (std dev)" << std::endl;
    
    // 恢复原始设置
    debugMode = originalDebugMode;
    godMode = originalGodMode;
    benchmarkMode = originalBenchmarkMode;  // ✅ 恢复
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "      Benchmark completed!" << std::endl;
    std::cout << "========================================\n" << std::endl;
}

void Game::SendClientInputToHost() {
    if (!isNetworkGame || localRole != NetworkRole::CLIENT) return;
    
    PlayerInput input;
    input.paddleX = paddle.GetPosition().x;
    input.paddleY = paddle.GetPosition().y;
    input.leftPressed = IsKeyDown(KEY_LEFT);
    input.rightPressed = IsKeyDown(KEY_RIGHT);
    input.spacePressed = IsKeyPressed(KEY_SPACE);
    input.timestamp = GetTime();
    input.frameNumber = clientFrameNumber++;  // 每帧递增
    
    networkManager.SendPlayerInput(input);
}

void Game::ResyncBalls(const GameStateSnapshot& state) {
    if (debugMode) {
        TraceLog(LOG_INFO, "Resyncing balls: host has %d balls, client has %d", 
                 state.ballCount, (int)balls.size());
    }
    
    balls.clear();
    for (int i = 0; i < state.ballCount; i++) {
        Ball ball(
            {state.ballX[i], state.ballY[i]},
            {state.ballSpeedX[i], state.ballSpeedY[i]},
            config.ballRadius
        );
        if (state.isFireball[i]) {
            ball.SetFire(config.powerups.fire_ball.duration);
        }
       ball.SetAttached(state.isAttached[i]);  // 如果游戏未开始，球附着
        balls.push_back(ball);
    }
}

// Game.cpp - CalculatePredictionError（计算预测误差）
float Game::CalculatePredictionError(const GameStateSnapshot& state) {
    if (balls.empty() || state.ballCount == 0) return 0.0f;
    
    float totalError = 0.0f;
    int count = std::min((int)balls.size(), state.ballCount);
    
    for (int i = 0; i < count; i++) {
        float dx = balls[i].GetPosition().x - state.ballX[i];
        float dy = balls[i].GetPosition().y - state.ballY[i];
        totalError += sqrtf(dx * dx + dy * dy);
    }
    
    return totalError / count;
}

// ========== 数据持久化实现 ==========

bool Game::SaveExists() const {
    std::ifstream f("save.json");
    return f.good();
}

void Game::SaveGame() {
    try {
        json save;
        save["version"] = 1;
        save["current_level"] = currentLevel;
        save["score"] = score;
        save["lives"] = lives;
        save["game_started"] = gameStarted;
        
        // 保存挡板状态
        save["paddle"]["width"] = paddle.GetWidth();
        save["paddle"]["effect_remaining"] = paddle.GetEffectRemaining();
        
        // 新增：保存砖块状态
        json bricks_arr = json::array();
        for (const auto& brick : bricks) {
            json b;
            b["active"] = brick.IsActive();
            b["health"] = brick.GetHealth();
            b["x"] = brick.GetRect().x;
            b["y"] = brick.GetRect().y;
            bricks_arr.push_back(b);
        }
        save["bricks"] = bricks_arr;

        // 保存所有球的状态
        json balls_arr = json::array();
        for (const auto& ball : balls) {
            json b;
            b["x"] = ball.GetPosition().x;
            b["y"] = ball.GetPosition().y;
            b["vx"] = ball.GetSpeed().x;
            b["vy"] = ball.GetSpeed().y;
            b["radius"] = ball.GetRadius();
            b["attached"] = ball.IsAttached();
            b["fireball_remaining"] = ball.GetFireTimer();
            balls_arr.push_back(b);
        }
        save["balls"] = balls_arr;
        
        // 减速效果剩余时间
        save["slow_effect_remaining"] = slowEffectRemainingTime;
        
        // 写入文件
        std::ofstream file("save.json");
        if (file.is_open()) {
            file << save.dump(4);  // 缩进4空格，方便阅读
            std::cout << "[Save] Game saved to save.json (Level " << currentLevel 
                      << ", Score " << score << ")" << std::endl;
        } else {
            std::cerr << "[Save] Failed to open save.json for writing" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[Save] Error: " << e.what() << std::endl;
    }
}

bool Game::LoadGame() {
    try {
        std::ifstream file("save.json");
        if (!file.is_open()) {
            std::cout << "[Load] save.json not found" << std::endl;
            return false;
        }
        
        json save = json::parse(file);
        
        // 版本检查
        int version = save.value("version", 0);
        if (version != 1) {
            std::cerr << "[Load] Unsupported save version: " << version << std::endl;
            return false;
        }
        
        // 基础数值恢复
        currentLevel = save["current_level"];
        score = save["score"];
        lives = save["lives"];
        gameStarted = save["game_started"];
        
        // 加载关卡配置（重要：必须先加载配置，再恢复具体状态）
        std::string levelFile = "level_" + std::to_string(currentLevel) + ".json";
        LoadLevelConfig(levelFile);
        
         // 恢复砖块状态（而不是重新构建）
        if (save.contains("bricks") && save["bricks"].is_array()) {
            bricks.clear();
            for (const auto& b : save["bricks"]) {
                // 根据保存的坐标和血量重建砖块
                float x = b["x"];
                float y = b["y"];
                int health = b["health"];
                bool active = b["active"];
                
                // 确定砖块颜色和分数（根据血量和关卡）
                Color brickColor;
                int points = 10;
                
                if (health >= 999) {
                    brickColor = GOLD;
                    points = 999;
                } else {
                    // 根据关卡和血量设置颜色
                    if (currentLevel == 1) {
                        if (health >= 2) brickColor = GREEN;
                        else brickColor = LIME;
                    } else if (currentLevel == 2) {
                        if (health >= 3) brickColor = ORANGE;
                        else if (health == 2) brickColor = YELLOW;
                        else brickColor = RED;
                    } else {
                        if (health >= 3) brickColor = BLUE;
                        else if (health == 2) brickColor = SKYBLUE;
                        else brickColor = DARKBLUE;
                    }
                    points = 10 * health;
                }
                
                Brick brick(x, y, config.brickWidth, config.brickHeight, 
                           brickColor, points, health, currentLevel);
                if (!active) {
                    brick.Destroy();
                }
                bricks.push_back(brick);
            }
        } else {
            // 兼容旧存档：没有砖块状态时重新构建
            BuildBricks();
        }

        // 恢复挡板
        float paddleW = save["paddle"]["width"];
        paddle.SetWidth(paddleW);
        paddle.SetEffectRemaining(save["paddle"]["effect_remaining"]);
        // 重新计算挡板位置（保持在底部）
        paddle.SetPosition((SW - paddle.GetWidth()) / 2.0f, SH - config.paddleYOffset);
        
        // 恢复所有球（清空现有球）
        balls.clear();
        for (const auto& b : save["balls"]) {
            Ball ball(
                Vector2{b["x"], b["y"]},
                Vector2{b["vx"], b["vy"]},
                b["radius"]
            );
            ball.SetAttached(b["attached"]);
            float fireRemaining = b.value("fireball_remaining", 0.0f);
            if (fireRemaining > 0.0f) {
                ball.SetFire(fireRemaining);
            }
            balls.push_back(ball);
        }
        
        // 如果加载后没有球（防止意外），创建一个默认球
        if (balls.empty()) {
            float ballX = paddle.GetPosition().x + paddle.GetWidth() / 2.0f;
            float ballY = paddle.GetPosition().y - config.ballRadius;
            balls.emplace_back(Vector2{ballX, ballY},
                               Vector2{config.ballSpeedX, config.ballSpeedY},
                               config.ballRadius);
            balls[0].SetAttached(true);
            gameStarted = false;
        }
        
        // 恢复减速效果
        slowEffectRemainingTime = save.value("slow_effect_remaining", 0.0f);
        if (slowEffectRemainingTime > 0.0f) {
            // 重新应用减速效果到所有球
            for (auto& ball : balls) {
                Vector2 spd = ball.GetSpeed();
                float factor = config.powerups.slow_ball.speed_factor;
                ball.SetSpeed(Vector2{spd.x * factor, spd.y * factor});
            }
        }
        
        // 清空道具（避免残留）
        powerUps.clear();
        particlePool.Clear();
        
        std::cout << "[Load] Game loaded successfully! Level " << currentLevel 
                  << ", Score " << score << ", Lives " << lives << std::endl;
        return true;
        
    } catch (const json::parse_error& e) {
        std::cerr << "[Load] JSON parse error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[Load] Error: " << e.what() << std::endl;
        return false;
    }
}

void Game::ContinueFromSave() {
    if (LoadGame()) {
        state = GameState::PLAYING;
        std::cout << "[Continue] Resumed from save file" << std::endl;
    } else {
        std::cout << "[Continue] Load failed, starting new game" << std::endl;
        ResetToNewGame();
        state = GameState::PLAYING;
    }
}

void Game::ResetToNewGame() {
    std::cout << "[Reset] Starting new game" << std::endl;
    
    currentLevel = 1;
    score = 0;
    lives = config.initialLives;
    gameStarted = false;
    slowEffectRemainingTime = 0.0f;
    
    // 重置挡板
    paddle.SetWidth(config.paddleWidth);
    paddle.SetEffectRemaining(0.0f);
    paddle.SetPosition((SW - config.paddleWidth) / 2.0f, SH - config.paddleYOffset);
    
    // 加载第一关配置
    LoadLevelConfig("level_1.json");
    BuildBricks();
    
    // 重置球（单个附着球）
    balls.clear();
    float ballX = paddle.GetPosition().x + paddle.GetWidth() / 2.0f;
    float ballY = paddle.GetPosition().y - config.ballRadius;
    balls.emplace_back(Vector2{ballX, ballY},
                       Vector2{config.ballSpeedX, config.ballSpeedY},
                       config.ballRadius);
    balls[0].SetAttached(true);
    
    // 清理道具和粒子
    powerUps.clear();
    particlePool.Clear();
    
    std::cout << "[Reset] New game ready" << std::endl;
}
// Game.cpp - 在文件末尾添加编辑器实现

// ========== 关卡编辑器实现 ==========

void Game::HandleEditorMode() {
    // 切换编辑模式（按 E 键）
   if (IsKeyPressed(KEY_E)) {
        editorMode = !editorMode;
        if (editorMode) {
            // 进入编辑模式时暂停游戏
            if (state == GameState::PLAYING) {
                state = GameState::PAUSED;
            }
            // 使用配置中的砖块尺寸
            editorGridCols = config.brickCols;
            editorGridRows = config.brickRows;
            TraceLog(LOG_INFO, "Editor mode: ON (using brick size: %.0f x %.0f)", 
                     config.brickWidth, config.brickHeight);
        } else {
            TraceLog(LOG_INFO, "Editor mode: OFF");
        }
    }
    
    if (!editorMode) return;
    
    // 选择砖块类型（数字键 1-9 选择血量）
    for (int i = 1; i <= 9; i++) {
        if (IsKeyPressed(KEY_ONE + i - 1)) {
            selectedBrickType = i;
            TraceLog(LOG_INFO, "Selected brick type: %d HP", selectedBrickType);
        }
    }
    // 0 键选择无敌砖块
    if (IsKeyPressed(KEY_ZERO)) {
        selectedBrickType = 999;
        TraceLog(LOG_INFO, "Selected brick type: INVINCIBLE");
    }
    
    // 左键添加砖块
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        AddBrickAtMouse();
    }
    
    // 右键删除砖块
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        RemoveBrickAtMouse();
    }
    
    // S 键保存关卡
    if (IsKeyPressed(KEY_S)) {
        SaveCurrentLayout();
    }
    
    // K 键加载自定义关卡
    if (IsKeyPressed(KEY_K)) {
        LoadCustomLevel("levels/custom.json");
    }
    
    // Delete 键清空所有砖块
    if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE)) {
        bricks.clear();
        TraceLog(LOG_INFO, "Editor: Cleared all bricks");
    }
}

void Game::AddBrickAtMouse() {
    Vector2 mouse = GetMousePosition();
    
    // 使用配置中的砖块尺寸和间距
    float brickWidth = config.brickWidth;
    float brickHeight = config.brickHeight;
    float paddingX = config.brickPaddingX;
    float paddingY = config.brickPaddingY;
    float startX = config.brickStartX;
    float startY = config.brickStartY;

    // 计算鼠标所在的网格位置
      int gridX = (int)((mouse.x - startX) / (brickWidth + paddingX));
    int gridY = (int)((mouse.y - startY) / (brickHeight + paddingY));
    
    // 边界检查
    if (gridX < 0 || gridX >= editorGridCols || gridY < 0 || gridY >= editorGridRows) {
        return;
    }
    
    // 计算砖块的实际位置
    float brickX = startX + gridX * (brickWidth + paddingX);
    float brickY = startY + gridY * (brickHeight + paddingY);
    
    // 检查是否已有砖块在同一位置
    for (auto& brick : bricks) {
        Rectangle rect = brick.GetRect();
        if (fabs(rect.x - brickX) < 5.0f && fabs(rect.y - brickY) < 5.0f && brick.IsActive()) {
            TraceLog(LOG_WARNING, "Editor: Brick already exists at this position");
            return;
        }
    }
    
    // 确定砖块颜色
    Color brickColor;
    if (selectedBrickType >= 999) {
        brickColor = GOLD;
    } else if (selectedBrickType >= 4) {
        brickColor = RED;
    } else if (selectedBrickType == 3) {
        brickColor = ORANGE;
    } else if (selectedBrickType == 2) {
        brickColor = GREEN;
    } else {
        brickColor = BLUE;
    }
    
    int points = (selectedBrickType >= 999) ? 999 : selectedBrickType * 10;
    
    // 添加砖块
    bricks.emplace_back(brickX, brickY, brickWidth, brickHeight, 
                        brickColor, points, selectedBrickType, currentLevel);
    
    TraceLog(LOG_INFO, "Editor: Added brick at (%d, %d) with %d HP", 
             gridX, gridY, selectedBrickType);
}

void Game::RemoveBrickAtMouse() {
    Vector2 mouse = GetMousePosition();
    
    // 使用配置中的砖块尺寸
    float brickWidth = config.brickWidth;
    float brickHeight = config.brickHeight;
    float paddingX = config.brickPaddingX;
    float paddingY = config.brickPaddingY;
    float startX = config.brickStartX;
    float startY = config.brickStartY;
    
    // 计算鼠标所在的网格位置
    int gridX = (int)((mouse.x - startX) / (brickWidth + paddingX));
    int gridY = (int)((mouse.y - startY) / (brickHeight + paddingY));
    
    // 边界检查
    if (gridX < 0 || gridX >= editorGridCols || gridY < 0 || gridY >= editorGridRows) {
        return;
    }
    
    // 计算该网格位置的砖块矩形
    float brickX = startX + gridX * (brickWidth + paddingX);
    float brickY = startY + gridY * (brickHeight + paddingY);
    
    // 找到并删除该位置的砖块
    for (auto it = bricks.begin(); it != bricks.end(); ++it) {
        if (!it->IsActive()) continue;
        
        Rectangle rect = it->GetRect();
        // 检查位置是否匹配（允许小误差）
        if (fabs(rect.x - brickX) < 5.0f && fabs(rect.y - brickY) < 5.0f) {
            it->Destroy();
            TraceLog(LOG_INFO, "Editor: Removed brick at (%d, %d)", gridX, gridY);
            return;
        }
    }
}
void Game::SaveCurrentLayout() {
    // 创建 levels 目录（如果不存在）
    int ret = system("mkdir -p levels");
    if (ret != 0) {
        TraceLog(LOG_WARNING, "Editor: Failed to create levels directory");
        // 继续执行，因为目录可能已经存在
    }
    
    json layout;
    
    // 初始化布局矩阵为全0
    std::vector<std::vector<int>> layoutData(editorGridRows, 
                                              std::vector<int>(editorGridCols, 0));
    
    // 填充砖块数据
    for (const auto& brick : bricks) {
        if (!brick.IsActive()) continue;
        
        Rectangle rect = brick.GetRect();
        
        // 使用配置中的砖块尺寸计算网格位置
        float brickWidth = config.brickWidth;
        float brickHeight = config.brickHeight;
        float paddingX = config.brickPaddingX;
        float paddingY = config.brickPaddingY;
        float startX = config.brickStartX;
        float startY = config.brickStartY;
        
        int gridX = (int)((rect.x - startX) / (brickWidth + paddingX) + 0.5f);
        int gridY = (int)((rect.y - startY) / (brickHeight + paddingY) + 0.5f);
        
        if (gridX >= 0 && gridX < editorGridCols && 
            gridY >= 0 && gridY < editorGridRows) {
            int health = brick.GetHealth();
            layoutData[gridY][gridX] = (health >= 999) ? 999 : health;
        }
    }
    
    // 使用配置文件中的数值
    layout["bricks"]["rows"] = editorGridRows;
    layout["bricks"]["cols"] = editorGridCols;
    layout["bricks"]["width"] = config.brickWidth;
    layout["bricks"]["height"] = config.brickHeight;
    layout["bricks"]["startX"] = config.brickStartX;
    layout["bricks"]["startY"] = config.brickStartY;
    layout["bricks"]["paddingX"] = config.brickPaddingX;
    layout["bricks"]["paddingY"] = config.brickPaddingY;
    layout["bricks"]["layout"] = layoutData;
    
    // 健康值映射
    json healthMap;
    for (int i = 1; i <= 9; i++) {
        healthMap[std::to_string(i)] = i;
    }
    healthMap["999"] = 999;
    layout["bricks"]["health_map"] = healthMap;
    
    // 保存文件
    std::string filename = "levels/custom_" + 
                          std::to_string(time(nullptr)) + ".json";
    std::ofstream file(filename);
    if (file.is_open()) {
        file << layout.dump(4);
        TraceLog(LOG_INFO, "Editor: Saved layout to %s", filename.c_str());
    } else {
        TraceLog(LOG_ERROR, "Editor: Failed to save layout");
    }
}

void Game::LoadCustomLevel(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            TraceLog(LOG_WARNING, "Editor: Cannot open %s", path.c_str());
            return;
        }
        
        json levelJson = json::parse(file);
        
        // 清空现有砖块
        bricks.clear();
        
        // 读取布局
        auto bricksCfg = levelJson["bricks"];
        int rows = bricksCfg["rows"];
        int cols = bricksCfg["cols"];
        float width = bricksCfg["width"];
        float height = bricksCfg["height"];
        float startX = bricksCfg.value("startX", 1.0f);
        float startY = bricksCfg.value("startY", 1.0f);
        float paddingX = bricksCfg.value("paddingX", 0.0f);
        float paddingY = bricksCfg.value("paddingY", 0.0f);
        
        auto layout = bricksCfg["layout"];
        auto healthMap = bricksCfg.value("health_map", json::object());
        
        // 更新网格尺寸
        editorGridCols = cols;
        editorGridRows = rows;
        editorCellWidth = (float)SW / editorGridCols;
        editorCellHeight = (float)SH / editorGridRows;
        
        // 颜色方案
        Color levelColors[] = {
            {200, 50, 50, 255},    // 红色系 (4+ HP)
            {200, 130, 50, 255},   // 橙色系 (3 HP)
            {50, 150, 50, 255},    // 绿色系 (2 HP)
            {50, 100, 200, 255},   // 蓝色系 (1 HP)
            {180, 50, 180, 255}    // 紫色系
        };
        
        for (int r = 0; r < rows && r < (int)layout.size(); r++) {
            for (int c = 0; c < cols && c < (int)layout[r].size(); c++) {
                int type = layout[r][c];
                if (type == 0) continue;
                
                float x = startX + c * (width + paddingX);
                float y = startY + r * (height + paddingY);
                
                int hp = type;
                if (healthMap.contains(std::to_string(type))) {
                    hp = healthMap[std::to_string(type)];
                }
                
                Color brickColor;
                if (hp >= 999) {
                    brickColor = GOLD;
                } else if (hp >= 4) {
                    brickColor = levelColors[0];
                } else if (hp == 3) {
                    brickColor = levelColors[1];
                } else if (hp == 2) {
                    brickColor = levelColors[2];
                } else {
                    brickColor = levelColors[3];
                }
                
                bricks.emplace_back(x, y, width, height, brickColor, hp * 10, hp, 0);
            }
        }
        
        TraceLog(LOG_INFO, "Editor: Loaded custom level with %zu bricks", bricks.size());
        
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "Editor: Failed to load level - %s", e.what());
    }
}
void Game::DrawEditorUI() {
    if (!editorMode) return;
    
    // 使用配置中的砖块尺寸绘制网格线
    float brickWidth = config.brickWidth;
    float brickHeight = config.brickHeight;
    float paddingX = config.brickPaddingX;
    float paddingY = config.brickPaddingY;
    float startX = config.brickStartX;
    float startY = config.brickStartY;
    
    // 绘制网格线（垂直线）
    for (int c = 0; c <= editorGridCols; c++) {
        float x = startX + c * (brickWidth + paddingX);
        DrawLine(x, 0, x, SH, ColorAlpha(GRAY, 0.5f));
    }
    // 绘制网格线（水平线）
    for (int r = 0; r <= editorGridRows; r++) {
        float y = startY + r * (brickHeight + paddingY);
        DrawLine(0, y, SW, y, ColorAlpha(GRAY, 0.5f));
    }
    
    // 绘制鼠标位置的预览砖块
    Vector2 mouse = GetMousePosition();
    int gridX = (int)((mouse.x - startX) / (brickWidth + paddingX));
    int gridY = (int)((mouse.y - startY) / (brickHeight + paddingY));
    
    if (gridX >= 0 && gridX < editorGridCols && 
        gridY >= 0 && gridY < editorGridRows) {
        float previewX = startX + gridX * (brickWidth + paddingX);
        float previewY = startY + gridY * (brickHeight + paddingY);
        
        Color previewColor;
        if (selectedBrickType >= 999) {
            previewColor = ColorAlpha(GOLD, 0.5f);
        } else if (selectedBrickType >= 4) {
            previewColor = ColorAlpha((Color){200, 50, 50, 255}, 0.5f);
        } else if (selectedBrickType == 3) {
            previewColor = ColorAlpha((Color){200, 130, 50, 255}, 0.5f);
        } else if (selectedBrickType == 2) {
            previewColor = ColorAlpha((Color){50, 150, 50, 255}, 0.5f);
        } else {
            previewColor = ColorAlpha((Color){50, 100, 200, 255}, 0.5f);
        }
        
        DrawRectangle(previewX, previewY, brickWidth, brickHeight, previewColor);
        DrawRectangleLines(previewX, previewY, brickWidth, brickHeight, WHITE);
    }
    
    // 绘制编辑器UI面板
    DrawRectangle(0, 0, SW, 80, ColorAlpha(BLACK, 0.8f));
    
    int y = 10;
    int x = 10;
    
    DrawText("=== EDITOR MODE ===", x, y, 20, YELLOW);
    y += 25;
    
    // 当前选择的砖块类型
    std::string typeText;
    if (selectedBrickType >= 999) {
        typeText = "INVINCIBLE";
    } else {
        typeText = std::to_string(selectedBrickType) + " HP";
    }
    DrawText(TextFormat("Selected: %s", typeText.c_str()), x, y, 16, WHITE);
    
    // 操作提示
    DrawText("Controls:", x + 200, y, 16, LIGHTGRAY);
    DrawText("1-9: HP | 0: Invincible | Left: Add | Right: Remove", 
             x + 280, y, 16, LIGHTGRAY);
    y += 20;
    DrawText("S: Save | K: Load custom | DEL: Clear all | E: Exit editor", 
             x, y, 16, LIGHTGRAY);
}
// Game.cpp - 添加配置文件预校验

bool Game::ValidateConfigFile(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            ShowConfigError("Config file not found: " + path);
            return false;
        }
        
        // 检查文件是否为空
        file.seekg(0, std::ios::end);
        if (file.tellg() == 0) {
            ShowConfigError("Config file is empty: " + path);
            return false;
        }
        file.seekg(0, std::ios::beg);
        
        // 尝试解析 JSON
        json config = json::parse(file);
        
        // 验证必需字段
        if (!config.contains("bricks")) {
            ShowConfigError("Missing 'bricks' section in " + path);
            return false;
        }
        
        return true;
        
    } catch (const json::parse_error& e) {
        ShowConfigError("JSON parse error in " + path + ": " + e.what());
        return false;
    } catch (const std::exception& e) {
        ShowConfigError("Error reading " + path + ": " + e.what());
        return false;
    }
}