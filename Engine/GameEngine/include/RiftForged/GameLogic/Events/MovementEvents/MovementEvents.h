#pragma once

#include <RiftForged/GameLogic/CombatData/CombatData.h> // For a potential future Effect struct
#include <RiftForged/Utilities/MathUtils/MathUtils.h>
#include <vector>
#include <string>

namespace RiftForged {
    namespace GameLogic {
        namespace Events {

            /**
             * @brief An event published when a RiftStep action has been successfully
             * executed by the GameEngine. This contains all data needed for clients
             * to visualize the effect.
             */
            struct RiftStepExecuted {
                uint64_t                instigatorEntityId;
                Utilities::Math::Vec3   actualStartPosition;
                Utilities::Math::Vec3   calculatedTargetPosition;
                Utilities::Math::Vec3   actualFinalPosition;
                float                   travelDurationSec;
                std::string             startVfxId;
                std::string             travelVfxId;
                std::string             endVfxId;

                // We would have a clean GameLogic version of these effects
                // std::vector<GameLogic::Combat::GameplayEffectInstance> entryEffects;
                // std::vector<GameLogic::Combat::GameplayEffectInstance> exitEffects;
            };

            // We would also define other events here like:
            // struct PlayerTurned { ... };

        } // namespace Events
    } // namespace GameLogic
} // namespace RiftForged