// File: GameEngine/GameEngine.cpp
//  RiftForged Game Development Team
// Copyright (c) 2023-2025 RiftForged Game Development Team

#include <RiftForged/GameEngine/GameEngine/GameEngine.h>
// ActivePlayer.h, PlayerManager.h, RiftStepLogic.h, CombatData.h, CombatSystem.h
// PhysicsEngine.h, MathUtil.h, Logger.h, and FlatBuffers are included via GameplayEngine.h

#include <cmath>     // For std::abs, std::cos
#include <algorithm> // For std::max, std::min
#include <random>    // For stubbed damage roll

// Assuming RIFTSTEP_ABILITY_ID and BASIC_ATTACK_ABILITY_ID are correctly scoped or globally available
// If they are in GameLogic namespace and defined in ActivePlayer.h, they'd be GameLogic::RIFTSTEP_ABILITY_ID

namespace RiftForged {
    namespace Gameplay {

        // --- Temporary Stub for Weapon Properties ---
        struct TempWeaponProperties {
            bool isMelee;
            float range;
            float attackCooldownSec;
            RiftForged::Networking::Shared::DamageInstance baseDamageInstance; // FlatBuffers struct
            float projectileSpeed;
            std::string projectileVfxTag;
            // Add projectile physics properties if needed
            // Physics::PhysicsEngine::ProjectilePhysicsProperties projectilePhysProps;
        };

        // Helper to get stubbed weapon properties (uses FlatBuffers DamageInstance)
        static TempWeaponProperties GetStubbedWeaponProperties(GameLogic::ActivePlayer* attacker) {
            using namespace RiftForged::Networking::Shared; // For DamageType, DamageInstance
            using RiftForged::GameLogic::EquippedWeaponCategory;

            float base_player_attack_cooldown = attacker ? attacker->base_basic_attack_cooldown_sec : 1.0f;
            EquippedWeaponCategory category = attacker ? attacker->current_weapon_category : EquippedWeaponCategory::Unarmed;

            auto create_dmg_inst = [&](int min_dmg, int max_dmg, DamageType type) {
                int amount = min_dmg + (max_dmg > min_dmg ? (rand() % (max_dmg - min_dmg + 1)) : 0);
                return DamageInstance(amount, type, false); // is_crit = false for base
                };

            switch (category) {
            case EquippedWeaponCategory::Generic_Melee_Sword:
            case EquippedWeaponCategory::Generic_Melee_Axe:
                return { true, 2.5f, base_player_attack_cooldown, create_dmg_inst(10, 15, DamageType::DamageType_Physical), 0.f, "" };
            case EquippedWeaponCategory::Generic_Melee_Maul:
                return { true, 3.0f, base_player_attack_cooldown * 1.2f, create_dmg_inst(15, 25, DamageType::DamageType_Physical), 0.f, "" };
            case EquippedWeaponCategory::Generic_Ranged_Bow:
                return { false, 30.0f, base_player_attack_cooldown, create_dmg_inst(12, 18, DamageType::DamageType_Physical), 40.f, "VFX_Projectile_Arrow" };
            case EquippedWeaponCategory::Generic_Ranged_Gun:
                return { false, 25.0f, base_player_attack_cooldown * 0.8f, create_dmg_inst(8, 12, DamageType::DamageType_Physical), 50.f, "VFX_Projectile_Bullet" };
            case EquippedWeaponCategory::Generic_Magic_Staff:
                return { false, 20.0f, base_player_attack_cooldown, create_dmg_inst(10, 16, DamageType::DamageType_Radiant), 30.f, "VFX_Magic_Bolt_Staff" };
            case EquippedWeaponCategory::Generic_Magic_Wand:
                return { false, 18.0f, base_player_attack_cooldown * 0.7f, create_dmg_inst(7, 11, DamageType::DamageType_Cosmic), 35.f, "VFX_Magic_Bolt_Wand" };
            case EquippedWeaponCategory::Unarmed:
            default:
                return { true, 1.5f, base_player_attack_cooldown, create_dmg_inst(1, 3, DamageType::DamageType_Physical), 0.f, "" };
            }
        }

