// File: GameplayEngine/GameplayEngine.h
// Copyright (c) 2023-2025 RiftForged Game Development Team
#pragma once

// Project-specific headers
#include <RiftForged/GameEngine/ActivePlayer/ActivePlayer.h>
#include <RiftForged/GameEngine/PlayerManager/PlayerManager.h> // For PlayerManager
#include <RiftForged/GameLogic/RiftStepLogic/RiftStepLogic.h>
#include <RiftForged/GameLogic/CombatData/CombatData.h>
#include <RiftForged/GameLogic/CombatSystem/CombatSystem.h> // For combat processing functions

#include <RiftForged/Physics/PhysicsEngine/PhysicsEngine.h>

// FlatBuffers generated headers (for enums and specific data structs) 
// REMOVED AND MOVED INTO DISPATCH LAYER REMEMBER TO DELETE
//#include "../FlatBuffers/Versioning/V0.0.5/riftforged_c2s_udp_messages_generated.h" // For C2S::RiftStepDirectionalIntent
//#include "../FlatBuffers/Versioning/V0.0.5/riftforged_s2c_udp_messages_generated.h" 
//#include "../FlatBuffers/Versioning/V0.0.5/riftforged_common_types_generated.h"    // For enums like DamageType if used directly
// REMOVED AND MOVED INTO DISPATCH LAYER REMEMBER TO DELETE

// Utility headers
#include <RiftForged/Utilities/MathUtils/MathUtils.h>
#include <RiftForged/Utilities/Logger/Logger.h>

namespace RiftForged {
    namespace Gameplay {

        class GameplayEngine {
        public:
            // Constructor injecting essential dependencies
            GameplayEngine(RiftForged::GameLogic::PlayerManager& playerManager,
                RiftForged::Physics::PhysicsEngine& physicsEngine);

            // Initialize player in the physics world
            // PARAMETER TYPES CHANGED to use GLM-based math types
            void InitializePlayerInWorld(
                RiftForged::GameLogic::ActivePlayer* player,
                const RiftForged::Utilities::Math::Vec3& spawn_position,
                const RiftForged::Utilities::Math::Quaternion& spawn_orientation
            );

            // GetPlayerManager
            RiftForged::GameLogic::PlayerManager& GetPlayerManager();

            // --- Player Actions ---

            // Handles player orientation changes based on client input
            void TurnPlayer(RiftForged::GameLogic::ActivePlayer* player, float turn_angle_degrees_delta);

            // Processes player movement input and interacts with the PhysicsEngine
            // PARAMETER TYPE CHANGED for local_desired_direction_from_client
            void ProcessMovement(
                RiftForged::GameLogic::ActivePlayer* player,
                const RiftForged::Utilities::Math::Vec3& local_desired_direction_from_client,
                bool is_sprinting,
                float delta_time_sec
            );

            // Orchestrates the RiftStep ability for a player
            // RiftStepDirectionalIntent is a FlatBuffers enum, so it remains.
            // RiftStepOutcome is assumed to now use GLM types internally.
            RiftForged::GameLogic::RiftStepOutcome ExecuteRiftStep(
                RiftForged::GameLogic::ActivePlayer* player,
                RiftForged::Networking::UDP::C2S::RiftStepDirectionalIntent intent
            );

            // Orchestrates a basic attack for a player
            // PARAMETER TYPE CHANGED for world_aim_direction
            RiftForged::GameLogic::AttackOutcome ExecuteBasicAttack(
                RiftForged::GameLogic::ActivePlayer* attacker,
                const RiftForged::Utilities::Math::Vec3& world_aim_direction,
                uint64_t optional_target_entity_id
            );

            // --- Potentially other gameplay logic methods ---
            // void UpdateGameWorld(float delta_time_sec); 
            // void ApplyStatusEffectToPlayer(GameLogic::ActivePlayer* player, Networking::Shared::StatusEffectCategory effect, uint32_t duration_ms, float magnitude);

            // Ability Placeholder Logic (Updated Vec3 parameters for future use)
            // struct BasicAttackTargetInfo { uint64_t targetId; }; // Example, if needed
            // SomeAbilityOutcome ExecuteSolarStrike(GameLogic::ActivePlayer* melee, const BasicAttackTargetInfo& target);
            // SomeAbilityOutcome ExecuteSolarFlare(GameLogic::ActivePlayer* caster, const RiftForged::Utilities::Math::Vec3& target_point);
            // SomeAbilityOutcome ExecuteGlacialBolt(GameLogic::ActivePlayer* caster, const RiftForged::Utilities::Math::Vec3& target_point);
            // SomeAbilityOutcome ExecuteGlacialStrike(GameLogic::ActivePlayer* melee, const BasicAttackTargetInfo& target);
            // SomeAbilityOutcome ExecuteVerdantStrike(GameLogic::ActivePlayer* melee, const BasicAttackTargetInfo& target);
            // SomeAbilityOutcome ExecuteVerdantLash(GameLogic::ActivePlayer* caster, const RiftForged::Utilities::Math::Vec3& target_point);
            // SomeAbilityOutcome ExecuteVerdantHealingLink(GameLogic::ActivePlayer* healer, const BasicAttackTargetInfo& target);
            // SomeAbilityOutcome ExecuteVerdantLifegivingBoon(GameLogic::ActivePlayer* healer, const RiftForged::Utilities::Math::Vec3& target_point);
            // SomeAbilityOutcome ExecuteRiftBolt(GameLogic::ActivePlayer* caster, const RiftForged::Utilities::Math::Vec3& target_point);
            // SomeAbilityOutcome ExecuteRiftStrike(GameLogic::ActivePlayer* melee, const BasicAttackTargetInfo& target);

        private:
            RiftForged::GameLogic::PlayerManager& m_playerManager;
            RiftForged::Physics::PhysicsEngine& m_physicsEngine;

            // --- Core Game Constants ---
            static constexpr float RIFTSTEP_MIN_COOLDOWN_SEC = 0.25f;
            static constexpr float BASE_WALK_SPEED_MPS = 3.0f;
            static constexpr float SPRINT_SPEED_MULTIPLIER = 1.5f;
        };

    } // namespace Gameplay
} // namespace RiftForged