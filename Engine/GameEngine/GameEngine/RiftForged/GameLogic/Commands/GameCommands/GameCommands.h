#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

// Assumed to exist. Replace with your actual math library header if different.
// For example: #include <YourThirdParty/Math/Vector.h>
namespace RiftForged { namespace Math { struct Vec3 { float x, y, z; }; } }

namespace RiftForged {
    namespace GameLogic {
        namespace Commands {

            //==--------------------------------------------------------------------==//
            // 1. CORE COMMAND STRUCTURES
            //==--------------------------------------------------------------------==//

            using PlayerID = uint64_t;
            using EntityID = uint64_t;

            //==--------------------------------------------------------------------==//
            // 2. ENUMS (Decoupled from Networking versions)
            //==--------------------------------------------------------------------==//

            enum class RiftStepDirectionalIntent : int8_t {
                Default_Backward,
                Intentional_Forward,
                Intentional_Backward,
                Intentional_Left,
                Intentional_Right
            };

            //==--------------------------------------------------------------------==//
            // 3. SPECIFIC COMMAND DATA STRUCTS (Plain Old Data)
            //==--------------------------------------------------------------------==//

            struct MovementInput {
                uint64_t    clientTimestampMs;
                Math::Vec3  localDirectionIntent;
                bool        isSprinting;
            };

            struct TurnIntent {
                uint64_t    clientTimestampMs;
                float       turnDeltaDegrees;
            };

            struct RiftStepActivation {
                uint64_t                  clientTimestampMs;
                RiftStepDirectionalIntent directionalIntent;
            };

            struct BasicAttackIntent {
                uint64_t    clientTimestampMs;
                Math::Vec3  aimDirection;
                EntityID    targetEntityId;
            };

            struct UseAbility {
                uint64_t    clientTimestampMs;
                uint32_t    abilityId;
                EntityID    targetEntityId;
                Math::Vec3  targetPosition;
            };

            struct Ping {
                uint64_t    clientTimestampMs;
            };

            struct JoinRequest {
                uint64_t    clientTimestampMs;
                std::string characterIdToLoad;
            };

            //==--------------------------------------------------------------------==//
            // 4. THE GENERIC COMMAND WRAPPER
            //==--------------------------------------------------------------------==//

            using CommandData = std::variant<
                MovementInput,
                TurnIntent,
                RiftStepActivation,
                BasicAttackIntent,
                UseAbility,
                Ping,
                JoinRequest
            >;

            struct GameCommand {
                PlayerID    originatingPlayerID;
                CommandData data;
            };

        } // namespace Commands
    } // namespace GameLogic
} // namespace RiftForged