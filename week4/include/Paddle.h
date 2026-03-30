// include/Paddle.h
#ifndef PADDLE_H
#define PADDLE_H

#include "raylib.h"

class Paddle {
public:
    Paddle(float x, float y, float w, float h);
    
    void MoveLeft(float speed);
    void MoveRight(float speed);
    void Draw();
    
    Rectangle GetRect() const { return rect; }
    
private:
    Rectangle rect;
    float speed;
};

#endif