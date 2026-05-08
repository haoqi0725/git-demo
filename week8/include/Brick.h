#ifndef BRICK_H
#define BRICK_H

#include "raylib.h"

class Brick {
public:
    // 构造函数保持不变，传入的 level 会存入私有变量 brickLevel
   Brick(float x, float y, float w, float h, Color color, int points,  int hp, int level);
    
    void Draw();
    bool Hit(); 
    
    bool IsActive() const { return active; }
    Rectangle GetRect() const { return rect; }
    Color GetColor() const { return color; }
    int GetHealth() const { return health; }
    
    void SetHitThisFrame(bool hit) { hitThisFrame = hit; }
    bool HitThisFrame() const { return hitThisFrame; }
    void Destroy() {
        health = 0;
        active = false;
    }
    void SetHealth(int h) { 
        health = h; 
        if (health <= 0) {
            active = false;
        }
        UpdateColor(); 
    }
private:
    // 内部私有函数，用于根据血量和关卡刷新 color 变量
    void UpdateColor(); 

    Rectangle rect;
    bool active;
    bool hitThisFrame = false;
    int health;           // 当前血量（统一使用此变量）
    int maxHealth;        // 最大血量
    int brickLevel;       // 关卡编号
    Color color;
    int points;           // 分数
};

#endif