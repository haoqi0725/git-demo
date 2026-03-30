// src/Brick.cpp
#include "Brick.h"

Brick::Brick(float x, float y, float w, float h) {
    rect = {x, y, w, h};
    active = true;
}

void Brick::Draw() {
    if (active) {
        DrawRectangleRec(rect, RED);
        DrawRectangleLinesEx(rect, 1, MAROON);
    }
}

void Brick::SetActive(bool active) {
    this->active = active;
}

bool Brick::CheckCollision(Rectangle other) const {
    if (!active) return false;
    return CheckCollisionRecs(rect, other);
}