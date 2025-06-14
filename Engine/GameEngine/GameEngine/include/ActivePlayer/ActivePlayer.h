// File: Gameplay/ActivePlayer.h (Refactored)
// RiftForged Game Development Team
// Purpose: Defines the ActivePlayer class, representing a connected player's
//          state and capabilities within a game world instance.

#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <map>
#include <cstdint>
#include <mutex>      // For m_internalDataMutex
#include <algorithm>  // For std::max/min
#include <numeric>    // For std::accumulate (example)

// REMOVED AND PUT INTO NETWORKENGINE/DISPATCH LAYER
// Project-specific Networking & FlatBuffer Types (for data structures, not direct networking)
// These are for enums and specific data structures that might still be in FlatBuffers format.
//#include "../FlatBuffers/Versioning/V0.0.5/riftforged_common_types_generated.h" // For AnimationState, StatusEffectCategory, DamageType (enums)
//                                                                                // And IF RiftForged::Networking::Shared::DamageInstance is a FlatBuffers struct.
//#include "../FlatBuffers/Versioning/V0.0.5/riftforged_c2s_udp_messages_generated.h" // For C2S::RiftStepDirectionalIntent (as parameter type)
// REMOVED AND PUT INTO NETWORKENGINE/DISPATCH LAYER


// Project-specific Game Logic Types
// IMPORTANT: RiftStepLogic.h and the RiftStepOutcome struct it defines
// MUST also be updated to use RiftForged::Utilities::Math::Vec3/Quaternion for any positional/orientational data.
#include <RiftForged/GameLogic/RiftStepLogic/RiftStepLogic.h>  // For GameLogic::RiftStepOutcome, ERiftStepType, RiftStepDefinition

// Utilities
#include <RiftForged/Utilities/MathUtils/MathUtils.h>
#include <RiftForged/Utilities/Logger/Logger.h>

namespace RiftForged {
    namespace GameLogic {

        // Represents the current movement state of the player.
        enum class PlayerMovementState : uint8_t {
            Idle, Walking, Sprinting, Rifting, Ability_In_Use, Stunned, Rooted, Dead
        };

        // Represents the category of weapon the player has equipped.
        enum class EquippedWeaponCategory : uint8_t {
            Unarmed, Generic_Melee_Sword, Generic_Melee_Axe, Generic_Melee_Maul,
            Generic_Ranged_Bow, Generic_Ranged_Gun, Generic_Magic_Staff, Generic_Magic_Wand
        };

        // Ability IDs (ensure these are consistent across your game data)
        const uint32_t RIFTSTEP_ABILITY_ID = 1;
        const uint32_t BASIC_ATTACK_ABILITY_ID = 2;
        // ... other ability IDs ...

        struct ActivePlayer {
            // --- Core Identifiers ---
            uint64_t playerId;
            std::string characterName;

            // --- Transform State (Using GLM-based types) ---
            RiftForged::Utilities::Math::Vec3 position;
            RiftForged::Utilities::Math::Quaternion orientation;

            // --- Physics Properties ---
            float capsule_radius;
            float capsule_half_height;

            // --- Core Stats ---
            int32_t currentHealth;
            int32_t maxHealth;
            int32_t currentWill;
            uint32_t maxWill;

            // --- Combat Stats & Resistances ---
            float base_ability_cooldown_modifier;
            float base_critical_hit_chance_percent;
            float base_critical_hit_damage_multiplier;
            float base_accuracy_rating_percent;
            float base_basic_attack_cooldown_sec;

            int32_t flat_physical_damage_reduction;   float percent_physical_damage_reduction;
            int32_t flat_radiant_damage_reduction;    float percent_radiant_damage_reduction;
            int32_t flat_frost_damage_reduction;      float percent_frost_damage_reduction;
            int32_t flat_shock_damage_reduction;      float percent_shock_damage_reduction;
            int32_t flat_necrotic_damage_reduction;   float percent_necrotic_damage_reduction;
            int32_t flat_void_damage_reduction;       float percent_void_damage_reduction;
            int32_t flat_cosmic_damage_reduction;     float percent_cosmic_damage_reduction;
            int32_t flat_poison_damage_reduction;     float percent_poison_damage_reduction;
            int32_t flat_nature_damage_reduction;     float percent_nature_damage_reduction;
            int32_t flat_aetherial_damage_reduction;  float percent_aetherial_damage_reduction;

