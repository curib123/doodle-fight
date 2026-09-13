#pragma once

#include <string>

struct WeaponDefinition {
    std::string id{"pulse_pistol"};
    std::string displayName{"Pulse Pistol"};
    float damage{25.0f};
    float projectileSpeed{760.0f};
    float fireInterval{0.22f};
    float projectileLifetime{1.8f};
    int magazineSize{12};
    float reloadTime{1.15f};
};

class Weapon final {
public:
    explicit Weapon(WeaponDefinition definition = {});

    void update(float dt);

    bool canFire() const;
    bool tryFire();
    bool beginReload();

    const WeaponDefinition& definition() const { return definition_; }
    int ammoInMagazine() const { return ammoInMagazine_; }
    bool isReloading() const { return reloadTimer_ > 0.0f; }
    float reloadRemaining() const { return reloadTimer_; }

private:
    WeaponDefinition definition_;
    int ammoInMagazine_{0};
    float cooldown_{0.0f};
    float reloadTimer_{0.0f};
};
