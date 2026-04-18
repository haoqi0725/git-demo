#include "../include/Game.h"
#include "../include/nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <ctime>
#include <algorithm>
#include <cmath>

using json = nlohmann::json;

// ========== 构造函数 ==========
Game::Game() 
    : SW(800), SH(600), windowTitle("ARKANOID 2D")
    , state(GameState::MENU), score(0), lives(3)
    , paddle(350, 550, 100, 20)
    , playerName(""), nameInputCursor(0), isEnteringName(false)
    , debugMode(true), godMode(false), currentLevel(1)
    , powerUpDropRate(0.3f), slowEffectRemainingTime(0.0f)
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
            config.initialLives = 3; config.scorePerBrick = 10;
            config.timeMultiplierDecay = 0.05;
            config.powerups.paddle_extend = {40.0f, 5.0f, 0.3f};
            config.powerups.multi_ball = {2, 0.0f, 0.2f};
            config.powerups.slow_ball = {0.7f, 5.0f, 0.25f};
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
    LoadConfig("config.json");
    LoadLeaderboard();

    balls.clear();
    balls.emplace_back(Vector2{(float)SW/2, (float)SH/2},
                       Vector2{config.ballSpeedX, config.ballSpeedY},
                       config.ballRadius);
    
    paddle = Paddle((SW - config.paddleWidth) / 2,
                    SH - config.paddleYOffset,
                    config.paddleWidth,
                    config.paddleHeight);
    BuildBricks();
    
    powerUps.clear();
    particles.clear();
    
    particles.resize(MAX_PARTICLES);
    for (auto& p : particles) {
        p.active = false;
    }
}

// ========== 构建砖块 ==========
void Game::BuildBricks() {
    bricks.clear();
    for (int r = 0; r < config.brickRows; ++r) {
        for (int c = 0; c < config.brickCols; ++c) {
            float x = config.brickStartX + c * (config.brickWidth + config.brickPaddingX);
            float y = config.brickStartY + r * (config.brickHeight + config.brickPaddingY);
            bricks.emplace_back(x, y, config.brickWidth, config.brickHeight);
        }
    }
}

// ========== 重置游戏 ==========
void Game::ResetGame() {
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
                    config.paddleHeight);
    
    isEnteringName = false;
    playerName = "";
    
    powerUps.clear();
    particles.clear();
    
    particles.resize(MAX_PARTICLES);
    for (auto& p : particles) {
        p.active = false;
    }
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
        
        if (p.position.x - 3 <= 0) { p.position.x = 3; p.velocity.x = -p.velocity.x; }
        if (p.position.x + 3 >= SW) { p.position.x = SW - 3; p.velocity.x = -p.velocity.x; }
        if (p.position.y - 3 <= 0) { p.position.y = 3; p.velocity.y = -p.velocity.y; }
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

// ========== 球更新 ==========
void Game::UpdateBalls(float dt) {
    for (auto& ball : balls) {
        ball.Move();
        ball.BounceEdge(SW, SH);
    }
}

