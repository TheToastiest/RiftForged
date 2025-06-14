// File: GameplayEngine/CombatSystem.cpp (or similar name)
// Copyright (c) 2023-2025 RiftForged Game Development Team

#include <RiftForged/GameLogic/CombatSystem/CombatSystem.h>
#include <RiftForged/GameLogic/CombatData/CombatData.h> // For AttackOutcome, MeleeAttackProperties
#include <RiftForged/GameLogic/PlayerManager/PlayerManager.h> // For PlayerManager, ActivePlayer
#include <RiftForged/Physics/PhysicsEngine/PhysicsEngine.h> // For Physics::PhysicsEngine
#include <RiftForged/Physics/PhysicsTypes.h> // For EPhysicsObjectType, Physics::ToPxVec3, etc.
#include <RiftForged/Utilities/MathUtils/MathUtils.h> // For GLM-based math types (Vec3, Quaternion)
//#include "../FlatBuffers/Versioning/V0.0.5/riftforged_common_types_generated.h" // For enums, DamageInstance
//#include "../FlatBuffers/Versioning/V0.0.5/riftforged_c2s_udp_messages_generated.h" // For C2S message types
#include <RiftForged/Utilities/Logger/Logger.h> // For logging macros

// For PhysX types
#include "physx/PxPhysicsAPI.h" 

namespace RiftForged {
    namespace GameLogic {
        namespace Combat {

            // Helper Query Filter Callback for Melee Sweeps
            struct MeleeSweepQueryFilterCallback : public physx::PxQueryFilterCallback {
                physx::PxRigidActor* m_casterPhysicsActor;
                uint64_t m_casterEntityId;

                MeleeSweepQueryFilterCallback(physx::PxRigidActor* casterActor, uint64_t casterId)
                    : m_casterPhysicsActor(casterActor), m_casterEntityId(casterId) {
                }

                virtual physx::PxQueryHitType::Enum preFilter(
                    const physx::PxFilterData& /*filterData*/, // PhysX filter data from the shape
                    const physx::PxShape* /*shape*/,
                    const physx::PxRigidActor* hitActor,
                    physx::PxHitFlags& /*queryFlags*/) override {

                    if (!hitActor) {
                        return physx::PxQueryHitType::eNONE;
                    }

                    if (hitActor == m_casterPhysicsActor && m_casterPhysicsActor != nullptr) {
                        return physx::PxQueryHitType::eNONE;
                    }

                    if (hitActor->userData) {
                        uint64_t hitEntityId = reinterpret_cast<uint64_t>(hitActor->userData);
                        if (hitEntityId == m_casterEntityId) {
                            return physx::PxQueryHitType::eNONE;
                        }
                    }
                    // TODO: Add more sophisticated game-specific filtering (faction, state, etc.)
                    return physx::PxQueryHitType::eBLOCK; // Process as a potential blocking hit
                }

                virtual physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData&, const physx::PxQueryHit&, const physx::PxShape*, const physx::PxRigidActor*) override {
                    return physx::PxQueryHitType::eBLOCK;
                }
            };


