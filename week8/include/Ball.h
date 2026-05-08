#ifndef BALL_H
#define BALL_H

#include "raylib.h"

enum class CollisionSide { NONE, TOP, BOTTOM, LEFT, RIGHT };

class Ball {
public:
    Ball(Vector2 pos, Vector2 spd, float rad);
    
    void Move(float dt);
    void Draw();
    void Reset(Vector2 pos, Vector2 spd);
    
    void BounceEdge(int screenWidth, int screenHeight);
    void CheckPaddleCollision(Rectangle paddleRect);
    CollisionSide CheckBrickCollision(Rectangle brickRect);
    
    void ReverseX() { speed.x *= -1; }
    void ReverseY() { speed.y *= -1; }
    
    Vector2 GetPosition() const { return position; }
    Vector2 GetSpeed() const { return speed; }
    void SetRadius(float r); 
    float GetRadius() const { return radius; }
    void SetSpeed(Vector2 newSpeed) { speed = newSpeed; }
    void SetPosition(Vector2 pos) { position = pos; }
    
    void SetAttached(bool attached) { isAttached = attached; }
    bool IsAttached() const { return isAttached; }
    
    // 1. 增加设置火球状态的函数声明
    void SetFire(float duration);
    
    // 2. 增加检查是否是火球的函数
    bool IsFireball() const { return isFireball; }
    void ClearFire() { isFireball = false; fireTimer = 0.0f; }
    // 3. 在 Update 函数中需要更新计时器（逻辑在 .cpp 里实现）
    void Update(float dt);

private:
    Vector2 position;
    Vector2 speed;
    float radius;
     bool isAttached = true; 
    bool isFireball = false;
    float fireTimer = 0.0f;
};

#endif
