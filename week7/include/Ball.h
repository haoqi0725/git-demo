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
    
    Vector2 GetPosition() const { return position; }
    Vector2 GetSpeed() const { return speed; }
    float GetRadius() const { return radius; }
    void SetSpeed(Vector2 newSpeed) { speed = newSpeed; }
    void SetPosition(Vector2 pos) { position = pos; }
    
private:
    Vector2 position;
    Vector2 speed;
    float radius;
};

#endif
