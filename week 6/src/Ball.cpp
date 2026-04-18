// src/Ball.cpp
#include "Ball.h"
#include <cmath>

Ball::Ball(Vector2 pos, Vector2 spd, float rad)
    : position(pos), speed(spd), radius(rad) {}

void Ball::Move() {
    position.x += speed.x;
    position.y += speed.y;
}

void Ball::Draw() {
    DrawCircleV(position, radius, WHITE);
}

void Ball::Reset(Vector2 pos, Vector2 spd) {
    position = pos;
    speed = spd;
}

void Ball::BounceEdge(int screenWidth, int screenHeight) {
    // 左右边界
    if (position.x - radius <= 0) {
        position.x = radius;
        speed.x *= -1;
    }
    if (position.x + radius >= screenWidth) {
        position.x = screenWidth - radius;
        speed.x *= -1;
    }
    
    // 上边界
    if (position.y - radius <= 0) {
        position.y = radius;
        speed.y *= -1;
    }
    // 下边界不处理，由主循环处理落底
}

void Ball::CheckPaddleCollision(Rectangle paddleRect) {
    if (CheckCollisionCircleRec(position, radius, paddleRect)) {
        // 确保球在挡板正上方再反弹
        if (position.y + radius >= paddleRect.y && speed.y > 0) {
            position.y = paddleRect.y - radius;
            speed.y *= -1;
            
            // 根据碰撞点偏移改变水平速度（增加趣味性）
            float hitPos = (position.x - paddleRect.x) / paddleRect.width;
            float angle = (hitPos - 0.5f) * 1.5f;  // -0.75 到 0.75 弧度
            float newSpeedX = 6.0f * angle;
            float newSpeedY = -sqrt(36.0f - newSpeedX * newSpeedX);
            speed = {newSpeedX, newSpeedY};
        }
    }
}

CollisionSide Ball::CheckBrickCollision(Rectangle brickRect) {
    if (!CheckCollisionCircleRec(position, radius, brickRect)) {
        return CollisionSide::NONE;
    }

    float ballCenterX = position.x;
    float ballCenterY = position.y;

    float brickCenterX = brickRect.x + brickRect.width  / 2.0f;
    float brickCenterY = brickRect.y + brickRect.height / 2.0f;

    float dx = ballCenterX - brickCenterX;
    float dy = ballCenterY - brickCenterY;

    float overlapX = (brickRect.width  / 2.0f + radius) - fabs(dx);
    float overlapY = (brickRect.height / 2.0f + radius) - fabs(dy);

    if (overlapX < overlapY) {
        return (dx > 0) ? CollisionSide::RIGHT : CollisionSide::LEFT;
    } else {
        return (dy > 0) ? CollisionSide::BOTTOM : CollisionSide::TOP;
    }
}