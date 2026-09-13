#include "GameScene.h"
#include "Player.h"

USING_NS_CC;

bool GameScene::init() {
    if (!Scene::init()) {
        return false;
    }

    setupArena();
    setupInput();
    scheduleUpdate();
    return true;
}

void GameScene::setupArena() {
    const auto size = Director::getInstance()->getVisibleSize();
    const auto origin = Director::getInstance()->getVisibleOrigin();

    auto* background = LayerColor::create(Color4B(131, 211, 255, 255));
    addChild(background, -10);

    auto* arena = DrawNode::create();
    arena->drawSolidRect(
        Vec2(origin.x, origin.y),
        Vec2(origin.x + size.width, groundY_ - 28.0f),
        Color4F(0.35f, 0.80f, 0.45f, 1.0f)
    );
    arena->drawSolidRect(
        Vec2(origin.x, groundY_ - 28.0f),
        Vec2(origin.x + size.width, groundY_),
        Color4F(0.18f, 0.57f, 0.34f, 1.0f)
    );
    addChild(arena, -5);

    auto* title = Label::createWithSystemFont("Doodle Fight - Prototype", "Arial", 28);
    title->setPosition(origin.x + size.width * 0.5f, origin.y + size.height - 42.0f);
    addChild(title);

    auto* hint = Label::createWithSystemFont("A/D or arrows: move   |   Space: jetpack", "Arial", 18);
    hint->setPosition(origin.x + size.width * 0.5f, origin.y + 42.0f);
    addChild(hint);

    player_ = Player::create();
    player_->setPosition(origin.x + size.width * 0.5f, groundY_);
    addChild(player_, 2);
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
    player_->simulate(dt, groundY_, origin.x + 24.0f, origin.x + size.width - 24.0f);
}
