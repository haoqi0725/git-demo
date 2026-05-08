#include "../include/Brick.h"

Brick::Brick(float x, float y, float w, float h)
    : rect({x, y, w, h}), active(true), hitThisFrame(false), color(GREEN) {}

void Brick::Draw() {
    if (active) {
        DrawRectangleRec(rect, color);
        DrawRectangleLines(rect.x, rect.y, rect.width, rect.height, DARKGREEN);
    }
}

void Brick::SetActive(bool active) {
    this->active = active;
}

bool Brick::CheckCollision(Rectangle other) const {
    return active && CheckCollisionRecs(rect, other);
}