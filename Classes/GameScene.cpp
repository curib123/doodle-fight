#include "GameScene.h"

#include "Combat/Projectile.h"
#include "Player.h"

#include <algorithm>
#include <cmath>

USING_NS_CC;

bool GameScene::init() {
    if (!Scene::init()) {
        return false;
    }

    setupArena();
    setupHud();
    setupInput();
    scheduleUpdate();
    return true;
}

void GameScene::setupArena() {
    const auto size = Director::getInstance()->getVisibleSize();
    const auto origin = Director::getInstance()->getVisibleOrigin();

    auto* background = LayerColor::create(Color4B(117, 205, 255, 255));
    addChild(background, -20);

    auto* scenery = DrawNode::create();

    // Warm sun and soft cloud clusters keep the prototype visually cheerful.
    scenery->drawSolidCircle(
        Vec2(origin.x + size.width - 100.0f, origin.y + size.height - 100.0f),
        48.0f,
        0.0f,
        48,
        Color4F(1.0f, 0.84f, 0.30f, 1.0f)
    );

    const Color4F cloud(1.0f, 1.0f, 1.0f, 0.92f);
    scenery->drawSolidCircle(Vec2(origin.x + 115.0f, origin.y + size.height - 120.0f), 26.0f, 0.0f, 32, cloud);
    scenery->drawSolidCircle(Vec2(origin.x + 145.0f, origin.y + size.height - 110.0f), 34.0f, 0.0f, 32, cloud);
    scenery->drawSolidCircle(Vec2(origin.x + 180.0f, origin.y + size.height - 122.0f), 25.0f, 0.0f, 32, cloud);

    scenery->drawSolidCircle(Vec2(origin.x + size.width * 0.56f, origin.y + size.height - 165.0f), 22.0f, 0.0f, 32, cloud);
    scenery->drawSolidCircle(Vec2(origin.x + size.width * 0.60f, origin.y + size.height - 154.0f), 31.0f, 0.0f, 32, cloud);
    scenery->drawSolidCircle(Vec2(origin.x + size.width * 0.64f, origin.y + size.height - 166.0f), 23.0f, 0.0f, 32, cloud);

    // Floating-island arena base.
    scenery->drawSolidRect(
        Vec2(origin.x, origin.y),
        Vec2(origin.x + size.width, groundY_ - 30.0f),
        Color4F(0.42f, 0.76f, 0.47f, 1.0f)
    );
    scenery->drawSolidRect(
        Vec2(origin.x, groundY_ - 30.0f),
        Vec2(origin.x + size.width, groundY_),
        Color4F(0.20f, 0.57f, 0.36f, 1.0f)
    );
    scenery->drawSolidRect(
        Vec2(origin.x, groundY_ - 6.0f),
        Vec2(origin.x + size.width, groundY_),
        Color4F(0.72f, 0.95f, 0.48f, 1.0f)
    );

    addChild(scenery, -10);

    auto* title = Label::createWithSystemFont("Doodle Fight - Combat Lab", "Arial", 28);
    title->setTextColor(Color4B(42, 57, 92, 255));
    title->setPosition(origin.x + size.width * 0.5f, origin.y + size.height - 38.0f);
    addChild(title, 10);

    auto* hint = Label::createWithSystemFont(
        "A/D move   SPACE jetpack   MOUSE aim   LEFT CLICK fire   R reload",
        "Arial",
        17
    );
    hint->setTextColor(Color4B(38, 62, 83, 255));
    hint->setPosition(origin.x + size.width * 0.5f, origin.y + 34.0f);
    addChild(hint, 10);

    playerSpawn_ = Vec2(origin.x + size.width * 0.24f, groundY_);
    targetSpawn_ = Vec2(origin.x + size.width * 0.78f, groundY_);

    player_ = Player::create();
    player_->setPosition(playerSpawn_);
    addChild(player_, 2);

    target_ = Player::create();
    target_->setPosition(targetSpawn_);
    target_->setScale(0.94f);
    target_->setAimDirection(Vec2(-1.0f, 0.0f));
    addChild(target_, 2);

    auto* targetTag = Label::createWithSystemFont("TRAINING BUDDY", "Arial", 14);
    targetTag->setTextColor(Color4B(76, 55, 117, 255));
    targetTag->setPosition(targetSpawn_ + Vec2(0.0f, 72.0f));
    addChild(targetTag, 3);

    reticle_ = DrawNode::create();
    const Color4F reticleColor(1.0f, 0.33f, 0.55f, 0.9f);
    reticle_->drawCircle(Vec2::ZERO, 11.0f, 0.0f, 28, false, reticleColor);
    reticle_->drawLine(Vec2(-17.0f, 0.0f), Vec2(-7.0f, 0.0f), reticleColor);
    reticle_->drawLine(Vec2(17.0f, 0.0f), Vec2(7.0f, 0.0f), reticleColor);
    reticle_->drawLine(Vec2(0.0f, -17.0f), Vec2(0.0f, -7.0f), reticleColor);
    reticle_->drawLine(Vec2(0.0f, 17.0f), Vec2(0.0f, 7.0f), reticleColor);
    reticle_->setPosition(aimWorld_);
    addChild(reticle_, 20);
}

