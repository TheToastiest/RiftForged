// File: GameLogic/CombatSystem.h (Refactored)
#pragma once

#include <vector>
#include <cstdint>
#include <string>

// GameLogic dependencies are clean and allowed
#include <RiftForged/GameLogic/PlayerManager/PlayerManager.h>
#include <RiftForged/Physics/PhysicsEngine/PhysicsEngine.h>
#include <RiftForged/Utilities/MathUtils/MathUtils.h>
#include <RiftForged/GameLogic/CombatData/CombatData.h>

// ADDED: The CombatSystem now operates on clean GameCommands.
#include <RiftForged/GameLogic/GameCommands/GameCommands.h>

// REMOVED: Forward declaration of FlatBuffer types are no longer needed.
// namespace RiftForged { namespace Networking { namespace UDP { namespace C2S { struct C2S_BasicAttackIntentMsgT; } } } }

// Forward declaration for PhysX actor remains.
namespace physx { class PxRigidActor; }


namespace RiftForged {
    namespace GameLogic {
        namespace Combat {

            // This struct is now pure GameLogic
            struct MeleeAttackProperties {
                float sweepDistance;
                float capsuleRadius;
                float capsuleHalfHeight;
                float sweepStartOffset;
                DamageInstance damage; // CHANGED: Now uses our clean GameLogic::Combat::DamageInstance

                MeleeAttackProperties(float dist, float radius, float halfHeight, float offset,
                    const DamageInstance& dmg) // CHANGED: Takes our clean struct
                    : sweepDistance(dist), capsuleRadius(radius), capsuleHalfHeight(halfHeight),
                    sweepStartOffset(offset), damage(dmg) {
                }
            };

            // This function signature is now pure GameLogic
            AttackOutcome ProcessBasicMeleeAttack(
                uint64_t casterPlayerId,
                const Commands::BasicAttackIntent& attackIntent, // CHANGED: Now uses our clean command struct
                PlayerManager& playerManager,
                Physics::PhysicsEngine& physicsEngine
            );

            // This struct is fine as-is for now
            struct AbilityDefinition {
                // ...
            };

            // This function signature is now pure GameLogic
            AttackOutcome ProcessAbilityLaunchPhysicsProjectile(
                uint64_t casterPlayerId,
                const Commands::UseAbility& useAbilityIntent, // CHANGED: Now uses our clean command struct
                const AbilityDefinition& abilityDef,
                PlayerManager& playerManager,
                Physics::PhysicsEngine& physicsEngine
            );

        } // namespace Combat
    } // namespace GameLogic
} // namespace RiftForged
} // namespace RiftForged