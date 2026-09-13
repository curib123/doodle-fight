#include "GameScene.h"

#include "Combat/Projectile.h"
#include "Network/NetworkManager.h"
#include "Player.h"

#include <algorithm>
#include <sstream>

USING_NS_CC;

namespace {

net::EntityState captureEntity(const Player* player) {
    net::EntityState state{};
    if (!player) {
        return state;
    }

    const auto position = player->getPosition();
    const auto velocity = player->velocity();
    const auto aim = player->aimDirection();

    state.x = position.x;
    state.y = position.y;
    state.velocityX = velocity.x;
    state.velocityY = velocity.y;
    state.aimX = aim.x;
    state.aimY = aim.y;
    state.health = player->health();
    state.alive = player->isAlive();
    return state;
}

void applyEntity(Player* player, const net::EntityState& state, float blend) {
    if (!player) {
        return;
    }

    player->applyNetworkState(
        Vec2(state.x, state.y),
        Vec2(state.velocityX, state.velocityY),
        Vec2(state.aimX, state.aimY),
        state.health,
        state.alive,
        blend
    );
}

} // namespace

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

    auto* title = Label::createWithSystemFont("Doodle Fight - LAN Combat Lab", "Arial", 28);
    title->setTextColor(Color4B(42, 57, 92, 255));
    title->setPosition(origin.x + size.width * 0.5f, origin.y + size.height - 38.0f);
    addChild(title, 10);

    auto* hint = Label::createWithSystemFont(
        "A/D move  SPACE jetpack  MOUSE aim/fire  R reload  |  H host  F find  J join  L leave",
        "Arial",
        15
    );
    hint->setTextColor(Color4B(38, 62, 83, 255));
    hint->setPosition(origin.x + size.width * 0.5f, origin.y + 27.0f);
    addChild(hint, 10);

    hostSpawn_ = Vec2(origin.x + size.width * 0.22f, groundY_);
    peerSpawn_ = Vec2(origin.x + size.width * 0.78f, groundY_);
    targetSpawn_ = peerSpawn_;

    player_ = Player::create();
    player_->setPosition(hostSpawn_);
    addChild(player_, 2);

    remotePlayer_ = Player::create();
    remotePlayer_->setPosition(peerSpawn_);
    remotePlayer_->setScale(0.96f);
    remotePlayer_->setAimDirection(Vec2(-1.0f, 0.0f));
    remotePlayer_->setVisible(false);
    addChild(remotePlayer_, 2);

    target_ = Player::create();
    target_->setPosition(targetSpawn_);
    target_->setScale(0.94f);
    target_->setAimDirection(Vec2(-1.0f, 0.0f));
    addChild(target_, 2);

    targetTagLabel_ = Label::createWithSystemFont("TRAINING BUDDY", "Arial", 14);
    targetTagLabel_->setTextColor(Color4B(76, 55, 117, 255));
    targetTagLabel_->setPosition(targetSpawn_ + Vec2(0.0f, 72.0f));
    addChild(targetTagLabel_, 3);

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
        Vec2(origin.x + 285.0f, origin.y + size.height - 58.0f),
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

    networkStatusLabel_ = Label::createWithSystemFont("LAN: Offline", "Arial", 16);
    networkStatusLabel_->setAnchorPoint(Vec2(0.0f, 0.5f));
    networkStatusLabel_->setTextColor(Color4B(42, 57, 92, 255));
    networkStatusLabel_->setPosition(origin.x + 24.0f, origin.y + 62.0f);
    addChild(networkStatusLabel_, 10);

    roomsLabel_ = Label::createWithSystemFont("", "Arial", 14);
    roomsLabel_->setAnchorPoint(Vec2(0.0f, 0.0f));
    roomsLabel_->setTextColor(Color4B(42, 57, 92, 255));
    roomsLabel_->setPosition(origin.x + 24.0f, origin.y + 78.0f);
    addChild(roomsLabel_, 10);

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
                reloadRequested_ = true;
                break;
            case EventKeyboard::KeyCode::KEY_H:
                startHosting();
                break;
            case EventKeyboard::KeyCode::KEY_F:
                startDiscovery();
                break;
            case EventKeyboard::KeyCode::KEY_J:
                joinFirstRoom();
                break;
            case EventKeyboard::KeyCode::KEY_L:
                leaveNetworkSession();
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
        aimWorld_ = event->getLocation();
    };

    mouse->onMouseDown = [this](EventMouse* event) {
        if (event->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT) {
            firing_ = true;
        }
    };

    mouse->onMouseUp = [this](EventMouse* event) {
        if (event->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT) {
            firing_ = false;
        }
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouse, this);
}

