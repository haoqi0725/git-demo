#include "../include/Game.h"
#include "../include/nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <ctime>
#include <algorithm>

using json = nlohmann::json;

// ========== 构造函数 ==========
Game::Game() 
    : SW(800), SH(600), windowTitle("ARKANOID 2D")
    , state(GameState::MENU), score(0), lives(3)
    , ball({400, 300}, {3, -4}, 10)
    , paddle(350, 550, 100, 20)
    , playerName(""), nameInputCursor(0), isEnteringName(false)
    , debugMode(false), godMode(false), currentLevel(1) {
    levels = {"config/level_1.json", "config/level_2.json", "config/level_3.json"};
}

Game::~Game() {
}

// ========== 配置加载 ==========
void Game::LoadConfig(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Warning: Cannot open config: " << path << std::endl;
            return;
        }
        json cfg = json::parse(file);
        SW = cfg.value("window", json::object()).value("width", 800);
        SH = cfg.value("window", json::object()).value("height", 600);
        windowTitle = cfg.value("window", json::object()).value("title", "ARKANOID 2D");
        
        auto ballCfg = cfg.value("ball", json::object());
        config.ballRadius = ballCfg.value("radius", 10.0f);
        config.ballSpeedX = ballCfg.value("speedX", 3.0f);
        config.ballSpeedY = ballCfg.value("speedY", -4.0f);
        config.gravity = ballCfg.value("gravity", 0.08f);
        config.maxSpeed = ballCfg.value("maxSpeed", 15.0f);
        config.bounceForce = ballCfg.value("bounceForce", 0.5f);
        
        auto paddleCfg = cfg.value("paddle", json::object());
        config.paddleWidth = paddleCfg.value("width", 100);
        config.paddleHeight = paddleCfg.value("height", 20);
        config.paddleSpeed = paddleCfg.value("speed", 8);
        config.paddleYOffset = paddleCfg.value("yOffset", 50);
        config.paddleBoostSpeed = paddleCfg.value("boostSpeed", 28);
        
        auto bricksCfg = cfg.value("bricks", json::object());
        config.brickRows = bricksCfg.value("rows", 4);
        config.brickCols = bricksCfg.value("cols", 9);
        config.brickWidth = bricksCfg.value("width", 75.0f);
        config.brickHeight = bricksCfg.value("height", 25.0f);
        config.brickStartX = bricksCfg.value("startX", 25.0f);
        config.brickStartY = bricksCfg.value("startY", 80.0f);
        config.brickPaddingX = bricksCfg.value("paddingX", 10.0f);
        config.brickPaddingY = bricksCfg.value("paddingY", 8.0f);
        
        auto gameCfg = cfg.value("game", json::object());
        config.initialLives = gameCfg.value("initialLives", 3);
        config.scorePerBrick = gameCfg.value("scorePerBrick", 10);
        config.timeMultiplierDecay = gameCfg.value("timeMultiplierDecay", 0.05f);
    } catch (...) {}
}

// ========== 排行榜 ==========
void Game::LoadLeaderboard() {
    leaderboard.clear();
    std::ifstream file("leaderboard.txt");
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            size_t p1 = line.find('|'), p2 = line.find('|', p1 + 1);
            if (p1 != std::string::npos && p2 != std::string::npos) {
                ScoreEntry se;
                se.name = line.substr(0, p1);
                se.score = std::stoi(line.substr(p1 + 1, p2 - p1 - 1));
                se.date = line.substr(p2 + 1);
                leaderboard.push_back(se);
            }
        }
    }
    std::sort(leaderboard.begin(), leaderboard.end(),
              [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });
}

void Game::SaveLeaderboard() {
    std::ofstream file("leaderboard.txt");
    for (const auto& e : leaderboard)
        file << e.name << "|" << e.score << "|" << e.date << "\n";
}

