// include/Brick.h
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
    
private:
    Rectangle rect;
    bool active;
};

#endif