void GameScene::startHosting() {
    auto& network = net::NetworkManager::instance();
    if (!network.hostLan(net::kDefaultLanPort, "Doodle Fight Room")) {
        return;
    }

    for (auto& active : projectiles_) {
        if (active.node) {
            active.node->removeFromParent();
        }
    }
    projectiles_.clear();

    weapon_ = Weapon{};
    remoteWeapon_ = Weapon{};
    hostKOs_ = 0;
    peerKOs_ = 0;
    hostShotCounter_ = 0;
    peerShotCounter_ = 0;
    lastSeenHostShotCounter_ = 0;
    previousNetworkConnected_ = false;

    player_->applyNetworkState(hostSpawn_, Vec2::ZERO, Vec2(1.0f, 0.0f), 100.0f, true);
    remotePlayer_->applyNetworkState(peerSpawn_, Vec2::ZERO, Vec2(-1.0f, 0.0f), 100.0f, true);
    remotePlayer_->setVisible(false);
    target_->setVisible(false);
    targetTagLabel_->setVisible(false);
}

void GameScene::startDiscovery() {
    net::NetworkManager::instance().beginDiscovery(net::kDefaultLanPort);
}

void GameScene::joinFirstRoom() {
    auto& network = net::NetworkManager::instance();
    if (!network.joinFirstDiscoveredRoom()) {
        return;
    }

    for (auto& active : projectiles_) {
        if (active.node) {
            active.node->removeFromParent();
        }
    }
    projectiles_.clear();

    weapon_ = Weapon{};
    remoteWeapon_ = Weapon{};
    hostKOs_ = 0;
    peerKOs_ = 0;
    lastSeenHostShotCounter_ = 0;
    previousNetworkConnected_ = false;

    player_->applyNetworkState(peerSpawn_, Vec2::ZERO, Vec2(-1.0f, 0.0f), 100.0f, true);
    remotePlayer_->applyNetworkState(hostSpawn_, Vec2::ZERO, Vec2(1.0f, 0.0f), 100.0f, true);
    remotePlayer_->setVisible(false);
    target_->setVisible(false);
    targetTagLabel_->setVisible(false);
}

void GameScene::leaveNetworkSession() {
    net::NetworkManager::instance().disconnect();

    for (auto& active : projectiles_) {
        if (active.node) {
            active.node->removeFromParent();
        }
    }
    projectiles_.clear();

    weapon_ = Weapon{};
    remoteWeapon_ = Weapon{};
    previousNetworkConnected_ = false;
    hostKOs_ = 0;
    peerKOs_ = 0;
    hostShotCounter_ = 0;
    peerShotCounter_ = 0;
    lastSeenHostShotCounter_ = 0;

    player_->applyNetworkState(hostSpawn_, Vec2::ZERO, Vec2(1.0f, 0.0f), 100.0f, true);
    remotePlayer_->setVisible(false);
    target_->applyNetworkState(targetSpawn_, Vec2::ZERO, Vec2(-1.0f, 0.0f), 100.0f, true);
    target_->setVisible(true);
    targetTagLabel_->setVisible(true);
}