        // --- Constructor ---
        GameplayEngine::GameplayEngine(RiftForged::GameLogic::PlayerManager& playerManager,
            RiftForged::Physics::PhysicsEngine& physicsEngine)
            : m_playerManager(playerManager),
            m_physicsEngine(physicsEngine) {
            RF_GAMEPLAY_INFO("GameplayEngine: Initialized and ready.");
        }

        RiftForged::GameLogic::PlayerManager& GameplayEngine::GetPlayerManager() {
            return m_playerManager;
        }

        // --- Initialize Players ---
        // PARAMETER TYPES CHANGED to use GLM-based math types
        void GameplayEngine::InitializePlayerInWorld(
            RiftForged::GameLogic::ActivePlayer* player,
            const RiftForged::Utilities::Math::Vec3& spawn_position,         // Now GLM Vec3
            const RiftForged::Utilities::Math::Quaternion& spawn_orientation // Now GLM Quaternion
        ) {
            if (!player) {
                RF_GAMEPLAY_ERROR("GameplayEngine::InitializePlayerInWorld: Null player pointer provided.");
                return;
            }
            if (player->playerId == 0) {
                RF_GAMEPLAY_ERROR("GameplayEngine::InitializePlayerInWorld: Attempted to initialize player with ID 0.");
                return;
            }

            // Logging GLM types using .x, .y, .z, .w
            RF_GAMEPLAY_INFO("GameplayEngine: Initializing player {} in world at Pos({:.2f}, {:.2f}, {:.2f}) Orient({:.2f},{:.2f},{:.2f},{:.2f})",
                player->playerId,
                spawn_position.x, spawn_position.y, spawn_position.z,
                spawn_orientation.x, spawn_orientation.y, spawn_orientation.z, spawn_orientation.w);

            // ActivePlayer::SetPosition and SetOrientation now expect GLM types
            player->SetPosition(spawn_position);
            player->SetOrientation(spawn_orientation);
            player->SetMovementState(RiftForged::GameLogic::PlayerMovementState::Idle);
            player->SetAnimationState(RiftForged::Networking::Shared::AnimationState::AnimationState_Idle);

            if (player->capsule_radius <= 0.0f || player->capsule_half_height <= 0.0f) {
                RF_GAMEPLAY_ERROR("GameplayEngine::InitializePlayerInWorld: Player {} has invalid capsule dimensions (R: {:.2f}, HH: {:.2f}). Cannot create controller.",
                    player->playerId, player->capsule_radius, player->capsule_half_height);
                return;
            }

            // PhysicsEngine::CreateCharacterController and SetCharacterControllerOrientation now expect GLM types
            // player->position and player->orientation are already GLM types within ActivePlayer
            physx::PxController* controller = m_physicsEngine.CreateCharacterController(
                player->playerId,
                player->position,           // Pass GLM Vec3 (ActivePlayer's internal position)
                player->capsule_radius,
                player->capsule_half_height, // PhysicsEngine's CreateCharacterController now takes half_height as per last update to its signature.
                // If it still expects full height: player->capsule_half_height * 2.0f
                nullptr,                     // Use default PxMaterial from PhysicsEngine
                reinterpret_cast<void*>(player->playerId)
            );

            if (controller) {
                bool orientation_set = m_physicsEngine.SetCharacterControllerOrientation(player->playerId, player->orientation); // Pass GLM Quaternion

                if (orientation_set) {
                    RF_GAMEPLAY_INFO("Player {} PhysX controller created and initial pose set in world.", player->playerId);
                }
                else {
                    RF_GAMEPLAY_WARN("Player {} PhysX controller created, but failed to set initial orientation in physics world.", player->playerId);
                }
            }
            else {
                RF_GAMEPLAY_ERROR("GameplayEngine: Failed to create PhysX controller for player {}. Player will lack physics presence.", player->playerId);
            }
        }

