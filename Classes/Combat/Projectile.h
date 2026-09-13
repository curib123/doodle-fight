#pragma once

#include "cocos2d.h"

class Projectile final : public cocos2d::Node {
public:
    static Projectile* create(const cocos2d::Vec2& direction,
                              float speed,
                              float damage,
                              float lifetime);

    bool initWithData(const cocos2d::Vec2& direction,
                      float speed,
                      float damage,
                      float lifetime);

    void simulate(float dt);

    float damage() const { return damage_; }
    bool expired() const { return lifetime_ <= 0.0f; }
    float hitRadius() const { return hitRadius_; }

private:
    cocos2d::Vec2 velocity_{cocos2d::Vec2::ZERO};
    float damage_{0.0f};
    float lifetime_{0.0f};
    float hitRadius_{7.0f};
};