            AttackOutcome ProcessBasicMeleeAttack(
                uint64_t casterPlayerId,
                const Networking::UDP::C2S::C2S_BasicAttackIntentMsgT& attackIntent, // FlatBuffers object API type
                PlayerManager& playerManager,
                Physics::PhysicsEngine& physicsEngine
            ) {
                AttackOutcome outcome;
                outcome.is_basic_attack = true;
                outcome.success = false; // Default to failure

                ActivePlayer* caster = playerManager.FindPlayerById(casterPlayerId);
                if (!caster) {
                    RF_COMBAT_WARN("ProcessBasicMeleeAttack: Caster with ID %llu not found.", casterPlayerId);
                    outcome.failure_reason_code = "CASTER_NOT_FOUND";
                    return outcome;
                }

                // Assuming ActivePlayer members 'position' and 'orientation' are GLM types
                Utilities::Math::Vec3 casterPos = caster->position;
                Utilities::Math::Quaternion casterOrientation = caster->orientation;

                physx::PxRigidActor* casterPhysicsActor = physicsEngine.GetRigidActor(casterPlayerId);
                if (!casterPhysicsActor) {
                    physx::PxController* controller = physicsEngine.GetPlayerController(casterPlayerId);
                    if (controller) {
                        casterPhysicsActor = controller->getActor();
                    }
                }
                if (!casterPhysicsActor) {
                    RF_COMBAT_WARN("ProcessBasicMeleeAttack: Could not retrieve PxRigidActor for caster ID %llu.", casterPlayerId);
                    // Filter callback will rely on entity ID check if PxActor pointer is null.
                }

                Utilities::Math::Vec3 casterForward = Utilities::Math::GetWorldForwardVector(casterOrientation);

                // Define Melee Attack Properties (HARDCODED for this example)
                Networking::Shared::DamageInstance baseDamage(15, Networking::Shared::DamageType::DamageType_Physical, false);
                MeleeAttackProperties props(
                    /*sweepDistance:*/ 2.0f,
                    /*capsuleRadius:*/ 0.6f,
                    /*capsuleHalfHeight:*/ caster->capsule_half_height,
                    /*sweepStartOffset:*/ 0.5f,
                    baseDamage
                );
                outcome.attack_animation_tag_for_caster = "BasicMelee_Sword_01"; // Placeholder

                physx::PxCapsuleGeometry capsuleGeometry(props.capsuleRadius, props.capsuleHalfHeight);
                Utilities::Math::Vec3 sweepStartPos = Utilities::Math::AddVectors(casterPos, Utilities::Math::ScaleVector(casterForward, props.sweepStartOffset));

                // Use Physics::ToPxVec3 and Physics::ToPxQuat defined in PhysicsTypes.h or PhysicsEngine.h
                physx::PxTransform capsuleInitialPose(
                    Physics::ToPxVec3(sweepStartPos),
                    Physics::ToPxQuat(casterOrientation)
                );

                physx::PxQueryFilterData filterData;
                filterData.flags = physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::ePREFILTER;
                // filterData.data.word0 = static_cast<physx::PxU32>(YOUR_FILTER_GROUP_FOR_PLAYER_ATTACKS); // Example

                MeleeSweepQueryFilterCallback filterCallback(casterPhysicsActor, casterPlayerId);

                const physx::PxU32 maxMeleeHits = 10;
                physx::PxSweepHit hitBufferArray[maxMeleeHits]; // Stack allocation for hit buffer
                physx::PxSweepBuffer multipleHitBuffer(hitBufferArray, maxMeleeHits);

                bool bSweepHitOccurred = false;
                if (physicsEngine.GetScene()) {
                    bSweepHitOccurred = physicsEngine.GetScene()->sweep(
                        capsuleGeometry,
                        capsuleInitialPose,
                        Physics::ToPxVec3(casterForward), // Sweep direction
                        props.sweepDistance,
                        multipleHitBuffer,
                        physx::PxHitFlags(physx::PxHitFlag::eDEFAULT | physx::PxHitFlag::eMESH_BOTH_SIDES),
                        filterData,
                        &filterCallback,
                        nullptr // PxQueryCache
                    );
                }
                else {
                    RF_COMBAT_ERROR("ProcessBasicMeleeAttack: Physics scene is null for caster ID %llu.", casterPlayerId);
                    outcome.failure_reason_code = "SCENE_NULL";
                    return outcome;
                }

                outcome.success = true; // Attack was processed (even if it missed)

                if (bSweepHitOccurred && multipleHitBuffer.getNbTouches() > 0) {
                    RF_COMBAT_TRACE("Melee sweep for caster %llu hit %u actor(s).", casterPlayerId, multipleHitBuffer.getNbTouches());
                    for (physx::PxU32 i = 0; i < multipleHitBuffer.getNbTouches(); ++i) {
                        const physx::PxSweepHit& touch = multipleHitBuffer.getTouch(i);
                        if (touch.actor && touch.actor->userData) {
                            uint64_t hitEntityId = reinterpret_cast<uint64_t>(touch.actor->userData);
                            if (hitEntityId == casterPlayerId) continue; // Should be caught by callback, but safety check

                            RF_COMBAT_TRACE("Caster %llu melee hit Entity ID: %llu", casterPlayerId, hitEntityId);

                            DamageApplicationDetails DADetails;
                            DADetails.target_id = hitEntityId;
                            DADetails.source_id = casterPlayerId; // Set the source
                            DADetails.final_damage_dealt = props.damage.amount();
                            DADetails.damage_type = props.damage.type();
                            DADetails.was_crit = props.damage.is_crit();
                            // DADetails.impact_point = Physics::FromPxVec3(touch.position); // If impact_point is enabled in CombatData.h

                            // TODO: Check if target exists in PlayerManager and apply damage via target->TakeDamage()
                            // For now, just add to event list.
                            // ActivePlayer* targetPlayer = playerManager.FindPlayerById(hitEntityId);
                            // if (targetPlayer) { DADetails.was_kill = (targetPlayer->GetCurrentHealth() - DADetails.final_damage_dealt <= 0); }

                            outcome.damage_events.push_back(DADetails);
                        }
                    }
                    if (!outcome.damage_events.empty()) {
                        outcome.simulated_combat_event_type = Networking::UDP::S2C::CombatEventType::CombatEventType_DamageDealt;
                    }
                    else {
                        outcome.simulated_combat_event_type = Networking::UDP::S2C::CombatEventType::CombatEventType_Miss;
                    }
                }
                else {
                    RF_COMBAT_TRACE("Melee sweep for caster %llu reported no hits.", casterPlayerId);
                    outcome.simulated_combat_event_type = Networking::UDP::S2C::CombatEventType::CombatEventType_Miss;
                }
                // TODO: Apply cooldown to caster for basic attack
                // caster->StartAbilityCooldown(BASIC_ATTACK_ABILITY_ID, caster->base_basic_attack_cooldown_sec);
                return outcome;
            }

