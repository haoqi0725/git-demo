#ifndef BALL_H
#define BALL_H

#include "raylib.h"

enum class CollisionSide { NONE, TOP, BOTTOM, LEFT, RIGHT };

class Ball {
public:
    // 构造函数
    Ball(Vector2 pos, Vector2 spd, float rad);
    
    // 核心游戏逻辑
    void Move(float dt);
    void Draw();
    void Update(float dt);
    void Reset(Vector2 pos, Vector2 spd);
    
    // 碰撞检测
    void BounceEdge(int screenWidth, int screenHeight);
    void CheckPaddleCollision(Rectangle paddleRect);
    CollisionSide CheckBrickCollision(Rectangle brickRect);
    
    // 反弹辅助
    void ReverseX() { speed.x *= -1; }
    void ReverseY() { speed.y *= -1; }
    
    // ========== Getter 方法 ==========
    Vector2 GetPosition() const { return position; }
    Vector2 GetSpeed() const { return speed; }
    float GetRadius() const { return radius; }
    
    // ========== Setter 方法 ==========
    void SetRadius(float r);
    void SetSpeed(Vector2 newSpeed) { speed = newSpeed; }
    void SetPosition(Vector2 pos) { position = pos; }
    void SetAttached(bool attached) { isAttached = attached; }
    
    // ========== 状态查询 ==========
    bool IsAttached() const { return isAttached; }
    bool IsFireball() const { return isFireball; }
    
    // ========== 火球相关 ==========
    void SetFire(float duration);
    void ClearFire() { isFireball = false; fireTimer = 0.0f; }
    float GetFireTimer() const { return fireTimer; }  // ✅ 添加在这里

private:
    Vector2 position;
    Vector2 speed;
    float radius;
    bool isAttached = true; 
    bool isFireball = false;
    float fireTimer = 0.0f;
};
#endif