#pragma once

// GameLogic and System dependencies
#include <RiftForged/GameLogic/PlayerManager/PlayerManager.h>
#include <RiftForged/GameLogic/CombatData/CombatData.h>
#include <RiftForged/GameLogic/Commands/GameCommands/GameCommands.h>
#include <RiftForged/GameLogic/RiftStepLogic/RiftStepLogic.h>
#include <RiftForged/GameLogic/CombatSystem/CombatSystem.h>
#include <RiftForged/Physics/PhysicsEngine/PhysicsEngine.h>
#include <RiftForged/Events/Core/GameEventBus/GameEventBus.h>

// Utility headers
#include <RiftForged/Utilities/MathUtils/MathUtils.h>
#include <RiftForged/Utilities/Logger/Logger.h>

namespace RiftForged {
    namespace Gameplay {

        class GameplayEngine {
        public:
            // Constructor is now injected with the GameEventBus
            GameplayEngine(
                Events::GameEventBus& eventBus,
                GameLogic::PlayerManager& playerManager,
                Physics::PhysicsEngine& physicsEngine
            );

            // Initializes a player's physical presence in the world
            void InitializePlayerInWorld(
                GameLogic::ActivePlayer* player,
                const Utilities::Math::Vec3& spawn_position,
                const Utilities::Math::Quaternion& spawn_orientation
            );

            // Handles player orientation changes from input
            void TurnPlayer(GameLogic::ActivePlayer* player, float turn_angle_degrees_delta);

            // Processes continuous player movement
            void ProcessMovement(
                GameLogic::ActivePlayer* player,
                const Utilities::Math::Vec3& local_desired_direction_from_client,
                bool is_sprinting,
                float delta_time_sec
            );

            // Executes the RiftStep ability and publishes the result as an event
            void ExecuteRiftStep(
                GameLogic::ActivePlayer* player,
                GameLogic::Commands::RiftStepDirectionalIntent intent
            );

            // Executes a basic attack and publishes the result as event(s)
            void ExecuteBasicAttack(
                GameLogic::ActivePlayer* attacker,
                const GameLogic::Commands::BasicAttackIntent& command
            );

            // A conceptual main update function for the engine, called by ShardEngine
            void UpdateWorldState(GameLogic::PlayerManager& playerManager, float deltaTime);


        private:
            // A reference to the event bus for publishing game events
            Events::GameEventBus& m_eventBus;

            // References to core systems
            GameLogic::PlayerManager& m_playerManager;
            Physics::PhysicsEngine& m_physicsEngine;

            // --- Core Game Constants ---
            static constexpr float RIFTSTEP_MIN_COOLDOWN_SEC = 0.25f;
            static constexpr float BASE_WALK_SPEED_MPS = 3.0f;
            static constexpr float SPRINT_SPEED_MULTIPLIER = 1.5f;
        };

    } // namespace Gameplay
} // namespace RiftForged