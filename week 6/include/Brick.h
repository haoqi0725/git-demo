#ifndef BRICK_H
#define BRICK_H

#include "raylib.h"

class Brick {
public:
    Brick(float x, float y, float w, float h);
    
    void Draw();
    void SetActive(bool active);
    bool IsActive() const { return active; }
    bool CheckCollision(Rectangle other) const;
    Rectangle GetRect() const { return rect; }
    Color GetColor() const { return color; }
    
    // ========== 新增：用于多球碰撞去重 ==========
    void SetHitThisFrame(bool hit) { hitThisFrame = hit; }
    bool HitThisFrame() const { return hitThisFrame; }
    
private:
    Rectangle rect;
    bool active;
    bool hitThisFrame;      // 新增：标记本帧是否已被击中
    Color color;
};

#endif