void GameScene::setupHud() {
    const auto size = Director::getInstance()->getVisibleSize();
    const auto origin = Director::getInstance()->getVisibleOrigin();

    auto* hudPanel = DrawNode::create();
    hudPanel->drawSolidRect(
        Vec2(origin.x + 18.0f, origin.y + size.height - 108.0f),
        Vec2(origin.x + 250.0f, origin.y + size.height - 58.0f),
        Color4F(0.16f, 0.20f, 0.34f, 0.82f)
    );
    addChild(hudPanel, 9);

    ammoLabel_ = Label::createWithSystemFont("", "Arial", 18);
    ammoLabel_->setAnchorPoint(Vec2(0.0f, 0.5f));
    ammoLabel_->setPosition(origin.x + 32.0f, origin.y + size.height - 83.0f);
    addChild(ammoLabel_, 10);

    targetHealthLabel_ = Label::createWithSystemFont("", "Arial", 18);
    targetHealthLabel_->setAnchorPoint(Vec2(1.0f, 0.5f));
    targetHealthLabel_->setTextColor(Color4B(76, 55, 117, 255));
    targetHealthLabel_->setPosition(origin.x + size.width - 24.0f, origin.y + size.height - 74.0f);
    addChild(targetHealthLabel_, 10);

    koLabel_ = Label::createWithSystemFont("", "Arial", 19);
    koLabel_->setAnchorPoint(Vec2(1.0f, 0.5f));
    koLabel_->setTextColor(Color4B(76, 55, 117, 255));
    koLabel_->setPosition(origin.x + size.width - 24.0f, origin.y + size.height - 100.0f);
    addChild(koLabel_, 10);

    updateHud();
}

void GameScene::setupInput() {
    auto* keyboard = EventListenerKeyboard::create();

    keyboard->onKeyPressed = [this](EventKeyboard::KeyCode key, Event*) {
        switch (key) {
            case EventKeyboard::KeyCode::KEY_A:
            case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
                moveLeft_ = true;
                break;
            case EventKeyboard::KeyCode::KEY_D:
            case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
                moveRight_ = true;
                break;
            case EventKeyboard::KeyCode::KEY_SPACE:
                jetpack_ = true;
                break;
            case EventKeyboard::KeyCode::KEY_R:
                weapon_.beginReload();
                break;
            default:
                break;
        }
    };

    keyboard->onKeyReleased = [this](EventKeyboard::KeyCode key, Event*) {
        switch (key) {
            case EventKeyboard::KeyCode::KEY_A:
            case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
                moveLeft_ = false;
                break;
            case EventKeyboard::KeyCode::KEY_D:
            case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
                moveRight_ = false;
                break;
            case EventKeyboard::KeyCode::KEY_SPACE:
                jetpack_ = false;
                break;
            default:
                break;
        }
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyboard, this);

    auto* mouse = EventListenerMouse::create();

    mouse->onMouseMove = [this](EventMouse* event) {
        const auto size = Director::getInstance()->getVisibleSize();
        const auto origin = Director::getInstance()->getVisibleOrigin();

        // Desktop cursor coordinates are converted into the scene's bottom-left origin.
        aimWorld_.x = origin.x + event->getCursorX();
        aimWorld_.y = origin.y + size.height - event->getCursorY();
    };

    mouse->onMouseDown = [this](EventMouse* event) {
        if (event->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT) {
            firing_ = true;
            fireProjectile();
        }
    };

    mouse->onMouseUp = [this](EventMouse* event) {
        if (event->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT) {
            firing_ = false;
        }
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouse, this);
}

