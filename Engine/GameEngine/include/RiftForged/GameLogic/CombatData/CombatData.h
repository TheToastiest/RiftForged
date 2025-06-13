// File: GameplayEngine/CombatData.h
// Copyright (c) 2023-2025 RiftForged Game Development Team
// Description: Defines structures for combat outcomes and related logic.
#pragma once

#include <string>
#include <vector>
#include <cstdint>

// Utility for GLM-based Vec3/Quaternion
#include <RiftForged/Utilities/MathUtils/MathUtils.h> // For Vec3, Quaternion

// FlatBuffers types (for enums and specific data structs like DamageInstance)
//#include "../FlatBuffers/Versioning/V0.0.5/riftforged_common_types_generated.h" // For DamageType, DamageInstance, StunInstance etc.
//#include "../FlatBuffers/Versioning/V0.0.5/riftforged_s2c_udp_messages_generated.h" // For S2C::CombatEventType
//#include "../FlatBuffers/Versioning/V0.0.5/riftforged_c2s_udp_messages_generated.h" // For C2S::CombatEventType (if needed directly here)

namespace RiftForged {
    namespace GameLogic {

        // Details for a single instance of damage being applied to a target
        struct DamageApplicationDetails {
            uint64_t target_id = 0;
            uint64_t source_id = 0; // ID of the entity that dealt the damage
            int32_t final_damage_dealt = 0;
            RiftForged::Networking::Shared::DamageType damage_type = RiftForged::Networking::Shared::DamageType_None; // From FlatBuffers
            bool was_crit = false;
            bool was_kill = false;
            // Optional: if needed for VFX precision. Changed to GLM-based Vec3.
            RiftForged::Utilities::Math::Vec3 impact_point{ 0.0f, 0.0f, 0.0f };

            DamageApplicationDetails() :
                target_id(0),
                source_id(0),
                final_damage_dealt(0),
                damage_type(RiftForged::Networking::Shared::DamageType_None),
                was_crit(false),
                was_kill(false),
                impact_point(0.0f, 0.0f, 0.0f) // Initialize GLM Vec3
            {
            }
        };


        // Outcome of a basic attack or combat ability execution
        struct AttackOutcome {
            bool success = true;
            std::string failure_reason_code;

            bool is_basic_attack = false;
            // Assuming CombatEventType_None exists for a more descriptive default than _MIN
            RiftForged::Networking::UDP::S2C::CombatEventType simulated_combat_event_type = RiftForged::Networking::UDP::S2C::CombatEventType_None;

            std::string attack_animation_tag_for_caster;

            std::vector<DamageApplicationDetails> damage_events; // Contains impact_point which is now GLM Vec3

            // For Ranged Projectile Basic Attacks / Abilities
            bool spawned_projectile = false;
            uint64_t projectile_id = 0;
            uint64_t projectile_owner_id = 0;
            RiftForged::Utilities::Math::Vec3 projectile_start_position; // Already GLM Vec3
            RiftForged::Utilities::Math::Vec3 projectile_direction;    // Already GLM Vec3 (normalized)
            float projectile_speed = 0.0f;
            float projectile_max_range = 0.0f;
            std::string projectile_vfx_tag;
            RiftForged::Networking::Shared::DamageInstance projectile_damage_on_hit; // FlatBuffers struct

            AttackOutcome() :
                success(false), // Default to false, set to true upon successful preparation
                simulated_combat_event_type(RiftForged::Networking::UDP::S2C::CombatEventType_None),
                is_basic_attack(false),
                spawned_projectile(false),
                projectile_id(0),
                projectile_owner_id(0),
                projectile_start_position(0.f, 0.f, 0.f),   // Initializes GLM Vec3
                projectile_direction(0.f, 1.f, 0.f),      // Initializes GLM Vec3 (default forward)
                projectile_speed(0.f),
                projectile_max_range(0.f)
                // projectile_damage_on_hit is default constructed (FlatBuffers struct)
            {
            }
        };

    } // namespace GameLogic
} // namespace RiftForged