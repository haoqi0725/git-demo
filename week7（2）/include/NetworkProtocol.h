#pragma once
#include <stdint.h>

#pragma pack(push, 1) // 强制对齐，防止跨平台错位

// 客户端发给服务器的数据 (玩家的板位置)
struct ClientInput {
    float paddleX;
    bool isFiring; // 比如发射子弹
};

// 服务器广播给所有客户端的数据 (游戏全局状态)
struct GameStateUpdate {
    float ballX, ballY;
    float paddle1X, paddle2X;
    int score1, score2;       // 双方分数
    int lives1, lives2;       // 双方生命
    int brickCount;           // 剩余砖块数
    int powerUpCount;         // 道具数量
};

#pragma pack(pop)