void Game::AddScoreToLeaderboard() {
    time_t now = time(nullptr);
    std::string date = ctime(&now); date.pop_back();
    ScoreEntry ne;
    ne.name = playerName.empty() ? "Anonymous" : playerName;
    ne.score = score;
    ne.date = date;
    leaderboard.push_back(ne);
    std::sort(leaderboard.begin(), leaderboard.end(),
              [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });
    if (leaderboard.size() > 10) leaderboard.resize(10);
    SaveLeaderboard();
}

// ========== 游戏逻辑 ==========
void Game::Init() {
    LoadConfig("config/level_1.json");
    LoadLeaderboard();
    ball = Ball({(float)SW/2, (float)SH/2}, {config.ballSpeedX, config.ballSpeedY}, config.ballRadius);
    paddle = Paddle((SW - config.paddleWidth)/2, SH - config.paddleYOffset, config.paddleWidth, config.paddleHeight);
    BuildBricks();
}

void Game::BuildBricks() {
    bricks.clear();
    for (int r = 0; r < config.brickRows; r++) {
        for (int c = 0; c < config.brickCols; c++) {
            float x = config.brickStartX + c * (config.brickWidth + config.brickPaddingX);
            float y = config.brickStartY + r * (config.brickHeight + config.brickPaddingY);
            bricks.emplace_back(x, y, config.brickWidth, config.brickHeight);
        }
    }
}

void Game::ResetGame() {
    score = 0;
    lives = config.initialLives;
    BuildBricks();
    ball.Reset({(float)SW/2, (float)SH/2}, {config.ballSpeedX, config.ballSpeedY});
    paddle = Paddle((SW - config.paddleWidth)/2, SH - config.paddleYOffset, config.paddleWidth, config.paddleHeight);
    isEnteringName = false;
    playerName = "";
}

void Game::SwitchLevel(int level) {
    currentLevel = level;
    std::string path = levels[level-1];
    LoadConfig(path);
    ball = Ball({(float)SW/2, (float)SH/2}, {config.ballSpeedX, config.ballSpeedY}, config.ballRadius);
    paddle = Paddle((SW - config.paddleWidth)/2, SH - config.paddleYOffset, config.paddleWidth, config.paddleHeight);
    BuildBricks();
    std::cout << "Level " << level << std::endl;
}

void Game::ChangeState(GameState newState) { state = newState; }

// ========== 状态处理 ==========
void Game::HandleMenuState() {
    if (IsKeyPressed(KEY_SPACE)) { ResetGame(); ChangeState(GameState::PLAYING); }
    if (IsKeyPressed(KEY_L)) { LoadLeaderboard(); ChangeState(GameState::LEADERBOARD); }
}

void Game::HandlePlayingState() {
    if (IsKeyPressed(KEY_D)) { debugMode = !debugMode; }
    if (IsKeyPressed(KEY_G)) { godMode = !godMode; }
    if (IsKeyPressed(KEY_ONE)) SwitchLevel(1);
    if (IsKeyPressed(KEY_TWO)) SwitchLevel(2);
    if (IsKeyPressed(KEY_THREE)) SwitchLevel(3);
    if (IsKeyPressed(KEY_P)) { ChangeState(GameState::PAUSED); return; }
    if (IsKeyDown(KEY_LEFT)) paddle.MoveLeft(config.paddleSpeed);
    if (IsKeyDown(KEY_RIGHT)) paddle.MoveRight(config.paddleSpeed);
    
    ball.Move();
    ball.BounceEdge(SW, SH);
    ball.CheckPaddleCollision(paddle.GetRect());
    
    int active = 0;
    for (auto& b : bricks) {
        if (!b.IsActive()) continue;
        active++;
        auto side = ball.CheckBrickCollision(b.GetRect());
        if (side != CollisionSide::NONE) {
            b.SetActive(false);
            score += config.scorePerBrick;
            if (side == CollisionSide::LEFT || side == CollisionSide::RIGHT) ball.ReverseX();
            else ball.ReverseY();
            break;
        }
    }
    
    if (ball.GetPosition().y + ball.GetRadius() >= SH) {
        if (godMode) ball.Reset({(float)SW/2, (float)SH/2}, {config.ballSpeedX, config.ballSpeedY});
        else {
            lives--;
            if (lives <= 0) { isEnteringName = true; ChangeState(GameState::GAMEOVER); }
            else ball.Reset({(float)SW/2, (float)SH/2}, {config.ballSpeedX, config.ballSpeedY});
        }
        return;
    }
    
    if (active == 0) {
        if (currentLevel < 3) SwitchLevel(currentLevel + 1);
        else { isEnteringName = true; ChangeState(GameState::VICTORY); }
    }
}

