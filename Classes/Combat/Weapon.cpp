#include "Combat/Weapon.h"
#include <algorithm>

Weapon::Weapon(WeaponDefinition definition)
    : definition_(std::move(definition)), ammoInMagazine_(definition_.magazineSize) {}

void Weapon::update(float dt) {
    cooldown_ = std::max(0.0f, cooldown_ - dt);

    if (reloadTimer_ > 0.0f) {
        reloadTimer_ = std::max(0.0f, reloadTimer_ - dt);
        if (reloadTimer_ <= 0.0f) {
            ammoInMagazine_ = definition_.magazineSize;
        }
    }
}

bool Weapon::canFire() const {
    return cooldown_ <= 0.0f && reloadTimer_ <= 0.0f && ammoInMagazine_ > 0;
}

bool Weapon::tryFire() {
    if (!canFire()) {
        return false;
    }

    --ammoInMagazine_;
    cooldown_ = definition_.fireInterval;

    if (ammoInMagazine_ <= 0) {
        beginReload();
    }

    return true;
}

bool Weapon::beginReload() {
    if (reloadTimer_ > 0.0f || ammoInMagazine_ >= definition_.magazineSize) {
        return false;
    }

    reloadTimer_ = definition_.reloadTime;
    return true;
}
