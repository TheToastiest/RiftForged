#pragma once
#include <RiftForged/GameLogic/CombatData/CombatData.h>
#include <string>

namespace RiftForged {
    namespace GameLogic {
        namespace Events {

            // Published when any entity successfully deals damage.
            // This will trigger the creation of an S2C_CombatEventMsg.
            struct EntityDealtDamage {
                Combat::DamageApplicationDetails Details;
            };

            // Published when a projectile is created by an attack or ability.
            // This will trigger the creation of an S2C_SpawnProjectileMsg.
            struct ProjectileSpawned {
                uint64_t                projectileId;
                uint64_t                ownerEntityId;
                Utilities::Math::Vec3   startPosition;
                Utilities::Math::Vec3   initialDirection;
                float                   speed;
                float                   maxRange;
                std::string             projectileVfxTag;
                Combat::DamageInstance  damageOnHit;
            };

            // Published when an ability fails.
            // This will trigger the creation of an S2C_AbilityFailedMsg.
            struct PlayerAbilityFailed {
                uint64_t    playerId;
                uint32_t    abilityId;
                std::string reason;
            };

            // We would add similar failure events as needed:
            // struct PlayerBasicAttackFailed { ... };

        } // namespace Events
    } // namespace GameLogic
} // namespace RiftForged