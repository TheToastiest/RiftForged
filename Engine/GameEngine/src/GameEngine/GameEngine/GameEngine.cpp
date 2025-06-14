#include <RiftForged/Engine/GameEngine/GameEngine.h>

// Include the new event definitions
#include <RiftForged/GameLogic/Events/CombatEvents/CombatEvents.h>
#include <RiftForged/GameLogic/Events/MovementEvents/MovementEvents.h>

#include <cmath>
#include <algorithm>
#include <random>

namespace RiftForged {
    namespace Gameplay {

        // --- Constructor ---
        GameplayEngine::GameplayEngine(
            Events::GameEventBus& eventBus,
            GameLogic::PlayerManager& playerManager,
            Physics::PhysicsEngine& physicsEngine)
            : m_eventBus(eventBus),
            m_playerManager(playerManager),
            m_physicsEngine(physicsEngine)
        {
            RF_GAMEPLAY_INFO("GameplayEngine: Initialized and ready.");
        }

        // --- Method Implementations ---

        void GameplayEngine::InitializePlayerInWorld(
            GameLogic::ActivePlayer* player,
            const Utilities::Math::Vec3& spawn_position,
            const Utilities::Math::Quaternion& spawn_orientation
        ) {
            if (!player || player->playerId == 0) {
                RF_GAMEPLAY_ERROR("GameplayEngine::InitializePlayerInWorld: Invalid player provided.");
                return;
            }

            player->SetPosition(spawn_position);
            player->SetOrientation(spawn_orientation);
            player->SetMovementState(GameLogic::PlayerMovementState::Idle);

            physx::PxController* controller = m_physicsEngine.CreateCharacterController(
                player->playerId,
                player->position,
                player->capsule_radius,
                player->capsule_half_height,
                nullptr,
                reinterpret_cast<void*>(player->playerId)
            );

            if (controller) {
                m_physicsEngine.SetCharacterControllerOrientation(player->playerId, player->orientation);
                RF_GAMEPLAY_INFO("Player {} PhysX controller created and initial pose set in world.", player->playerId);
            }
            else {
                RF_GAMEPLAY_ERROR("GameplayEngine: Failed to create PhysX controller for player {}.", player->playerId);
            }
        }

        void GameplayEngine::TurnPlayer(GameLogic::ActivePlayer* player, float turn_angle_degrees_delta) {
            if (!player) return;

            const Utilities::Math::Vec3 world_up_axis(0.0f, 0.0f, 1.0f); // Assuming Z-up
            Utilities::Math::Quaternion rotation_delta_q = Utilities::Math::FromAngleAxis(turn_angle_degrees_delta, world_up_axis);
            Utilities::Math::Quaternion new_orientation = Utilities::Math::MultiplyQuaternions(player->orientation, rotation_delta_q);
            player->SetOrientation(Utilities::Math::NormalizeQuaternion(new_orientation));
        }

        void GameplayEngine::ProcessMovement(
            GameLogic::ActivePlayer* player,
            const Utilities::Math::Vec3& local_desired_direction_from_client,
            bool is_sprinting,
            float delta_time_sec
        ) {
            // Your existing movement logic remains here, calling m_physicsEngine.MoveCharacterController
            // and player->SetPosition(...) as before.
            // This function does not typically publish a per-frame event. State synchronization
            // for continuous movement will be handled by a different mechanism.
        }

        void GameplayEngine::ExecuteRiftStep(
            GameLogic::ActivePlayer* player,
            GameLogic::Commands::RiftStepDirectionalIntent intent)
        {
            if (!player) return;

            // All of your existing logic from the old implementation for calculating the outcome remains.
            // This populates a local 'outcome' struct.
            GameLogic::RiftStepOutcome outcome = GameLogic::RiftStep::CalculateRiftStep(player, intent, m_physicsEngine);

            // Instead of returning, we now publish events based on the outcome.
            if (outcome.success) {
                // Apply state changes to the player and physics world first.
                m_physicsEngine.SetCharacterControllerPose(m_physicsEngine.GetPlayerController(player->playerId), outcome.actual_final_position);
                player->SetPosition(outcome.actual_final_position);
                player->SetAbilityCooldown(GameLogic::RIFTSTEP_ABILITY_ID, player->current_rift_step_definition.base_cooldown_sec);
                player->SetMovementState(GameLogic::PlayerMovementState::Idle);

                // Create and publish the success event for S2C formatters.
                GameLogic::Events::RiftStepExecuted event;
                event.instigatorEntityId = player->playerId;
                event.actualStartPosition = outcome.actual_start_position;
                event.calculatedTargetPosition = outcome.calculated_target_position;
                event.actualFinalPosition = outcome.actual_final_position;
                event.travelDurationSec = outcome.travel_duration_sec;
                // ... copy effect and vfx data ...
                m_eventBus.Publish(event);
            }
            else {
                // Create and publish a failure event.
                GameLogic::Events::PlayerRiftStepFailed event;
                event.playerId = player->playerId;
                event.reason = outcome.failure_reason_code;
                m_eventBus.Publish(event);
            }
        }

        void GameplayEngine::ExecuteBasicAttack(
            GameLogic::ActivePlayer* attacker,
            const GameLogic::Commands::BasicAttackIntent& command)
        {
            if (!attacker) return;

            // Your existing logic for calculating the attack outcome remains.
            GameLogic::AttackOutcome outcome = GameLogic::Combat::ProcessBasicMeleeAttack(
                attacker->playerId,
                command,
                m_playerManager,
                m_physicsEngine
            );

            // NEW: Instead of returning the outcome, we publish events.
            if (!outcome.success) {
                Events::PlayerBasicAttackFailed event;
                event.playerId = attacker->playerId;
                event.reason = outcome.failure_reason_code;
                m_eventBus.Publish(event);
                return;
            }

            // Announce any successful damage events.
            for (const auto& damageDetails : outcome.damage_events) {
                Events::EntityDealtDamage event;
                event.Details = damageDetails;
                m_eventBus.Publish(event);
            }

            // Announce if a projectile was spawned.
            if (outcome.spawned_projectile) {
                Events::ProjectileSpawned event;
                event.projectileId = outcome.projectile_id;
                event.ownerEntityId = outcome.projectile_owner_id;
                event.startPosition = outcome.projectile_start_position;
                event.initialDirection = outcome.projectile_direction;
                event.speed = outcome.projectile_speed;
                event.maxRange = outcome.projectile_max_range;
                event.projectileVfxTag = outcome.projectile_vfx_tag;
                // The pure GameLogic::Combat::DamageInstance is copied here
                // event.damageOnHit = outcome.projectile_damage_on_hit;
                m_eventBus.Publish(event);
            }
        }

        void GameplayEngine::UpdateWorldState(float deltaTime) {
            // This is where you would update systems that need a per-tick update,
            // such as AI, buffs/debuffs, cooldown timers, etc.
            // This method could also be responsible for publishing the periodic
            // EntityStateUpdated events for all "dirty" entities.
        }

    } // namespace Gameplay
} // namespace RiftForged