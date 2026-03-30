// src/Paddle.cpp
#include "Paddle.h"

Paddle::Paddle(float x, float y, float w, float h) {
    rect = {x, y, w, h};
    speed = 8.0f;
}

void Paddle::MoveLeft(float moveSpeed) {
    rect.x -= (moveSpeed > 0 ? moveSpeed : speed);
    if (rect.x < 5) rect.x = 5;
}

void Paddle::MoveRight(float moveSpeed) {
    rect.x += (moveSpeed > 0 ? moveSpeed : speed);
    if (rect.x + rect.width > GetScreenWidth() - 5) {
        rect.x = GetScreenWidth() - rect.width - 5;
    }
}

void Paddle::Draw() {
    DrawRectangleRec(rect, BLUE);
}