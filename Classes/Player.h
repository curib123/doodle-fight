#pragma once

#include "cocos2d.h"

class Player final : public cocos2d::Node {
public:
    static Player* create();
    bool init() override;

    void setMoveAxis(float axis);
    void setJetpackActive(bool active);
    void simulate(float dt, float groundY, float minX, float maxX);

private:
    cocos2d::Vec2 velocity_{0.0f, 0.0f};
    float moveAxis_{0.0f};
    bool jetpackActive_{false};

    float moveAcceleration_{1050.0f};
    float maxMoveSpeed_{260.0f};
    float gravity_{-980.0f};
    float jetpackAcceleration_{1450.0f};
    float maxFallSpeed_{-650.0f};
};
