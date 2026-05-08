// Paddle.cpp
#include "../include/Paddle.h"
#include <algorithm>

Paddle::Paddle(float x, float y, float w, float h, float spd)
    : position({x, y}), width(w), height(h), speed(spd),
      originalWidth(w), effectRemainingTime(0.0f)
{
}

void Paddle::MoveLeft(float deltaTime) {
    position.x -= speed * deltaTime;
}

void Paddle::MoveRight(float deltaTime) {
    position.x += speed * deltaTime;
}

void Paddle::ClampToScreen(int screenWidth) {
    if (position.x < 0) position.x = 0;
    if (position.x + width > screenWidth) position.x = screenWidth - width;
}

Rectangle Paddle::GetRect() const {
    return {position.x, position.y, width, height};
}

void Paddle::Draw() const {
    DrawRectangleV(position, {width, height}, BLUE);
    DrawRectangleLines(position.x, position.y, width, height, DARKBLUE);
}

// 加长挡板效果
void Paddle::Extend(float extraWidth, float duration) {
    // 最大宽度为原始宽度的 2 倍
    float maxWidth = originalWidth * 2.0f;
    
    float newWidth = width + extraWidth;
    if (newWidth > maxWidth) {
        newWidth = maxWidth;
    }
    width = newWidth;
    
    effectRemainingTime = duration;
}
        

void Paddle::Update(float dt) {
    if (effectRemainingTime > 0.0f) {
        effectRemainingTime -= dt;
        TraceLog(LOG_INFO, "Paddle effect remaining: %.2f seconds", effectRemainingTime);
        if (effectRemainingTime <= 0.0f) ResetWidth();
    }
}

void Paddle::ResetWidth() {
    if (width != originalWidth) {
        // 恢复原始宽度，同时调整位置保持中心不变
        float delta = width - originalWidth;
        width = originalWidth;
        // position.x += delta / 2.0f;
    }
    effectRemainingTime = 0.0f;
}