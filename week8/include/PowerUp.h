#ifndef POWERUP_H
#define POWERUP_H

#include "raylib.h"
#include <memory>

class Game;

class PowerUpEffect {
public:
    virtual ~PowerUpEffect() = default;
    virtual void Apply(Game& game) = 0;
};

class ExtendPaddleEffect : public PowerUpEffect {
    float extraWidth, duration;
public:
    ExtendPaddleEffect(float w, float d) : extraWidth(w), duration(d) {}
    void Apply(Game& game) override;
};

class MultiBallEffect : public PowerUpEffect {
    int extraCount;
public:
    MultiBallEffect(int count) : extraCount(count) {}
    void Apply(Game& game) override;
};

class SlowBallEffect : public PowerUpEffect {
    float factor, duration;
public:
    SlowBallEffect(float f, float d) : factor(f), duration(d) {}
    void Apply(Game& game) override;
};

class FireBallEffect : public PowerUpEffect {
    float duration;
public:
    FireBallEffect(float d) : duration(d) {}
    void Apply(Game& game) override;
};


enum class PowerUpType { PADDLE_EXTEND, MULTI_BALL, SLOW_BALL, FIRE_BALL };

class PowerUp {
public:
    PowerUp(float x, float y, PowerUpType type);
    void Update(float dt);
    void Draw() const;
    void Apply(Game& game);
    void Deactivate() { active = false; }
    bool IsActive() const { return active; }
    Rectangle GetRect() const;
    
private:
    Vector2 position;
    PowerUpType type;
    bool active;
    float fallSpeed;
    float radius;
    std::unique_ptr<PowerUpEffect> effect;
    void createEffect();
};

#endif