        // --- Player Actions Implementations ---

        void GameplayEngine::TurnPlayer(RiftForged::GameLogic::ActivePlayer* player, float turn_angle_degrees_delta) {
            if (!player) {
                RF_GAMEPLAY_ERROR("GameplayEngine::TurnPlayer: Called with null player.");
                return;
            }
            // world_up_axis should be Utilities::Math::Vec3
            const RiftForged::Utilities::Math::Vec3 world_up_axis(0.0f, 0.0f, 1.0f); // Assuming Z-up
            // FromAngleAxis returns GLM Quaternion
            RiftForged::Utilities::Math::Quaternion rotation_delta_q =
                RiftForged::Utilities::Math::FromAngleAxis(turn_angle_degrees_delta, world_up_axis);

            // player->orientation is GLM Quat, MultiplyQuaternions takes and returns GLM Quat
            RiftForged::Utilities::Math::Quaternion new_orientation =
                RiftForged::Utilities::Math::MultiplyQuaternions(player->orientation, rotation_delta_q);

            // player->SetOrientation expects GLM Quat, NormalizeQuaternion takes and returns GLM Quat
            player->SetOrientation(RiftForged::Utilities::Math::NormalizeQuaternion(new_orientation));
        }

        // PARAMETER TYPE CHANGED for local_desired_direction_from_client
        void GameplayEngine::ProcessMovement(
            RiftForged::GameLogic::ActivePlayer* player,
            const RiftForged::Utilities::Math::Vec3& local_desired_direction_from_client, // Now GLM Vec3
            bool is_sprinting,
            float delta_time_sec) {

            if (!player) { /* ... error handling ... */ return; }
            if (player->playerId == 0) { /* ... error handling ... */ return; }
            if (player->movementState == RiftForged::GameLogic::PlayerMovementState::Stunned ||
                player->movementState == RiftForged::GameLogic::PlayerMovementState::Rooted ||
                player->movementState == RiftForged::GameLogic::PlayerMovementState::Dead) {
                return;
            }
            if (delta_time_sec <= 0.0f) return;

            float current_base_speed = BASE_WALK_SPEED_MPS;
            float actual_speed = current_base_speed * (is_sprinting ? SPRINT_SPEED_MULTIPLIER : 1.0f);
            float displacement_amount = actual_speed * delta_time_sec;

            // local_desired_direction_from_client is now GLM Vec3, use .x, .y, .z
            if ((std::abs(local_desired_direction_from_client.x) < 1e-6f &&
                std::abs(local_desired_direction_from_client.y) < 1e-6f &&
                std::abs(local_desired_direction_from_client.z) < 1e-6f) || // Assuming Z can be part of local intent
                displacement_amount < 0.0001f) {
                if (player->movementState == RiftForged::GameLogic::PlayerMovementState::Walking || player->movementState == RiftForged::GameLogic::PlayerMovementState::Sprinting) {
                    player->SetMovementState(RiftForged::GameLogic::PlayerMovementState::Idle);
                }
                return;
            }

            // All math ops now use GLM types
            RiftForged::Utilities::Math::Vec3 normalized_local_dir = RiftForged::Utilities::Math::NormalizeVector(local_desired_direction_from_client);
            RiftForged::Utilities::Math::Vec3 world_move_direction =
                RiftForged::Utilities::Math::RotateVectorByQuaternion(normalized_local_dir, player->orientation);

            RiftForged::Utilities::Math::Vec3 displacement_vector =
                RiftForged::Utilities::Math::ScaleVector(world_move_direction, displacement_amount);

            physx::PxController* px_controller = m_physicsEngine.GetPlayerController(player->playerId);

            if (px_controller) {
                std::vector<physx::PxController*> other_controllers_to_ignore;
                // PhysicsEngine::MoveCharacterController expects GLM Vec3
                physx::PxU32 collisionFlagsU32 = m_physicsEngine.MoveCharacterController(
                    px_controller,
                    displacement_vector, // Pass GLM Vec3
                    delta_time_sec,
                    other_controllers_to_ignore
                );
                physx::PxControllerCollisionFlags collisionFlags = static_cast<physx::PxControllerCollisionFlags>(collisionFlagsU32);


                // PhysicsEngine::GetCharacterControllerPosition returns GLM Vec3
                RiftForged::Utilities::Math::Vec3 new_pos_from_physics = m_physicsEngine.GetCharacterControllerPosition(px_controller);
                player->SetPosition(new_pos_from_physics); // ActivePlayer::SetPosition expects GLM Vec3

                // Logging GLM Vec3
                RF_GAMEPLAY_DEBUG("GameplayEngine: Player {} new position after PhysX move: ({:.2f}, {:.2f}, {:.2f})",
                    player->playerId, new_pos_from_physics.x, new_pos_from_physics.y, new_pos_from_physics.z);

                if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_SIDES) { /* ... */ }
                // ... other collision flag checks ...
            }
            else {
                RF_GAMEPLAY_WARN("Player {} ProcessMovement - PhysX controller not found! Using direct kinematic move.", player->playerId);
                // Fallback kinematic move: player->position and displacement_vector are GLM Vec3
                RiftForged::Utilities::Math::Vec3 new_pos_direct = RiftForged::Utilities::Math::AddVectors(player->position, displacement_vector);
                player->SetPosition(new_pos_direct);
            }
            player->SetMovementState(is_sprinting ? RiftForged::GameLogic::PlayerMovementState::Sprinting : RiftForged::GameLogic::PlayerMovementState::Walking);
        }

