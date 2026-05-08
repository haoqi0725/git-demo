#include <iostream>
#include "../include/PowerUp.h"
#include "../include/Game.h"
#include <cmath>

void ExtendPaddleEffect::Apply(Game& game) {
    game.GetPaddle().Extend(extraWidth, duration);
}

void MultiBallEffect::Apply(Game& game) {
    std::cout << "MultiBallEffect: adding " << extraCount << " balls" << std::endl; game.AddMultiBall(extraCount);
}

void SlowBallEffect::Apply(Game& game) {
    game.SlowDownBalls(factor, duration);
}

void PowerUp::createEffect() {
    switch (type) {
        case PowerUpType::PADDLE_EXTEND:
            effect = std::make_unique<ExtendPaddleEffect>(40.0f, 5.0f);
            break;
        case PowerUpType::MULTI_BALL:
            effect = std::make_unique<MultiBallEffect>(2);
            break;
        case PowerUpType::SLOW_BALL:
            effect = std::make_unique<SlowBallEffect>(0.7f, 5.0f);
            break;
    }
}

PowerUp::PowerUp(float x, float y, PowerUpType t)
    : position({x, y}), type(t), active(true), fallSpeed(120.0f), radius(12.0f)
{
    createEffect();
}

void PowerUp::Update(float dt) {
    if (!active) return;
    position.y += fallSpeed * dt;
    if (position.y > GetScreenHeight() + radius) active = false;
}

void PowerUp::Draw() const {
    if (!active) return;
    
    for (int i = 3; i >= 1; --i) {
        DrawCircleV(position, radius + i * 2, Fade(WHITE, 0.2f / i));
    }
    
    switch (type) {
        case PowerUpType::PADDLE_EXTEND:
            DrawRectangleV({position.x - 15, position.y - 5}, {30, 10}, BLUE);
            DrawText("+", position.x - 3, position.y - 8, 16, WHITE);
            break;
        case PowerUpType::MULTI_BALL:
            DrawCircleV(position, radius - 2, ORANGE);
            DrawCircleV({position.x - 5, position.y - 3}, 4, ORANGE);
            DrawCircleV({position.x + 5, position.y - 3}, 4, ORANGE);
            DrawText("2", position.x - 3, position.y - 8, 12, WHITE);
            break;
        case PowerUpType::SLOW_BALL:
            DrawCircleV(position, radius - 2, GREEN);
            DrawText("S", position.x - 4, position.y - 8, 16, WHITE);
            break;
    }
    DrawCircleLines(position.x, position.y, radius, WHITE);
}

void PowerUp::Apply(Game& game) {
    std::cout << "PowerUp::Apply called! type=" << (int)type << std::endl;
    std::cout << "Calling effect->Apply()" << std::endl; if (effect) effect->Apply(game);
}

Rectangle PowerUp::GetRect() const {
    return {position.x - radius, position.y - radius, radius * 2, radius * 2};
}
