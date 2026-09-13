#pragma once

#include "cocos2d.h"

class Player final : public cocos2d::Node {
public:
    static Player* create();
    bool init() override;

    void setMoveAxis(float axis);
    void setJetpackActive(bool active);
    void setAimDirection(const cocos2d::Vec2& direction);

    void simulate(float dt, float groundY, float minX, float maxX);
    void updateLifecycle(float dt, const cocos2d::Vec2& respawnPosition);

    bool takeDamage(float amount);

    void applyNetworkState(const cocos2d::Vec2& position,
                           const cocos2d::Vec2& velocity,
                           const cocos2d::Vec2& aimDirection,
                           float health,
                           bool alive,
                           float motionBlend = 1.0f);

    const cocos2d::Vec2& aimDirection() const { return aimDirection_; }
    const cocos2d::Vec2& velocity() const { return velocity_; }
    float health() const { return health_; }
    float maxHealth() const { return maxHealth_; }
    bool isAlive() const { return alive_; }
    float hitRadius() const { return hitRadius_; }

private:
    cocos2d::Vec2 velocity_{0.0f, 0.0f};
    cocos2d::Vec2 aimDirection_{1.0f, 0.0f};
    cocos2d::Node* weaponPivot_{nullptr};

    float moveAxis_{0.0f};
    bool jetpackActive_{false};

    float health_{100.0f};
    float maxHealth_{100.0f};
    bool alive_{true};
    float respawnTimer_{0.0f};
    float respawnDelay_{2.0f};
    float hitRadius_{27.0f};

    float moveAcceleration_{1050.0f};
    float maxMoveSpeed_{260.0f};
    float gravity_{-980.0f};
    float jetpackAcceleration_{1450.0f};
    float maxFallSpeed_{-650.0f};
};
