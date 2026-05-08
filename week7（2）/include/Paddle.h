// Paddle.h
#ifndef PADDLE_H
#define PADDLE_H

#include "raylib.h"

class Paddle {
private:
    Vector2 position;
    float width;
    float height;
    float speed;           // 移动速度（像素/秒）
    
    // 道具效果相关
    float originalWidth;   // 原始宽度
    float effectRemainingTime; // 剩余效果时间（秒）

public:
    Paddle(float x, float y, float w, float h, float spd );
    
    void MoveLeft(float deltaTime);
    void MoveRight(float deltaTime);
    void ClampToScreen(int screenWidth);
    
    Rectangle GetRect() const;
    void Draw() const;
    
    // 道具效果：加长挡板
    void Extend(float extraWidth, float duration);
    
    // 更新效果计时（每帧调用）
    void Update(float deltaTime);
    
    // 重置到原始宽度（手动调用，或由 Update 自动恢复）
    void ResetWidth();
    
    // Getter/Setter
    float GetWidth() const { return width; }
    float GetHeight() const { return height; }
    Vector2 GetPosition() const { return position; }
    void SetPosition(float x, float y) { position = {x, y}; }
    void SetX(float newX) { position.x = newX; }
};

#endif