        RiftForged::GameLogic::RiftStepOutcome GameplayEngine::ExecuteRiftStep(
            RiftForged::GameLogic::ActivePlayer* player,
            RiftForged::Networking::UDP::C2S::RiftStepDirectionalIntent intent) {

            RiftForged::GameLogic::RiftStepOutcome outcome;
            if (!player) { /* ... error handling ... */ return outcome; }
            if (player->playerId == 0) { /* ... error handling ... */ return outcome; }
            if (!player->CanPerformRiftStep()) { /* ... error handling ... */ return outcome; }

            outcome = player->PrepareRiftStepOutcome(intent, player->current_rift_step_definition.type);
            if (!outcome.success) { /* ... error handling ... */ return outcome; }

            physx::PxController* px_controller = m_physicsEngine.GetPlayerController(player->playerId);
            physx::PxRigidActor* player_actor_to_ignore = nullptr;
            if (px_controller) { player_actor_to_ignore = px_controller->getActor(); }
            else { /* ... error handling, return outcome ... */ return outcome; }

            const float player_capsule_radius = player->capsule_radius;
            const float player_capsule_half_height = player->capsule_half_height;

            // All these are now GLM Vec3 operations as RiftStepOutcome members are GLM Vec3
            RiftForged::Utilities::Math::Vec3 travel_direction_unit =
                RiftForged::Utilities::Math::SubtractVectors(outcome.intended_target_position, outcome.actual_start_position);
            float max_travel_distance = RiftForged::Utilities::Math::Magnitude(travel_direction_unit);
            outcome.actual_final_position = outcome.actual_start_position;

            if (max_travel_distance > 0.001f) {
                travel_direction_unit = RiftForged::Utilities::Math::ScaleVector(travel_direction_unit, 1.0f / max_travel_distance);
                RiftForged::Physics::HitResult hit_result; // HitResult members are GLM Vec3
                physx::PxQueryFilterData physx_query_filter_data; // Default flags, or configure as needed
                physx_query_filter_data.flags = physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::ePREFILTER | physx::PxQueryFlag::ePOSTFILTER;


                // PhysicsEngine::CapsuleSweepSingle expects GLM types for positions/orientations
                bool found_blocking_hit = m_physicsEngine.CapsuleSweepSingle(
                    outcome.actual_start_position, // GLM Vec3
                    player->orientation,           // GLM Quaternion
                    player_capsule_radius,
                    player_capsule_half_height,
                    travel_direction_unit,         // GLM Vec3
                    max_travel_distance,
                    hit_result,
                    player_actor_to_ignore,
                    physx_query_filter_data
                );

                if (found_blocking_hit) {
                    float safe_distance = std::max(0.0f, hit_result.distance - (player_capsule_radius * 0.1f));
                    outcome.actual_final_position = RiftForged::Utilities::Math::AddVectors(
                        outcome.actual_start_position,
                        RiftForged::Utilities::Math::ScaleVector(travel_direction_unit, safe_distance)
                    );
                    // Logging GLM Vec3s with .x, .y, .z
                    RF_GAMEPLAY_INFO("Player {} RiftStep OBSTRUCTED. Intended: ({:.1f},{:.1f},{:.1f}), Actual: ({:.1f},{:.1f},{:.1f}) at dist {:.2f}",
                        player->playerId,
                        outcome.intended_target_position.x, outcome.intended_target_position.y, outcome.intended_target_position.z,
                        outcome.actual_final_position.x, outcome.actual_final_position.y, outcome.actual_final_position.z,
                        safe_distance);
                }
                else {
                    outcome.actual_final_position = outcome.intended_target_position;
                    RF_GAMEPLAY_INFO("Player {} RiftStep path clear. Final Pos: ({:.1f},{:.1f},{:.1f})",
                        player->playerId, outcome.actual_final_position.x, outcome.actual_final_position.y, outcome.actual_final_position.z);
                }
            }
            else {
                RF_GAMEPLAY_INFO("Player {} RiftStep: No significant travel distance.", player->playerId);
            }

            m_physicsEngine.SetCharacterControllerPose(px_controller, outcome.actual_final_position); // Expects GLM Vec3
            player->SetPosition(outcome.actual_final_position); // Expects GLM Vec3
            player->SetAbilityCooldown(RiftForged::GameLogic::RIFTSTEP_ABILITY_ID, player->current_rift_step_definition.base_cooldown_sec);

            // Logging GLM Vec3s with .x, .y, .z
            RF_GAMEPLAY_INFO("Player {} RiftStep EXECUTED. Type: {}. Effects: Entry({}), Exit({}). Target: ({:.1f},{:.1f},{:.1f}), Final: ({:.1f},{:.1f},{:.1f})",
                player->playerId, static_cast<int>(outcome.type_executed),
                outcome.entry_effects_data.size(), outcome.exit_effects_data.size(),
                outcome.intended_target_position.x, outcome.intended_target_position.y, outcome.intended_target_position.z,
                outcome.actual_final_position.x, outcome.actual_final_position.y, outcome.actual_final_position.z
            );

            player->SetMovementState(RiftForged::GameLogic::PlayerMovementState::Idle);
            player->SetAnimationState(RiftForged::Networking::Shared::AnimationState::AnimationState_Rifting_End);
            outcome.success = true;
            return outcome;
        }

