// main.cpp
#include "raylib.h"
#include "../include/Game.h"
#include <iostream>
#include <cstring>

int main(int argc, char* argv[]) {
    try {
        // ========== 检查命令行参数 ==========
        bool benchmarkMode = false;
        int benchmarkFrames = 600;  // 默认600帧
        
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--benchmark") == 0) {
                benchmarkMode = true;
                // 检查是否有指定帧数
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    benchmarkFrames = std::atoi(argv[i + 1]);
                    i++;  // 跳过帧数参数
                }
            }
            else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                std::cout << "Usage: ./breakout [OPTIONS]" << std::endl;
                std::cout << "Options:" << std::endl;
                std::cout << "  --benchmark [frames]  Run benchmark mode (default: 600 frames)" << std::endl;
                std::cout << "  --help, -h           Show this help message" << std::endl;
                return 0;
            }
        }
        
        // ========== 基准测试模式 ==========
        if (benchmarkMode) {
            std::cout << "Starting in BENCHMARK mode..." << std::endl;
            std::cout << "Running " << benchmarkFrames << " frames..." << std::endl;
            
            InitWindow(800, 600, "Benchmark - ARKANOID 2D");
            SetTargetFPS(0);  // 不限制帧率
            // 创建游戏实例
            Game game;
            game.Init();
            
            // 运行基准测试（不显示窗口）
            game.RunBenchmark(benchmarkFrames);
            
            std::cout << "Benchmark complete. Exiting." << std::endl;
            return 0;
        }
        
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