void GameScene::updateNetworkPresentation() {
    auto& network = net::NetworkManager::instance();
    const bool sessionMode = network.role() == net::NetworkRole::Host ||
                             network.role() == net::NetworkRole::Peer;

    if (target_) {
        target_->setVisible(!sessionMode && target_->isAlive());
    }
    if (targetTagLabel_) {
        targetTagLabel_->setVisible(!sessionMode);
    }

    if (!network.connected() && remotePlayer_) {
        remotePlayer_->setVisible(false);
    }

    if (network.connected() && !previousNetworkConnected_) {
        for (auto& active : projectiles_) {
            if (active.node) {
                active.node->removeFromParent();
            }
        }
        projectiles_.clear();

        weapon_ = Weapon{};
        remoteWeapon_ = Weapon{};
        hostKOs_ = 0;
        peerKOs_ = 0;
        hostShotCounter_ = 0;
        peerShotCounter_ = 0;
        lastSeenHostShotCounter_ = 0;

        if (network.role() == net::NetworkRole::Host) {
            player_->applyNetworkState(hostSpawn_, Vec2::ZERO, Vec2(1.0f, 0.0f), 100.0f, true);
            remotePlayer_->applyNetworkState(peerSpawn_, Vec2::ZERO, Vec2(-1.0f, 0.0f), 100.0f, true);
        } else {
            player_->applyNetworkState(peerSpawn_, Vec2::ZERO, Vec2(-1.0f, 0.0f), 100.0f, true);
            remotePlayer_->applyNetworkState(hostSpawn_, Vec2::ZERO, Vec2(1.0f, 0.0f), 100.0f, true);
        }
        remotePlayer_->setVisible(true);
    }

    previousNetworkConnected_ = network.connected();
}

bool GameScene::fireProjectile(Player* shooter,
                               Weapon& weapon,
                               ProjectileOwner owner,
                               bool authoritative) {
    if (!shooter || !shooter->isAlive() || !weapon.tryFire()) {
        return false;
    }

    const auto direction = shooter->aimDirection();
    const auto& definition = weapon.definition();
    auto* projectile = Projectile::create(
        direction,
        definition.projectileSpeed,
        definition.damage,
        definition.projectileLifetime
    );

    if (!projectile) {
        return false;
    }

    projectile->setPosition(shooter->getPosition() + direction * 48.0f);
    addChild(projectile, 4);
    projectiles_.push_back({projectile, owner, authoritative});

    if (authoritative && net::NetworkManager::instance().role() == net::NetworkRole::Host) {
        if (owner == ProjectileOwner::Host) {
            ++hostShotCounter_;
        } else if (owner == ProjectileOwner::Peer) {
            ++peerShotCounter_;
        }
    }

    return true;
}

void GameScene::spawnVisualProjectile(Player* shooter, ProjectileOwner owner) {
    if (!shooter || !shooter->isAlive()) {
        return;
    }

    const auto direction = shooter->aimDirection();
    const auto& definition = remoteWeapon_.definition();
    auto* projectile = Projectile::create(
        direction,
        definition.projectileSpeed,
        definition.damage,
        definition.projectileLifetime
    );

    if (!projectile) {
        return;
    }

    projectile->setPosition(shooter->getPosition() + direction * 48.0f);
    addChild(projectile, 4);
    projectiles_.push_back({projectile, owner, false});
}

