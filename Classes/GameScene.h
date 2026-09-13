#pragma once

#include "cocos2d.h"
#include "Combat/Weapon.h"

#include <vector>

class Player;
class Projectile;

class GameScene final : public cocos2d::Scene {
public:
    CREATE_FUNC(GameScene);
    bool init() override;
    void update(float dt) override;

private:
    void setupArena();
    void setupInput();
    void setupHud();

    void fireProjectile();
    void updateProjectiles(float dt);
    void updateHud();

    Player* player_{nullptr};
    Player* target_{nullptr};
    std::vector<Projectile*> projectiles_;

    Weapon weapon_{};

    cocos2d::Vec2 aimWorld_{700.0f, 320.0f};
    cocos2d::Vec2 playerSpawn_{240.0f, 90.0f};
    cocos2d::Vec2 targetSpawn_{760.0f, 90.0f};

    cocos2d::DrawNode* reticle_{nullptr};
    cocos2d::Label* ammoLabel_{nullptr};
    cocos2d::Label* targetHealthLabel_{nullptr};
    cocos2d::Label* koLabel_{nullptr};

    bool moveLeft_{false};
    bool moveRight_{false};
    bool jetpack_{false};
    bool firing_{false};

    int koCount_{0};
    float groundY_{90.0f};
};
