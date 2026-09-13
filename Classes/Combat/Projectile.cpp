#include "Combat/Projectile.h"

#include <cmath>
#include <new>

USING_NS_CC;

Projectile* Projectile::create(const Vec2& direction,
                               float speed,
                               float damage,
                               float lifetime) {
    auto* projectile = new (std::nothrow) Projectile();
    if (projectile && projectile->initWithData(direction, speed, damage, lifetime)) {
        projectile->autorelease();
        return projectile;
    }

    CC_SAFE_DELETE(projectile);
    return nullptr;
}

bool Projectile::initWithData(const Vec2& direction,
                              float speed,
                              float damage,
                              float lifetime) {
    if (!Node::init()) {
        return false;
    }

    auto dir = direction.getNormalized();
    if (dir.isZero()) {
        dir = Vec2::UNIT_X;
    }

    velocity_ = dir * speed;
    damage_ = damage;
    lifetime_ = lifetime;

    auto* glow = DrawNode::create();
    glow->drawSolidCircle(Vec2::ZERO, 7.0f, 0.0f, 20, Color4F(1.0f, 0.94f, 0.35f, 0.95f));
    glow->drawCircle(Vec2::ZERO, 10.0f, 0.0f, 20, false, Color4F(1.0f, 0.55f, 0.25f, 0.65f));
    addChild(glow);

    const float angle = CC_RADIANS_TO_DEGREES(std::atan2(dir.y, dir.x));
    setRotation(-angle);

    return true;
}

void Projectile::simulate(float dt) {
    setPosition(getPosition() + velocity_ * dt);
    lifetime_ -= dt;
}
