#pragma once
#include <RiftForged/Utilities/MathUtils/MathUtils.h>
#include <cstdint>

namespace RiftForged {
    namespace GameLogic {
        namespace Events {

            // Published periodically for any entity that has moved or changed state.
            // This will trigger the creation of an S2C_EntityStateUpdateMsg.
            struct EntityStateUpdated {
                uint64_t                entityId;
                Utilities::Math::Vec3   position;
                Utilities::Math::Quaternion orientation;
                // We can add more state here later (health, animation state, etc.)
            };

        } // namespace Events
    } // namespace GameLogic
} // namespace RiftForged