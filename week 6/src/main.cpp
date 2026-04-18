#include "raylib.h"
#include "../include/Game.h"

int main() {
    // 创建窗口
    InitWindow(800, 600, "Breakout");
    InitAudioDevice();
    SetTargetFPS(60);
    
    // 创建游戏对象
    Game game;
    game.Init();
    
    // 主循环
    while (!WindowShouldClose()) {
        game.Update();
        
        BeginDrawing();
        ClearBackground(BLACK);
        game.Draw();
        EndDrawing();
    }
    
    // 清理
    game.Shutdown();
    CloseAudioDevice();
    CloseWindow();
    
    return 0;
}
