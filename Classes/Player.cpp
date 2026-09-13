#include "Player.h"
#include <algorithm>
#include <cmath>

USING_NS_CC;

Player* Player::create() {
    auto* player = new (std::nothrow) Player();
    if (player && player->init()) {
        player->autorelease();
        return player;
    }
    CC_SAFE_DELETE(player);
    return nullptr;
}

bool Player::init() {
    if (!Node::init()) {
        return false;
    }

    // Temporary original vector mascot: no copied sprites/assets.
    auto* art = DrawNode::create();
    art->drawSolidCircle(Vec2(0, 18), 22.0f, 0.0f, 40, Color4F(1.0f, 0.82f, 0.32f, 1.0f));
    art->drawSolidRect(Vec2(-17, -16), Vec2(17, 17), Color4F(0.33f, 0.73f, 1.0f, 1.0f));
    art->drawSolidCircle(Vec2(-8, 23), 3.5f, 0.0f, 16, Color4F::BLACK);
    art->drawSolidCircle(Vec2(8, 23), 3.5f, 0.0f, 16, Color4F::BLACK);
    art->drawSolidRect(Vec2(-24, -7), Vec2(-17, 12), Color4F(0.65f, 0.38f, 0.95f, 1.0f));
    art->drawSolidRect(Vec2(17, -7), Vec2(24, 12), Color4F(0.65f, 0.38f, 0.95f, 1.0f));
    addChild(art);

    return true;
}

void Player::setMoveAxis(float axis) {
    moveAxis_ = std::clamp(axis, -1.0f, 1.0f);
}

void Player::setJetpackActive(bool active) {
    jetpackActive_ = active;
}

void Player::simulate(float dt, float groundY, float minX, float maxX) {
    velocity_.x += moveAxis_ * moveAcceleration_ * dt;

    if (std::abs(moveAxis_) < 0.01f) {
        velocity_.x *= std::pow(0.001f, dt);
    }

    velocity_.x = std::clamp(velocity_.x, -maxMoveSpeed_, maxMoveSpeed_);
    velocity_.y += gravity_ * dt;

    if (jetpackActive_) {
        velocity_.y += jetpackAcceleration_ * dt;
    }

    velocity_.y = std::max(velocity_.y, maxFallSpeed_);

    Vec2 next = getPosition() + velocity_ * dt;
    next.x = std::clamp(next.x, minX, maxX);

    if (next.y < groundY) {
        next.y = groundY;
        if (velocity_.y < 0.0f) {
            velocity_.y = 0.0f;
        }
    }

    setPosition(next);
}
