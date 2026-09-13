#pragma once

#include "cocos2d.h"

class Player;

class GameScene final : public cocos2d::Scene {
public:
    CREATE_FUNC(GameScene);
    bool init() override;
    void update(float dt) override;

private:
    void setupArena();
    void setupInput();

    Player* player_{nullptr};
    bool moveLeft_{false};
    bool moveRight_{false};
    bool jetpack_{false};
    float groundY_{90.0f};
};
