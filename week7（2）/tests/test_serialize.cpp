#include <iostream>
#include <cstring>
#include "../include/NetworkProtocol.h"

int main() {
    // 1. 测试结构体大小是否正确
    size_t size = sizeof(GameStateUpdate);
    std::cout << "sizeof(GameStateUpdate) = " << size << " bytes" << std::endl;
    
    // 2. 创建测试数据
    GameStateUpdate original;
    original.ballX = 100.5f;
    original.ballY = 200.3f;
    original.paddle1X = 350.0f;
    original.paddle2X = 400.0f;
    original.score1 = 10;
    original.score2 = 20;
    original.lives1 = 3;
    original.lives2 = 2;
    original.brickCount = 30;
    original.powerUpCount = 1;
    
    // 3. 序列化：结构体转字节数组
    unsigned char buffer[sizeof(GameStateUpdate)];
    memcpy(buffer, &original, sizeof(GameStateUpdate));
    
    // 4. 反序列化：字节数组转结构体
    GameStateUpdate received;
    memcpy(&received, buffer, sizeof(GameStateUpdate));
    
    // 5. 验证数据正确
    bool pass = true;
    if (received.ballX != 100.5f) { std::cout << "FAIL: ballX" << std::endl; pass = false; }
    if (received.ballY != 200.3f) { std::cout << "FAIL: ballY" << std::endl; pass = false; }
    if (received.paddle1X != 350.0f) { std::cout << "FAIL: paddle1X" << std::endl; pass = false; }
    if (received.paddle2X != 400.0f) { std::cout << "FAIL: paddle2X" << std::endl; pass = false; }
    if (received.score1 != 10) { std::cout << "FAIL: score1" << std::endl; pass = false; }
    if (received.score2 != 20) { std::cout << "FAIL: score2" << std::endl; pass = false; }
    if (received.lives1 != 3) { std::cout << "FAIL: lives1" << std::endl; pass = false; }
    if (received.lives2 != 2) { std::cout << "FAIL: lives2" << std::endl; pass = false; }
    if (received.brickCount != 30) { std::cout << "FAIL: brickCount" << std::endl; pass = false; }
    if (received.powerUpCount != 1) { std::cout << "FAIL: powerUpCount" << std::endl; pass = false; }
    
    if (pass) {
        std::cout << "ALL TESTS PASSED!" << std::endl;
    }
    
    return pass ? 0 : 1;
}