// ========== 主更新循环 ==========
void Game::Update() {
    if (state != GameState::LEADERBOARD && IsKeyPressed(KEY_L)) {
        LoadLeaderboard();
        ChangeState(GameState::LEADERBOARD);
        return;
    }
    
    float dt = GetFrameTime();
    
    // 更新减速效果计时器
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
            if (IsKeyPressed(KEY_SPACE)) {
                ResetGame();
                ChangeState(GameState::PLAYING);
            }
            break;
            
        case GameState::PLAYING: {
            if (IsKeyPressed(KEY_P)) {
                ChangeState(GameState::PAUSED);
                return;
            }
            
            if (IsKeyDown(KEY_LEFT)) paddle.MoveLeft(config.paddleSpeed);
            if (IsKeyDown(KEY_RIGHT)) paddle.MoveRight(config.paddleSpeed);
            paddle.ClampToScreen(SW);
            
            if (IsKeyPressed(KEY_K)) {
                powerUps.clear();
            }
            
            if (IsKeyPressed(KEY_END)) {
                isEnteringName = true;
                ChangeState(GameState::GAMEOVER);
                return;
            }
            
            UpdatePowerUps(dt);
            HandlePowerUpCollisions();
            UpdateParticles(dt);
            UpdateBalls(dt);
            
            for (auto& ball : balls) {
                ball.CheckPaddleCollision(paddle.GetRect());
            }
            
            for (auto& brick : bricks) {
                brick.SetHitThisFrame(false);
            }
            
            for (auto& ball : balls) {
                for (auto& brick : bricks) {
                    if (!brick.IsActive() || brick.HitThisFrame()) continue;
                    
                    Rectangle rect = brick.GetRect();
                    auto side = ball.CheckBrickCollision(rect);
                    if (side == CollisionSide::NONE) continue;
                    
                    brick.SetActive(false);
                    brick.SetHitThisFrame(true);
                    score += config.scorePerBrick;
                    SpawnParticles(rect, brick.GetColor());
                    
                    int dropChance = (int)(powerUpDropRate * 100);
                    if (GetRandomValue(0, 99) < dropChance) {
                        PowerUpType type = static_cast<PowerUpType>(GetRandomValue(0, 2));
                        SpawnPowerUp(rect.x + rect.width / 2, rect.y + rect.height / 2, type);
                    }
                    
                    if (side == CollisionSide::LEFT || side == CollisionSide::RIGHT)
                        ball.ReverseX();
                    else
                        ball.ReverseY();
                    
                    break;
                }
            }
            
            int activeBricks = 0;
            for (auto& brick : bricks) {
                if (brick.IsActive()) activeBricks++;
            }
            
            bool ballLost = false;
            for (auto it = balls.begin(); it != balls.end(); ) {
                if (it->GetPosition().y + it->GetRadius() >= SH) {
                    std::cout << "Ball died at (" << it->GetPosition().x << ", " << it->GetPosition().y << ") speed=(" << it->GetSpeed().x << ", " << it->GetSpeed().y << ") Remaining: " << balls.size()-1 << std::endl;
                    it = balls.erase(it);
                    ballLost = true;
                } else {
                    ++it;
                }
            }
            
            if (balls.empty()) {  // 只要球没了就处理，不管是不是刚掉的
                lives--;
                if (lives <= 0 && !godMode) {
                    isEnteringName = true;
                    ChangeState(GameState::GAMEOVER);
                } else {
                    balls.emplace_back(Vector2{(float)SW/2, (float)SH/2},
                                    Vector2{config.ballSpeedX, config.ballSpeedY},
                                    config.ballRadius);
                }
                return;
            }
            
            if (activeBricks == 0) {
                float currentPaddleWidth = paddle.GetWidth();
                BuildBricks();
                if (currentPaddleWidth > config.paddleWidth) {
                    paddle.Extend(currentPaddleWidth - config.paddleWidth, 0);
                }
                if (balls.empty()) {
                    balls.emplace_back(Vector2{(float)SW/2, (float)SH/2},
                                       Vector2{config.ballSpeedX, config.ballSpeedY},
                                       config.ballRadius);
                } else {
                    for (auto& ball : balls) {
                        ball.SetPosition(Vector2{(float)SW/2, (float)SH/2});
                        ball.SetSpeed(Vector2{config.ballSpeedX, config.ballSpeedY});
                    }
                }
                // 只停用粒子，不清空粒子池
                for (auto& p : particles) {
                    p.active = false;
                }
            }


            break;
        }
        
        case GameState::PAUSED:
            if (IsKeyPressed(KEY_P)) ChangeState(GameState::PLAYING);
            break;
            
        case GameState::GAMEOVER:
            if (isEnteringName) {
                if (IsKeyPressed(KEY_ENTER)) {
                    AddScoreToLeaderboard();
                    isEnteringName = false;
                    ChangeState(GameState::MENU);
                }
            } else {
                if (IsKeyPressed(KEY_R)) ChangeState(GameState::MENU);
            }
            break;
            
        case GameState::VICTORY:
            if (isEnteringName) {
                if (IsKeyPressed(KEY_ENTER)) {
                    AddScoreToLeaderboard();
                    isEnteringName = false;
                    ChangeState(GameState::MENU);
                }
            } else {
                if (IsKeyPressed(KEY_R)) ChangeState(GameState::MENU);
            }
            break;
            
        case GameState::LEADERBOARD:
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_R)) ChangeState(GameState::MENU);
            break;
    }
}

// ========== 绘制 ==========
void Game::Draw() {
    switch (state) {
        case GameState::MENU: DrawMenu(); break;
        case GameState::PLAYING: DrawPlaying(); break;
        case GameState::PAUSED: DrawPaused(); break;
        case GameState::GAMEOVER: DrawGameOver(); break;
        case GameState::VICTORY: DrawWin(); break;
        case GameState::LEADERBOARD: DrawLeaderboard(); break;
    }
}

void Game::DrawMenu() {
    DrawText(windowTitle.c_str(), SW/2 - 150, 150, 40, WHITE);
    DrawText("PRESS SPACE TO START", SW/2 - 130, 230, 24, GRAY);
    DrawText("PRESS L FOR LEADERBOARD", SW/2 - 130, 270, 20, DARKGRAY);
    DrawText("ARROWS MOVE  P  PAUSE", SW/2 - 130, 310, 20, DARKGRAY);
}

void Game::DrawPlaying() {
    DrawRectangle(0, 0, SW, 5, DARKGRAY);
    DrawRectangle(0, 0, 5, SH, DARKGRAY);
    DrawRectangle(SW-5, 0, 5, SH, DARKGRAY);
    for (auto& ball : balls) ball.Draw();
    paddle.Draw();
    for (auto& b : bricks) if (b.IsActive()) b.Draw();
    for (auto& p : powerUps) p.Draw();
    DrawParticles();
    DrawText(TextFormat("SCORE: %d", score), 10, 10, 20, WHITE);
    DrawText(TextFormat("BALLS: %d", (int)balls.size()), 10, 35, 20, YELLOW);
    DrawText(TextFormat("LIVES: %d", lives), SW - 100, 10, 20, RED);
    if (debugMode) DrawDebugInfo();
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
}

void Game::Shutdown() {
    SaveLeaderboard();
}