        // PARAMETER TYPE CHANGED for world_aim_direction
        RiftForged::GameLogic::AttackOutcome GameplayEngine::ExecuteBasicAttack(
            RiftForged::GameLogic::ActivePlayer* attacker,
            const RiftForged::Utilities::Math::Vec3& world_aim_direction, // Now GLM Vec3
            uint64_t optional_target_entity_id) {

            using namespace RiftForged::GameLogic; // For PlayerMovementState, ActivePlayer, AttackOutcome etc.
            using namespace RiftForged::Networking::Shared; // For DamageType, DamageInstance, AnimationState
            using namespace RiftForged::Networking::UDP::S2C; // For CombatEventType

            AttackOutcome outcome;
            outcome.is_basic_attack = true;

            if (!attacker) { /* ... error handling ... */ outcome.success = false; return outcome; }
            // ... (rest of preconditions as before) ...

            TempWeaponProperties weapon_props = GetStubbedWeaponProperties(attacker);
            if (attacker->IsAbilityOnCooldown(GameLogic::BASIC_ATTACK_ABILITY_ID)) { /* ... error handling ... */ outcome.success = false; return outcome; }
            attacker->SetAbilityCooldown(GameLogic::BASIC_ATTACK_ABILITY_ID, weapon_props.attackCooldownSec);
            attacker->SetMovementState(PlayerMovementState::Ability_In_Use); // Or a specific "Attacking" state
            attacker->SetAnimationState(AnimationState::AnimationState_Attacking_Primary); // Example

            // ... (switch for attack_animation_tag_for_caster as before) ...

            if (weapon_props.isMelee) {
                outcome.simulated_combat_event_type = CombatEventType_Miss;
                ActivePlayer* target_player = nullptr;
                if (optional_target_entity_id != 0 && optional_target_entity_id != attacker->playerId) {
                    target_player = m_playerManager.FindPlayerById(optional_target_entity_id);
                }

                if (target_player && target_player->movementState != PlayerMovementState::Dead) {
                    // All vector math here now uses GLM types because attacker->position, target_player->position,
                    // and world_aim_direction are all GLM Vec3.
                    float dist_sq = Utilities::Math::DistanceSquared(attacker->position, target_player->position);
                    Utilities::Math::Vec3 dir_to_target = Utilities::Math::NormalizeVector(Utilities::Math::SubtractVectors(target_player->position, attacker->position));
                    Utilities::Math::Vec3 normalized_aim_dir = Utilities::Math::NormalizeVector(world_aim_direction);
                    float dot_product = Utilities::Math::DotProduct(normalized_aim_dir, dir_to_target);

                    if (dist_sq <= weapon_props.range * weapon_props.range && dot_product > 0.707f) {
                        DamageApplicationDetails hit_details;
                        // ... (damage calculation as before) ...
                        hit_details.final_damage_dealt = weapon_props.baseDamageInstance.amount(); // Example
                        hit_details.damage_type = weapon_props.baseDamageInstance.type();
                        // ...
                        outcome.damage_events.push_back(hit_details);
                        outcome.simulated_combat_event_type = CombatEventType_DamageDealt;
                    } // ...
                } // ...
            }
            else { // Ranged Attack
                outcome.simulated_combat_event_type = CombatEventType_None; // Or ProjectileLaunched
                outcome.spawned_projectile = true;
                outcome.projectile_id = m_playerManager.GetNextAvailableProjectileID();
                outcome.projectile_owner_id = attacker->playerId; // Set owner
                // GetMuzzlePosition and attacker->orientation are GLM types
                outcome.projectile_start_position = attacker->GetMuzzlePosition();
                outcome.projectile_direction = Utilities::Math::NormalizeVector(world_aim_direction); // world_aim_direction is GLM
                outcome.projectile_speed = weapon_props.projectileSpeed;
                outcome.projectile_max_range = weapon_props.range;
                outcome.projectile_vfx_tag = weapon_props.projectileVfxTag;
                outcome.projectile_damage_on_hit = weapon_props.baseDamageInstance;

                RF_GAMEPLAY_INFO("Player {} Basic Attack: SPAWNED Projectile ID {} (Dmg: {}, Type: {})",
                    attacker->playerId, outcome.projectile_id,
                    outcome.projectile_damage_on_hit.amount(), EnumNameDamageType(outcome.projectile_damage_on_hit.type()));
            }

            outcome.success = true;
            if (attacker->movementState == PlayerMovementState::Ability_In_Use) {
                attacker->SetMovementState(PlayerMovementState::Idle);
            }
            return outcome;
        }

    } // namespace Gameplay
} // namespace RiftForged