void GameScene::fireProjectile() {
    if (!player_ || !player_->isAlive() || !weapon_.tryFire()) {
        return;
    }

    const auto direction = player_->aimDirection();
    const auto& definition = weapon_.definition();

    auto* projectile = Projectile::create(
        direction,
        definition.projectileSpeed,
        definition.damage,
        definition.projectileLifetime
    );

    if (!projectile) {
        return;
    }

    projectile->setPosition(player_->getPosition() + direction * 48.0f);
    addChild(projectile, 4);
    projectiles_.push_back(projectile);
}

void GameScene::updateProjectiles(float dt) {
    const auto size = Director::getInstance()->getVisibleSize();
    const auto origin = Director::getInstance()->getVisibleOrigin();

    for (auto it = projectiles_.begin(); it != projectiles_.end();) {
        auto* projectile = *it;
        projectile->simulate(dt);

        bool remove = projectile->expired();
        const auto position = projectile->getPosition();

        if (position.x < origin.x - 60.0f ||
            position.x > origin.x + size.width + 60.0f ||
            position.y < origin.y - 60.0f ||
            position.y > origin.y + size.height + 60.0f) {
            remove = true;
        }

        if (!remove && target_ && target_->isAlive()) {
            const float hitDistance = projectile->hitRadius() + target_->hitRadius();
            if (position.distanceSquared(target_->getPosition()) <= hitDistance * hitDistance) {
                const bool knockedOut = target_->takeDamage(projectile->damage());
                if (knockedOut) {
                    ++koCount_;
                }
                remove = true;
            }
        }

        if (remove) {
            projectile->removeFromParent();
            it = projectiles_.erase(it);
        } else {
            ++it;
        }
    }
}

void GameScene::updateHud() {
    if (ammoLabel_) {
        if (weapon_.isReloading()) {
            ammoLabel_->setString(
                weapon_.definition().displayName + "  |  RELOADING " +
                StringUtils::format("%.1fs", weapon_.reloadRemaining())
            );
        } else {
            ammoLabel_->setString(
                weapon_.definition().displayName + "  |  " +
                StringUtils::format("%d / %d", weapon_.ammoInMagazine(), weapon_.definition().magazineSize)
            );
        }
    }

    if (targetHealthLabel_ && target_) {
        if (target_->isAlive()) {
            targetHealthLabel_->setString(
                StringUtils::format("Buddy HP: %.0f / %.0f", target_->health(), target_->maxHealth())
            );
        } else {
            targetHealthLabel_->setString("Buddy: respawning...");
        }
    }

    if (koLabel_) {
        koLabel_->setString(StringUtils::format("KOs: %d", koCount_));
    }
}

void GameScene::update(float dt) {
    if (!player_) {
        return;
    }

    const auto size = Director::getInstance()->getVisibleSize();
    const auto origin = Director::getInstance()->getVisibleOrigin();

    float axis = 0.0f;
    if (moveLeft_) axis -= 1.0f;
    if (moveRight_) axis += 1.0f;

    player_->setMoveAxis(axis);
    player_->setJetpackActive(jetpack_);
    player_->setAimDirection(aimWorld_ - player_->getPosition());
    player_->simulate(dt, groundY_, origin.x + 28.0f, origin.x + size.width - 28.0f);

    if (target_) {
        target_->updateLifecycle(dt, targetSpawn_);
    }

    weapon_.update(dt);
    if (firing_) {
        fireProjectile();
    }

    updateProjectiles(dt);

    if (reticle_) {
        reticle_->setPosition(aimWorld_);
    }

    updateHud();
}