            // --- Equipment & Abilities ---
            EquippedWeaponCategory current_weapon_category;
            uint32_t equipped_weapon_definition_id;
            RiftStepDefinition current_rift_step_definition; // Ensure this struct also uses GLM types if it contains Vec3/Quat
            std::map<uint32_t, std::chrono::steady_clock::time_point> abilityCooldowns;

            // --- State Flags and Info ---
            PlayerMovementState movementState;
            uint32_t animationStateId; // Corresponds to Networking::Shared::AnimationState (FlatBuffers enum)
            std::vector<Networking::Shared::StatusEffectCategory> activeStatusEffects; // FlatBuffers enum
            std::atomic<bool> isDirty;

            // --- Input Intentions (Using GLM-based types) ---
            RiftForged::Utilities::Math::Vec3 last_processed_movement_intent;
            bool was_sprint_intended;

            // --- Synchronization ---
            mutable std::mutex m_internalDataMutex;

            // --- Constructor (Uses GLM-based types for startPos/Orientation) ---
            ActivePlayer(uint64_t pId,
                const RiftForged::Utilities::Math::Vec3& startPos = RiftForged::Utilities::Math::Vec3(0.f, 0.f, 1.f),
                const RiftForged::Utilities::Math::Quaternion& startOrientation = RiftForged::Utilities::Math::Quaternion(1.f, 0.f, 0.f, 0.f), // Identity: w,x,y,z
                float cap_radius = 0.5f, float cap_half_height = 0.9f);

            // --- Methods (Signatures updated to use GLM-based types) ---
            void SetPosition(const RiftForged::Utilities::Math::Vec3& newPosition);
            void SetOrientation(const RiftForged::Utilities::Math::Quaternion& newOrientation);

            void SetWill(int32_t value);
            void DeductWill(int32_t amount);
            void AddWill(int32_t amount);

            void SetHealth(int32_t value);
            void HealDamage(int32_t amount);
            int32_t TakeDamage(int32_t raw_damage_amount, Networking::Shared::DamageType damage_type); // damage_type is FlatBuffers enum

            void SetAnimationState(Networking::Shared::AnimationState newState); // newState is FlatBuffers enum
            void SetAnimationStateId(uint32_t newStateId);
            void SetMovementState(PlayerMovementState newState);

            bool IsAbilityOnCooldown(uint32_t abilityId) const;
            void StartAbilityCooldown(uint32_t abilityId, float base_duration_sec);
            void SetAbilityCooldown(uint32_t abilityId, float cooldown_sec) {
                StartAbilityCooldown(abilityId, cooldown_sec);
            }

            void UpdateActiveRiftStepDefinition(const RiftStepDefinition& new_definition);
            bool CanPerformRiftStep() const;

            // directional_intent is FlatBuffers enum
            // RiftStepOutcome struct needs its Vec3/Quat members updated to GLM types
            RiftStepOutcome PrepareRiftStepOutcome(Networking::UDP::C2S::RiftStepDirectionalIntent directional_intent, ERiftStepType type_requested);

            void AddStatusEffects(const std::vector<Networking::Shared::StatusEffectCategory>& effects_to_add); // FlatBuffers enum
            void RemoveStatusEffects(const std::vector<Networking::Shared::StatusEffectCategory>& effects_to_remove); // FlatBuffers enum
            bool HasStatusEffect(Networking::Shared::StatusEffectCategory effect) const; // FlatBuffers enum

            void SetEquippedWeapon(uint32_t weapon_def_id, EquippedWeaponCategory category);
            RiftForged::Utilities::Math::Vec3 GetMuzzlePosition() const; // Returns GLM type

            void MarkDirty();
        };

    } // namespace GameLogic
} // namespace RiftForged