void Game::HandlePausedState() { if (IsKeyPressed(KEY_P)) ChangeState(GameState::PLAYING); }
void Game::HandleGameOverState() {
    if (isEnteringName) { if (IsKeyPressed(KEY_ENTER)) { AddScoreToLeaderboard(); isEnteringName = false; ChangeState(GameState::MENU); } }
    else { if (IsKeyPressed(KEY_R)) ChangeState(GameState::MENU); if (IsKeyPressed(KEY_L)) { LoadLeaderboard(); ChangeState(GameState::LEADERBOARD); } }
}
void Game::HandleWinState() {
    if (isEnteringName) { if (IsKeyPressed(KEY_ENTER)) { AddScoreToLeaderboard(); isEnteringName = false; ChangeState(GameState::MENU); } }
    else { if (IsKeyPressed(KEY_R)) ChangeState(GameState::MENU); if (IsKeyPressed(KEY_L)) { LoadLeaderboard(); ChangeState(GameState::LEADERBOARD); } }
}
void Game::HandleLeaderboardState() { if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_R)) ChangeState(GameState::MENU); }

void Game::Update() {
    if (state != GameState::LEADERBOARD && IsKeyPressed(KEY_L)) { LoadLeaderboard(); ChangeState(GameState::LEADERBOARD); return; }
    switch (state) {
        case GameState::MENU: HandleMenuState(); break;
        case GameState::PLAYING: HandlePlayingState(); break;
        case GameState::PAUSED: HandlePausedState(); break;
        case GameState::GAMEOVER: HandleGameOverState(); break;
        case GameState::VICTORY: HandleWinState(); break;
        case GameState::LEADERBOARD: HandleLeaderboardState(); break;
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
    DrawText(windowTitle.c_str(), SW/2-150, 150, 40, WHITE);
    DrawText("PRESS SPACE TO START", SW/2-130, 230, 24, GRAY);
    DrawText("PRESS L FOR LEADERBOARD", SW/2-130, 270, 20, DARKGRAY);
    DrawText("1,2,3 SWITCH LEVEL | D DEBUG | G GOD", SW/2-160, 310, 16, DARKGRAY);
}

void Game::DrawPlaying() {
    DrawRectangle(0,0,SW,5,DARKGRAY); DrawRectangle(0,0,5,SH,DARKGRAY); DrawRectangle(SW-5,0,5,SH,DARKGRAY);
    ball.Draw(); paddle.Draw();
    for (auto& b : bricks) if(b.IsActive()) b.Draw();
    DrawText(TextFormat("SCORE: %d", score), 10, 10, 20, WHITE);
    DrawText(TextFormat("LIVES: %d", lives), SW-100, 10, 20, RED);
    DrawText(TextFormat("LEVEL: %d/3", currentLevel), SW/2-40, 10, 20, YELLOW);
    if (debugMode) DrawDebugInfo();
}

void Game::DrawPaused() { DrawPlaying(); DrawRectangle(0,0,SW,SH,ColorAlpha(BLACK,0.7f)); DrawText("PAUSED", SW/2-80, SH/2-20, 48, YELLOW); DrawText("PRESS P TO RESUME", SW/2-120, SH/2+40, 24, WHITE); }
void Game::DrawGameOver() { DrawRectangle(0,0,SW,SH,ColorAlpha(BLACK,0.8f)); DrawText("GAME OVER", SW/2-100, SH/2-60, 48, RED); DrawText(TextFormat("SCORE: %d", score), SW/2-100, SH/2, 28, WHITE); if(isEnteringName) DrawNameInput(); else DrawText("PRESS R TO MENU", SW/2-90, SH/2+60, 22, GRAY); }
void Game::DrawWin() { DrawRectangle(0,0,SW,SH,ColorAlpha(BLACK,0.8f)); DrawText("YOU WIN!", SW/2-80, SH/2-60, 48, GOLD); DrawText(TextFormat("SCORE: %d", score), SW/2-100, SH/2, 28, WHITE); if(isEnteringName) DrawNameInput(); else DrawText("PRESS R TO MENU", SW/2-90, SH/2+60, 22, GRAY); }

void Game::DrawLeaderboard() {
    DrawRectangle(0,0,SW,SH,ColorAlpha(BLACK,0.9f));
    DrawText("=== LEADERBOARD ===", SW/2-120, 50, 30, GOLD);
    DrawText("RANK", 150, 120, 20, WHITE); DrawText("NAME", 300, 120, 20, WHITE); DrawText("SCORE", 550, 120, 20, WHITE);
    DrawLine(100,145,SW-100,145,GRAY);
    int y=170;
    for(size_t i=0;i<leaderboard.size()&&i<10;i++){
        Color c = (i==0)?GOLD:((i==1)?LIGHTGRAY:((i==2)?ORANGE:WHITE));
        DrawText(TextFormat("%d",i+1),160,y,20,c);
        DrawText(leaderboard[i].name.c_str(),300,y,20,c);
        DrawText(TextFormat("%d",leaderboard[i].score),550,y,20,c);
        DrawText(leaderboard[i].date.c_str(),650,y,18,GRAY);
        y+=35;
    }
    DrawText("PRESS ESC OR R TO RETURN", SW/2-140, SH-50, 20, GRAY);
}

void Game::DrawNameInput() {
    DrawRectangle(SW/2-150, SH/2+30, 300, 40, DARKGRAY);
    DrawRectangleLines(SW/2-150, SH/2+30, 300, 40, WHITE);
    std::string disp = playerName.empty() ? "ENTER YOUR NAME" : playerName;
    DrawText(disp.c_str(), SW/2-140, SH/2+40, 20, playerName.empty()?GRAY:WHITE);
    if ((int)(GetTime()*2)%2==0) { int w = MeasureText(disp.c_str(),20); DrawRectangle(SW/2-140+w, SH/2+40, 2, 20, WHITE); }
    DrawText("PRESS ENTER TO SAVE", SW/2-110, SH/2+80, 18, GRAY);
    int key = GetCharPressed();
    while(key>0){ if(key>=32 && key<=126 && playerName.length()<20) playerName+=(char)key; key=GetCharPressed(); }
    if(IsKeyPressed(KEY_BACKSPACE) && !playerName.empty()) playerName.pop_back();
}

void Game::DrawDebugInfo() {
    DrawText("DEBUG MODE", 10, SH-30, 20, YELLOW);
    DrawText(TextFormat("God: %s", godMode?"ON":"OFF"), 10, SH-55, 16, godMode?GREEN:RED);
    DrawText(TextFormat("Level: %d/3", currentLevel), 10, SH-80, 16, WHITE);
    DrawText(TextFormat("Speed: (%.1f,%.1f)", ball.GetSpeed().x, ball.GetSpeed().y), 10, SH-105, 16, WHITE);
    DrawText(TextFormat("Score: %d", score), 10, SH-130, 16, WHITE);
    DrawText(TextFormat("Lives: %d", lives), 10, SH-155, 16, WHITE);
}

void Game::Shutdown() { SaveLeaderboard(); }
