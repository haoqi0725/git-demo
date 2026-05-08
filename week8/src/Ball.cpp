// src/Ball.cpp
#include "Ball.h"
#include <cmath>

Ball::Ball(Vector2 pos, Vector2 spd, float rad)
    : position(pos), speed(spd), radius(rad), isAttached(true) {}

void Ball::Move(float dt) {
    position.x += speed.x * dt;
    position.y += speed.y * dt;
}

void Ball::Draw() {
    if (isFireball) {
        // 计算尾迹位置，假设变量名为 speed
        // 如果编译还是报错，请看下方的方案 B
        Vector2 trailPos = { 
            position.x - (speed.x * 0.05f), 
            position.y - (speed.y * 0.05f) 
        };

        DrawCircleV(trailPos, radius * 0.8f, RED);    // 红色尾迹
        DrawCircleV(position, radius + 2.0f, RED);    // 核心发光
        DrawCircleV(position, radius, ORANGE);        // 核心颜色
        DrawCircleV(position, radius * 0.5f, YELLOW); // 中心高亮
    } else {
        DrawCircleV(position, radius, WHITE);
    }
}

void Ball::Reset(Vector2 pos, Vector2 spd) {
    position = pos;
    speed = spd;
}

void Ball::SetRadius(float r) {
    radius = r;
}

void Ball::BounceEdge(int screenWidth, int /*screenHeight*/) {
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

// CheckPaddleCollision 中删除未使用的 maxBounceAngle
void Ball::CheckPaddleCollision(Rectangle paddleRect) {
    if (!CheckCollisionCircleRec(position, radius, paddleRect))
        return;

    if (position.y + radius >= paddleRect.y && speed.y > 0) {
        // 防止卡进挡板
        position.y = paddleRect.y - radius;

        // 计算碰撞位置
        float hitPos = (position.x - paddleRect.x) / paddleRect.width;

        // 限制范围
        if (hitPos < 0.0f) hitPos = 0.0f;
        if (hitPos > 1.0f) hitPos = 1.0f;

        float angle = (hitPos - 0.5f) * 1.0f;

        // 保持总速度
        float totalSpeed = sqrtf(speed.x * speed.x + speed.y * speed.y);

        speed.x = totalSpeed * sinf(angle);
        speed.y = -totalSpeed * cosf(angle);
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

// 1. 实现 SetFire 函数
void Ball::SetFire(float duration) {
    isFireball = true;
    fireTimer = duration;
}

void Ball::Update(float dt) {
    
    // 计时器递减
    if (isFireball) {
        fireTimer -= dt;
        if (fireTimer <= 0) {
            isFireball = false;
            fireTimer = 0;
        }
    }
}