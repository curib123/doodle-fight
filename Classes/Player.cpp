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

    // Original vector mascot: a tiny cheerful sky-explorer, not copied art.
    auto* art = DrawNode::create();

    // Jetpack pods behind the body.
    art->drawSolidCircle(Vec2(-22, -2), 9.0f, 0.0f, 24, Color4F(0.67f, 0.38f, 0.95f, 1.0f));
    art->drawSolidCircle(Vec2(22, -2), 9.0f, 0.0f, 24, Color4F(0.67f, 0.38f, 0.95f, 1.0f));
    art->drawSolidCircle(Vec2(-22, -4), 4.0f, 0.0f, 18, Color4F(0.35f, 0.95f, 1.0f, 1.0f));
    art->drawSolidCircle(Vec2(22, -4), 4.0f, 0.0f, 18, Color4F(0.35f, 0.95f, 1.0f, 1.0f));

    // Body and oversized friendly head.
    art->drawSolidRect(Vec2(-16, -17), Vec2(16, 13), Color4F(0.28f, 0.72f, 1.0f, 1.0f));
    art->drawSolidCircle(Vec2(0, 20), 23.0f, 0.0f, 42, Color4F(1.0f, 0.84f, 0.38f, 1.0f));

    // Hair/helmet tuft and face.
    art->drawTriangle(Vec2(-12, 39), Vec2(0, 50), Vec2(8, 39), Color4F(0.98f, 0.42f, 0.58f, 1.0f));
    art->drawSolidCircle(Vec2(-8, 24), 3.5f, 0.0f, 16, Color4F::BLACK);
    art->drawSolidCircle(Vec2(8, 24), 3.5f, 0.0f, 16, Color4F::BLACK);
    art->drawLine(Vec2(-5, 13), Vec2(5, 13), Color4F(0.55f, 0.22f, 0.25f, 1.0f));

    addChild(art, 1);

    // The blaster pivots independently so the character can aim in 360 degrees.
    weaponPivot_ = Node::create();
    weaponPivot_->setPosition(Vec2(0, 4));

    auto* blaster = DrawNode::create();
    blaster->drawSolidRect(Vec2(8, -5), Vec2(38, 5), Color4F(0.96f, 0.40f, 0.62f, 1.0f));
    blaster->drawSolidCircle(Vec2(39, 0), 6.0f, 0.0f, 20, Color4F(1.0f, 0.86f, 0.28f, 1.0f));
    blaster->drawSolidRect(Vec2(15, -12), Vec2(22, -4), Color4F(0.24f, 0.28f, 0.48f, 1.0f));
    weaponPivot_->addChild(blaster);
    addChild(weaponPivot_, 2);

    return true;
}

void Player::setMoveAxis(float axis) {
    moveAxis_ = std::clamp(axis, -1.0f, 1.0f);
}

void Player::setJetpackActive(bool active) {
    jetpackActive_ = active;
}

void Player::setAimDirection(const Vec2& direction) {
    if (direction.lengthSquared() < 0.0001f) {
        return;
    }

    aimDirection_ = direction.getNormalized();
    const float angle = CC_RADIANS_TO_DEGREES(std::atan2(aimDirection_.y, aimDirection_.x));
    if (weaponPivot_) {
        weaponPivot_->setRotation(-angle);
    }
}

void Player::simulate(float dt, float groundY, float minX, float maxX) {
    if (!alive_) {
        return;
    }

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

void Player::updateLifecycle(float dt, const Vec2& respawnPosition) {
    if (alive_) {
        return;
    }

    respawnTimer_ -= dt;
    if (respawnTimer_ > 0.0f) {
        return;
    }

    alive_ = true;
    health_ = maxHealth_;
    velocity_ = Vec2::ZERO;
    setPosition(respawnPosition);
    setVisible(true);
}

bool Player::takeDamage(float amount) {
    if (!alive_ || amount <= 0.0f) {
        return false;
    }

    health_ = std::max(0.0f, health_ - amount);
    if (health_ > 0.0f) {
        return false;
    }

    alive_ = false;
    respawnTimer_ = respawnDelay_;
    velocity_ = Vec2::ZERO;
    setVisible(false);
    return true;
}