void GameScene::updateProjectiles(float dt) {
    const auto size = Director::getInstance()->getVisibleSize();
    const auto origin = Director::getInstance()->getVisibleOrigin();
    const auto role = net::NetworkManager::instance().role();
    const bool networkConnected = net::NetworkManager::instance().connected();

    for (auto it = projectiles_.begin(); it != projectiles_.end();) {
        auto* projectile = it->node;
        projectile->simulate(dt);

        bool remove = projectile->expired();
        const auto position = projectile->getPosition();
        if (position.x < origin.x - 60.0f ||
            position.x > origin.x + size.width + 60.0f ||
            position.y < origin.y - 60.0f ||
            position.y > origin.y + size.height + 60.0f) {
            remove = true;
        }

        Player* damageTarget = nullptr;
        if (!remove && it->authoritative) {
            if (role == net::NetworkRole::Offline && it->owner == ProjectileOwner::OfflinePlayer) {
                damageTarget = target_;
            } else if (role == net::NetworkRole::Host && networkConnected) {
                if (it->owner == ProjectileOwner::Host) {
                    damageTarget = remotePlayer_;
                } else if (it->owner == ProjectileOwner::Peer) {
                    damageTarget = player_;
                }
            }
        }

        if (!remove && damageTarget && damageTarget->isAlive()) {
            const float hitDistance = projectile->hitRadius() + damageTarget->hitRadius();
            if (position.distanceSquared(damageTarget->getPosition()) <= hitDistance * hitDistance) {
                const bool knockedOut = damageTarget->takeDamage(projectile->damage());
                if (knockedOut) {
                    if (role == net::NetworkRole::Offline) {
                        ++offlineKOs_;
                    } else if (it->owner == ProjectileOwner::Host) {
                        ++hostKOs_;
                    } else if (it->owner == ProjectileOwner::Peer) {
                        ++peerKOs_;
                    }
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

void GameScene::updateOffline(float dt) {
    const auto size = Director::getInstance()->getVisibleSize();
    const auto origin = Director::getInstance()->getVisibleOrigin();

    player_->simulate(dt, groundY_, origin.x + 28.0f, origin.x + size.width - 28.0f);
    target_->updateLifecycle(dt, targetSpawn_);

    weapon_.update(dt);
    if (firing_) {
        fireProjectile(player_, weapon_, ProjectileOwner::OfflinePlayer, true);
    }

    updateProjectiles(dt);
}

void GameScene::updateHost(float dt) {
    auto& network = net::NetworkManager::instance();
    const auto size = Director::getInstance()->getVisibleSize();
    const auto origin = Director::getInstance()->getVisibleOrigin();

    player_->updateLifecycle(dt, hostSpawn_);
    player_->simulate(dt, groundY_, origin.x + 28.0f, origin.x + size.width - 28.0f);

    weapon_.update(dt);
    remoteWeapon_.update(dt);

    if (network.connected()) {
        if (firing_) {
            fireProjectile(player_, weapon_, ProjectileOwner::Host, true);
        }

        const auto& peerInput = network.latestPeerInput();
        remotePlayer_->setMoveAxis(peerInput.moveAxis);
        remotePlayer_->setJetpackActive(peerInput.jetpack);
        remotePlayer_->setAimDirection(Vec2(peerInput.aimX, peerInput.aimY));
        remotePlayer_->updateLifecycle(dt, peerSpawn_);
        remotePlayer_->simulate(dt, groundY_, origin.x + 28.0f, origin.x + size.width - 28.0f);

        if (peerInput.reload) {
            remoteWeapon_.beginReload();
        }
        if (peerInput.firing) {
            fireProjectile(remotePlayer_, remoteWeapon_, ProjectileOwner::Peer, true);
        }
    }

    updateProjectiles(dt);

    net::MatchSnapshot snapshot{};
    snapshot.host = captureEntity(player_);
    snapshot.peer = captureEntity(remotePlayer_);
    snapshot.hostKOs = hostKOs_;
    snapshot.peerKOs = peerKOs_;
    snapshot.hostShotCounter = hostShotCounter_;
    snapshot.peerShotCounter = peerShotCounter_;
    network.publishSnapshot(snapshot);
}

void GameScene::updatePeer(float dt) {
    auto& network = net::NetworkManager::instance();
    const auto size = Director::getInstance()->getVisibleSize();
    const auto origin = Director::getInstance()->getVisibleOrigin();

    // Local prediction: movement is immediate on the peer, then gently corrected
    // toward the authoritative position received from the host.
    player_->simulate(dt, groundY_, origin.x + 28.0f, origin.x + size.width - 28.0f);

    weapon_.update(dt);
    remoteWeapon_.update(dt);
    if (network.connected() && firing_) {
        fireProjectile(player_, weapon_, ProjectileOwner::Peer, false);
    }

    net::MatchSnapshot snapshot{};
    if (network.consumeLatestSnapshot(snapshot)) {
        applyEntity(remotePlayer_, snapshot.host, 0.65f);
        applyEntity(player_, snapshot.peer, 0.18f);

        hostKOs_ = snapshot.hostKOs;
        peerKOs_ = snapshot.peerKOs;

        if (snapshot.hostShotCounter < lastSeenHostShotCounter_) {
            lastSeenHostShotCounter_ = snapshot.hostShotCounter;
        }

        const std::uint32_t unseenShots = snapshot.hostShotCounter - lastSeenHostShotCounter_;
        const std::uint32_t visualShots = std::min<std::uint32_t>(unseenShots, 3);
        for (std::uint32_t i = 0; i < visualShots; ++i) {
            spawnVisualProjectile(remotePlayer_, ProjectileOwner::Host);
        }
        lastSeenHostShotCounter_ = snapshot.hostShotCounter;
    }

    updateProjectiles(dt);
}

void GameScene::updateHud() {
    auto& network = net::NetworkManager::instance();

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

    if (targetHealthLabel_) {
        if (network.role() == net::NetworkRole::Host) {
            targetHealthLabel_->setString(
                network.connected()
                    ? (remotePlayer_->isAlive()
                        ? StringUtils::format("Peer HP: %.0f", remotePlayer_->health())
                        : "Peer: respawning...")
                    : "Waiting for peer"
            );
        } else if (network.role() == net::NetworkRole::Peer) {
            targetHealthLabel_->setString(
                network.connected()
                    ? (remotePlayer_->isAlive()
                        ? StringUtils::format("Host HP: %.0f", remotePlayer_->health())
                        : "Host: respawning...")
                    : "Connecting to host"
            );
        } else {
            targetHealthLabel_->setString(
                target_->isAlive()
                    ? StringUtils::format("Buddy HP: %.0f / %.0f", target_->health(), target_->maxHealth())
                    : "Buddy: respawning..."
            );
        }
    }

    if (koLabel_) {
        if (network.role() == net::NetworkRole::Host || network.role() == net::NetworkRole::Peer) {
            koLabel_->setString(StringUtils::format("HOST %u  -  %u PEER", hostKOs_, peerKOs_));
        } else {
            koLabel_->setString(StringUtils::format("KOs: %d", offlineKOs_));
        }
    }

    if (networkStatusLabel_) {
        networkStatusLabel_->setString("LAN: " + network.statusText());
    }

    if (roomsLabel_) {
        if (network.rooms().empty()) {
            roomsLabel_->setString(network.discovering() ? "Scanning for rooms..." : "");
        } else {
            std::ostringstream text;
            const std::size_t count = std::min<std::size_t>(network.rooms().size(), 3);
            for (std::size_t i = 0; i < count; ++i) {
                const auto& room = network.rooms()[i];
                if (i > 0) {
                    text << "   |   ";
                }
                text << "[J] " << room.name << " "
                     << static_cast<int>(room.playerCount) << "/"
                     << static_cast<int>(room.maxPlayers) << " @ " << room.address;
            }
            roomsLabel_->setString(text.str());
        }
    }
}

void GameScene::update(float dt) {
    if (!player_) {
        return;
    }

    float axis = 0.0f;
    if (moveLeft_) axis -= 1.0f;
    if (moveRight_) axis += 1.0f;

    player_->setMoveAxis(axis);
    player_->setJetpackActive(jetpack_);
    player_->setAimDirection(aimWorld_ - player_->getPosition());

    auto& network = net::NetworkManager::instance();
    if (network.role() == net::NetworkRole::Peer) {
        net::InputState input{};
        input.moveAxis = axis;
        input.aimX = player_->aimDirection().x;
        input.aimY = player_->aimDirection().y;
        input.jetpack = jetpack_;
        input.firing = firing_;
        input.reload = reloadRequested_;
        network.setLocalInput(input);
    }

    network.update(dt);
    updateNetworkPresentation();

    switch (network.role()) {
        case net::NetworkRole::Host:
            updateHost(dt);
            break;
        case net::NetworkRole::Peer:
            updatePeer(dt);
            break;
        case net::NetworkRole::Offline:
        default:
            updateOffline(dt);
            break;
    }

    reloadRequested_ = false;

    if (reticle_) {
        reticle_->setPosition(aimWorld_);
    }

    updateHud();
}