            AttackOutcome ProcessAbilityLaunchPhysicsProjectile(
                uint64_t casterPlayerId,
                const Networking::UDP::C2S::C2S_UseAbilityMsgT& useAbilityIntent, // FlatBuffers object API type
                const AbilityDefinition& abilityDef, // Contains static data about the ability
                PlayerManager& playerManager,
                Physics::PhysicsEngine& physicsEngine
            ) {
                AttackOutcome outcome;
                outcome.success = false;

                ActivePlayer* caster = playerManager.FindPlayerById(casterPlayerId);
                if (!caster) {
                    RF_COMBAT_WARN("ProcessAbilityLaunchPhysicsProjectile: Caster ID %llu not found.", casterPlayerId);
                    outcome.failure_reason_code = "CASTER_NOT_FOUND";
                    return outcome;
                }

                // Assuming ActivePlayer::GetMuzzlePosition() returns Utilities::Math::Vec3 (GLM)
                // Assuming ActivePlayer::orientation is Utilities::Math::Quaternion (GLM)
                Utilities::Math::Vec3 projectileStartPosition = caster->GetMuzzlePosition();
                Utilities::Math::Vec3 projectileInitialDirection; // GLM Vec3

                if (useAbilityIntent.target_position) { // target_position is std::unique_ptr<Networking::Shared::Vec3T>
                    // Convert FlatBuffers Vec3T to Utilities::Math::Vec3 (GLM)
                    const auto& fbTargetPos = *(useAbilityIntent.target_position);
                    // Call .x(), .y(), .z() as functions for FlatBuffers Vec3T
                    Utilities::Math::Vec3 targetPosGLM(fbTargetPos.x(), fbTargetPos.y(), fbTargetPos.z());
                    projectileInitialDirection = Utilities::Math::SubtractVectors(targetPosGLM, projectileStartPosition);
                }
                else if (useAbilityIntent.target_entity_id != 0) {
                    ActivePlayer* targetEntity = playerManager.FindPlayerById(useAbilityIntent.target_entity_id);
                    if (targetEntity) {
                        // targetEntity->position is already Utilities::Math::Vec3 (GLM)
                        projectileInitialDirection = Utilities::Math::SubtractVectors(targetEntity->position, projectileStartPosition);
                    }
                    else {
                        RF_COMBAT_WARN("ProcessAbilityLaunchPhysicsProjectile: Target entity ID %llu not found. Defaulting to caster forward.", useAbilityIntent.target_entity_id);
                        projectileInitialDirection = Utilities::Math::GetWorldForwardVector(caster->orientation);
                    }
                }
                else {
                    projectileInitialDirection = Utilities::Math::GetWorldForwardVector(caster->orientation);
                }

                if (Utilities::Math::Magnitude(projectileInitialDirection) > Utilities::Math::VECTOR_NORMALIZATION_EPSILON_SQ) {
                    projectileInitialDirection = Utilities::Math::NormalizeVector(projectileInitialDirection);
                }
                else {
                    RF_COMBAT_WARN("ProcessAbilityLaunchPhysicsProjectile: Target direction for ability %u is (near) zero. Defaulting to caster forward.", useAbilityIntent.ability_id);
                    projectileInitialDirection = Utilities::Math::GetWorldForwardVector(caster->orientation);
                    // Second normalization attempt if the forward vector itself might be zero (unlikely for a valid orientation)
                    if (Utilities::Math::Magnitude(projectileInitialDirection) < Utilities::Math::VECTOR_NORMALIZATION_EPSILON_SQ) {
                        RF_COMBAT_ERROR("ProcessAbilityLaunchPhysicsProjectile: Caster forward vector is zero for ability %u. Defaulting to Y-axis.", useAbilityIntent.ability_id);
                        projectileInitialDirection = Utilities::Math::Vec3(0.f, 1.f, 0.f); // Default world forward
                    }
                    else {
                        projectileInitialDirection = Utilities::Math::NormalizeVector(projectileInitialDirection);
                    }
                }

                // Hardcoded "Arrow" properties (replace with data from abilityDef later)
                Physics::ProjectilePhysicsProperties arrowPhysProps;
                arrowPhysProps.radius = 0.05f; arrowPhysProps.halfHeight = 0.0f; // Sphere projectile
                arrowPhysProps.mass = 0.1f;    arrowPhysProps.enableGravity = true;
                arrowPhysProps.enableCCD = true;

                float arrowSpeed = 40.0f;
                Networking::Shared::DamageInstance arrowDamageOnHit(20, Networking::Shared::DamageType::DamageType_Physical, false);
                std::string arrowVfxTag = "VFX_Arrow_Flying_Standard";
                float arrowMaxRangeOrLifetime = 100.0f; // Max travel distance or lifetime in seconds

                Utilities::Math::Vec3 initialVelocity = Utilities::Math::ScaleVector(projectileInitialDirection, arrowSpeed);
                uint64_t newProjectileId = playerManager.GetNextAvailableProjectileID(); // Assume this exists

                Physics::ProjectileGameData gameData(
                    newProjectileId, casterPlayerId, arrowDamageOnHit, arrowVfxTag, arrowMaxRangeOrLifetime
                );

                physx::PxRigidDynamic* projectileActor = physicsEngine.CreatePhysicsProjectileActor(
                    arrowPhysProps,
                    gameData,
                    Physics::EPhysicsObjectType::PROJECTILE, // Or use abilityDef to determine type
                    projectileStartPosition,   // This is Utilities::Math::Vec3
                    initialVelocity,           // This is Utilities::Math::Vec3
                    nullptr                    // Optional PxMaterial
                );

                if (projectileActor) {
                    outcome.success = true;
                    outcome.spawned_projectile = true;
                    outcome.projectile_id = newProjectileId;
                    outcome.projectile_owner_id = casterPlayerId; // Set owner ID

                    // CORRECTED: Direct assignment, as members of AttackOutcome are already GLM Vec3
                    outcome.projectile_start_position = projectileStartPosition;
                    outcome.projectile_direction = projectileInitialDirection;

                    outcome.projectile_speed = arrowSpeed;
                    outcome.projectile_max_range = arrowMaxRangeOrLifetime;
                    outcome.projectile_vfx_tag = arrowVfxTag;
                    outcome.projectile_damage_on_hit = arrowDamageOnHit; // This is FlatBuffers DamageInstance
                    // Typically, a projectile spawn itself isn't a "DamageDealt" event yet.
                    // It might be CombatEventType_ProjectileLaunched or similar.
                    outcome.simulated_combat_event_type = Networking::UDP::S2C::CombatEventType_None; // Or a specific projectile launch event type

                    // TODO: Apply cooldown for this ability to the caster
                    // caster->StartAbilityCooldown(abilityDef.id, abilityDef.cooldown);
                }
                else {
                    outcome.success = false;
                    outcome.failure_reason_code = "PROJECTILE_PHYSICS_CREATION_FAILED";
                }
                return outcome;
            }

        } // namespace Combat
    } // namespace GameLogic
} // namespace RiftForged