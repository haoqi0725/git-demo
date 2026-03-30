// include/Ball.h
#ifndef BALL_H
#define BALL_H

#include "raylib.h"

enum class CollisionSide { NONE, TOP, BOTTOM, LEFT, RIGHT };

class Ball {
public:
    Ball(Vector2 pos, Vector2 spd, float rad);
    
    void Move();
    void Draw();
    void Reset(Vector2 pos, Vector2 spd);
    
    void BounceEdge(int screenWidth, int screenHeight);
    void CheckPaddleCollision(Rectangle paddleRect);
    CollisionSide CheckBrickCollision(Rectangle brickRect);
    
    void ReverseX() { speed.x *= -1; }
    void ReverseY() { speed.y *= -1; }
    
    // Getter 方法
    Vector2 GetPosition() const { return position; }
    Vector2 GetSpeed() const { return speed; }   // ← 添加这行
    float GetRadius() const { return radius; }
    
private:
    Vector2 position;
    Vector2 speed;
    float radius;
};

#endif
