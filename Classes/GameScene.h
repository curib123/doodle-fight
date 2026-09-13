#pragma once

#include "cocos2d.h"
#include "Combat/Weapon.h"

#include <cstdint>
#include <vector>

class Player;
class Projectile;

class GameScene final : public cocos2d::Scene {
public:
    CREATE_FUNC(GameScene);
    bool init() override;
    void update(float dt) override;

private:
    enum class ProjectileOwner : std::uint8_t {
        OfflinePlayer,
        Host,
        Peer
    };

    struct ActiveProjectile {
        Projectile* node{nullptr};
        ProjectileOwner owner{ProjectileOwner::OfflinePlayer};
        bool authoritative{false};
    };

    void setupArena();
    void setupInput();
    void setupHud();

    void startHosting();
    void startDiscovery();
    void joinFirstRoom();
    void leaveNetworkSession();
    void updateNetworkPresentation();

    bool fireProjectile(Player* shooter,
                        Weapon& weapon,
                        ProjectileOwner owner,
                        bool authoritative);
    void spawnVisualProjectile(Player* shooter, ProjectileOwner owner);
    void updateProjectiles(float dt);
    void updateOffline(float dt);
    void updateHost(float dt);
    void updatePeer(float dt);
    void updateHud();

    Player* player_{nullptr};
    Player* remotePlayer_{nullptr};
    Player* target_{nullptr};
    std::vector<ActiveProjectile> projectiles_;

    Weapon weapon_{};
    Weapon remoteWeapon_{};

    cocos2d::Vec2 aimWorld_{700.0f, 320.0f};
    cocos2d::Vec2 hostSpawn_{220.0f, 90.0f};
    cocos2d::Vec2 peerSpawn_{820.0f, 90.0f};
    cocos2d::Vec2 targetSpawn_{760.0f, 90.0f};

    cocos2d::DrawNode* reticle_{nullptr};
    cocos2d::Label* ammoLabel_{nullptr};
    cocos2d::Label* targetHealthLabel_{nullptr};
    cocos2d::Label* koLabel_{nullptr};
    cocos2d::Label* networkStatusLabel_{nullptr};
    cocos2d::Label* roomsLabel_{nullptr};

    bool moveLeft_{false};
    bool moveRight_{false};
    bool jetpack_{false};
    bool firing_{false};
    bool reloadRequested_{false};

    int offlineKOs_{0};
    std::uint32_t hostKOs_{0};
    std::uint32_t peerKOs_{0};
    std::uint32_t hostShotCounter_{0};
    std::uint32_t peerShotCounter_{0};
    std::uint32_t lastSeenHostShotCounter_{0};

    bool previousNetworkConnected_{false};
    float groundY_{90.0f};
};
