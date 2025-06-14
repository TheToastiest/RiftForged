#pragma once

#include <RiftForged/GameLogic/CombatData/CombatData.h> // We use the structs we just defined
#include <string>

namespace RiftForged {
    namespace GameLogic {
        namespace Events {

            /**
             * @brief Published when any entity successfully deals damage to another.
             * This will be the trigger for a formatter to create an S2C_CombatEventMsg.
             */
            struct EntityDealtDamage {
                // We can just use the details struct we already have!
                Combat::DamageApplicationDetails Details;
            };

            /**
             * @brief Published when a player's ability fails for a specific reason.
             * This will be the trigger for a formatter to create an S2C_AbilityFailedMsg.
             */
            struct PlayerAbilityFailed {
                uint64_t    playerId;
                uint32_t    abilityId;
                std::string reason;
            };

            // We would add other specific events as needed, for example:
            // struct PlayerDied { uint64_t deceasedId; uint64_t killerId; };

        } // namespace Events
    } // namespace GameLogic
} // namespace RiftForged