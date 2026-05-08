#include "raylib.h"
#include "../include/Game.h"
#include <enet/enet.h>
#include <iostream>
#include <cstring>

int main(int argc, char* argv[]) {
    // ===== 1. 解析命令行参数 =====
    bool isServer = true;  // 默认主机
    
    if (argc > 1) {
        if (strcmp(argv[1], "client") == 0 || strcmp(argv[1], "c") == 0) {
            isServer = false;
        }
    } else {
        std::cout << "用法: ./breakout [host|client]" << std::endl;
        std::cout << "  host   - 主机模式 (默认)" << std::endl;
        std::cout << "  client - 客户端模式" << std::endl;
        std::cout << "使用默认主机模式启动..." << std::endl;
    }

    // ===== 2. 初始化 ENet =====
    if (enet_initialize() != 0) {
        std::cerr << "ENet 初始化失败!" << std::endl;
        return -1;
    }

    // ===== 3. 初始化窗口 =====
    InitWindow(800, 600, isServer ? "Breakout - 主机" : "Breakout - 客户端");
    InitAudioDevice();
    SetTargetFPS(60);

    // ===== 4. 初始化游戏 =====
    Game game;
    game.Init(isServer);

    // ===== 5. 主循环 =====
    while (!WindowShouldClose()) {
        game.Update();

        BeginDrawing();
        ClearBackground(BLACK);
        game.Draw();
        EndDrawing();
    }

    // ===== 6. 清理 =====
    game.Shutdown();
    CloseAudioDevice();
    CloseWindow();
    enet_deinitialize();

    return 0;
}