// main.cpp
#include "raylib.h"
#include "../include/Game.h"
#include <iostream>

int main() {
    try {
        // 窗口配置
        const int screenWidth = 800;
        const int screenHeight = 600;
        
        InitWindow(screenWidth, screenHeight, "ARKANOID 2D");
        
        if (!IsWindowReady()) {
            std::cerr << "Failed to create window!" << std::endl;
            return -1;
        }
        
        InitAudioDevice();
        SetTargetFPS(60);
        
        Game game;
        
        try {
            game.Init();
        } catch (const std::exception& e) {
            std::cerr << "Game initialization failed: " << e.what() << std::endl;
            CloseAudioDevice();
            CloseWindow();
            return -1;
        }
        
        // 主循环
        while (!WindowShouldClose()) {
            try {
                game.Update();
            } catch (const std::exception& e) {
                std::cerr << "Error during update: " << e.what() << std::endl;
            }
            
            BeginDrawing();
            ClearBackground(BLACK);
            
            try {
                game.Draw();
            } catch (const std::exception& e) {
                std::cerr << "Error during draw: " << e.what() << std::endl;
            }
            
            EndDrawing();
        }
        
        game.Shutdown();
        CloseAudioDevice();
        CloseWindow();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        CloseWindow();
        return -1;
    }
    
    return 0;
}