// File: GameplayEngine/RiftStepLogic.h
// Copyright (c) 2023-2025 RiftForged Game Development Team
// Description: Defines the core logic, types, and data structures for the RiftStep ability.
#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>

// Utility for GLM-based Vec3/Quaternion
#include <RiftForged/Utilities/MathUtils/MathUtils.h> // For Vec3, Quaternion

// FlatBuffers types (for enums and specific data structs like DamageInstance, not for Vec3/Quat in calculations)
//#include "../FlatBuffers/Versioning/V0.0.5/riftforged_common_types_generated.h"
//#include "../FlatBuffers/Versioning/V0.0.5/riftforged_s2c_udp_messages_generated.h"
//#include "../FlatBuffers/Versioning/V0.0.5/riftforged_c2s_udp_messages_generated.h"

namespace RiftForged {
    namespace GameLogic {

        // GameplayEffectInstance struct (defines a specific gameplay effect that can be triggered)
        struct GameplayEffectInstance {
            RiftForged::Networking::UDP::S2C::RiftStepEffectPayload effect_payload_type;
            RiftForged::Utilities::Math::Vec3 center_position; // This was already correct! (glm::vec3)
            float radius = 0.0f;
            uint32_t duration_ms = 0;
            RiftForged::Networking::Shared::DamageInstance damage; // FlatBuffers struct
            RiftForged::Networking::Shared::StunInstance stun;     // FlatBuffers struct
            RiftForged::Networking::Shared::StatusEffectCategory buff_debuff_to_apply; // FlatBuffers enum
            std::string visual_effect_tag;
            std::optional<std::vector<Networking::Shared::StatusEffectCategory>> persistent_area_applied_effects;

            GameplayEffectInstance() :
                effect_payload_type(RiftForged::Networking::UDP::S2C::RiftStepEffectPayload_NONE),
                center_position(0.0f, 0.0f, 0.0f), // Initializes glm::vec3
                radius(0.0f),
                duration_ms(0),
                damage(0, RiftForged::Networking::Shared::DamageType::DamageType_MIN, false), // Assuming DamageType_MIN is like 'None'
                stun(RiftForged::Networking::Shared::StunSeverity::StunSeverity_MIN, 0),   // Assuming StunSeverity_MIN is like 'None'
                buff_debuff_to_apply(RiftForged::Networking::Shared::StatusEffectCategory::StatusEffectCategory_None),
                visual_effect_tag("") {
            }

            // Convenience constructor for an Area Damage effect
            // PARAMETER TYPE CHANGED for center
            GameplayEffectInstance(const RiftForged::Utilities::Math::Vec3& center, float rad,
                const RiftForged::Networking::Shared::DamageInstance& dmg_instance)
                : effect_payload_type(RiftForged::Networking::UDP::S2C::RiftStepEffectPayload::RiftStepEffectPayload_AreaDamage),
                center_position(center), // Now glm::vec3 to glm::vec3 assignment
                radius(rad),
                damage(dmg_instance),
                duration_ms(0),
                stun(RiftForged::Networking::Shared::StunSeverity::StunSeverity_MIN, 0),
                buff_debuff_to_apply(RiftForged::Networking::Shared::StatusEffectCategory::StatusEffectCategory_None),
                visual_effect_tag("") {
            }

            // Convenience constructor for an Area Stun effect
            // PARAMETER TYPE CHANGED for center
            GameplayEffectInstance(const RiftForged::Utilities::Math::Vec3& center, float rad,
                const RiftForged::Networking::Shared::StunInstance& stun_instance)
                : effect_payload_type(RiftForged::Networking::UDP::S2C::RiftStepEffectPayload::RiftStepEffectPayload_AreaStun),
                center_position(center), // Now glm::vec3 to glm::vec3 assignment
                radius(rad),
                stun(stun_instance),
                duration_ms(0),
                damage(0, RiftForged::Networking::Shared::DamageType::DamageType_MIN, false),
                buff_debuff_to_apply(RiftForged::Networking::Shared::StatusEffectCategory::StatusEffectCategory_None),
                visual_effect_tag("") {
            }

            // Convenience constructor for ApplyBuffDebuff effect (Area of Effect)
            // PARAMETER TYPE CHANGED for center
            GameplayEffectInstance(const RiftForged::Utilities::Math::Vec3& center, float rad, uint32_t effect_duration_ms,
                RiftForged::Networking::Shared::StatusEffectCategory effect_to_apply, const std::string& vfx_tag = "")
                : effect_payload_type(RiftForged::Networking::UDP::S2C::RiftStepEffectPayload::RiftStepEffectPayload_MIN), // Corrected enum name
                center_position(center), // Now glm::vec3 to glm::vec3 assignment
                radius(rad),
                duration_ms(effect_duration_ms),
                buff_debuff_to_apply(effect_to_apply),
                visual_effect_tag(vfx_tag),
                damage(0, RiftForged::Networking::Shared::DamageType::DamageType_MIN, false),
                stun(RiftForged::Networking::Shared::StunSeverity::StunSeverity_MIN, 0) {
            }

            // Convenience constructor for PersistentArea effect
            // PARAMETER TYPE CHANGED for center
            GameplayEffectInstance(const RiftForged::Utilities::Math::Vec3& center, float rad, uint32_t area_duration_ms,
                const std::string& persistent_vfx_tag,
                RiftForged::Networking::Shared::DamageInstance periodic_damage_instance = RiftForged::Networking::Shared::DamageInstance(0, RiftForged::Networking::Shared::DamageType::DamageType_MIN, false),
                RiftForged::Networking::Shared::StatusEffectCategory periodic_effect_to_apply = RiftForged::Networking::Shared::StatusEffectCategory::StatusEffectCategory_None)
                : effect_payload_type(RiftForged::Networking::UDP::S2C::RiftStepEffectPayload::RiftStepEffectPayload_PersistentArea),
                center_position(center), // Now glm::vec3 to glm::vec3 assignment
                radius(rad),
                duration_ms(area_duration_ms),
                damage(periodic_damage_instance),
                buff_debuff_to_apply(periodic_effect_to_apply),
                visual_effect_tag(persistent_vfx_tag),
                stun(RiftForged::Networking::Shared::StunSeverity::StunSeverity_MIN, 0) {
            }
        };

        // Defines the different types/styles of RiftStep abilities
        enum class ERiftStepType : uint8_t {
            None = 0,
            Basic,
            SolarExplosionExit,
            SolarFlareBlindEntrance,
            GlacialFrozenAttackerEntrance,
            GlacialChilledGroundExit,
            RootingVinesEntrance,
            NatureShieldExit,
            Rapid연속이동, // Korean: Rapid Consecutive Movement
            StealthEntrance,
            GravityWarpEntrance,
            TimeDilationExit,
        };

        // Definition/Template for a RiftStep ability's static properties
        // This struct does not contain Vec3/Quaternion members itself that need changing for math.
        // Its nested 'props' structs use FlatBuffers types for DamageInstance, StunInstance etc.
        struct RiftStepDefinition {
            ERiftStepType type = ERiftStepType::Basic;
            std::string name_tag;

            float max_travel_distance = 15.0f;
            float base_cooldown_sec = 1.25f;

            struct SolarExplosionParams {
                RiftForged::Networking::Shared::DamageInstance damage_on_exit{ 0, RiftForged::Networking::Shared::DamageType::DamageType_Radiant, false };
                float explosion_radius = 5.0f;
            } solar_explosion_props;

            struct SolarBlindParams {
                RiftForged::Networking::Shared::StatusEffectCategory blind_effect = RiftForged::Networking::Shared::StatusEffectCategory::StatusEffectCategory_Debuff_AwarenessReduced;
                uint32_t blind_duration_ms = 2000;
                float blind_radius = 5.0f;
            } solar_blind_props;

            struct GlacialFreezeParams {
                RiftForged::Networking::Shared::StunInstance freeze_stun_on_entrance{ RiftForged::Networking::Shared::StunSeverity::StunSeverity_Medium, 1500 };
                float freeze_radius = 3.0f;
            } glacial_freeze_props;

            struct GlacialChilledGroundParams {
                float chilled_ground_radius = 4.0f;
                uint32_t chilled_ground_duration_ms = 5000;
                RiftForged::Networking::Shared::StatusEffectCategory slow_effect = RiftForged::Networking::Shared::StatusEffectCategory::StatusEffectCategory_Slow_Movement;
                std::string chilled_ground_vfx_tag = "vfx_glacial_chill_ground";
            } glacial_chill_props;

            struct RootingVinesParams {
                RiftForged::Networking::Shared::StatusEffectCategory root_effect = RiftForged::Networking::Shared::StatusEffectCategory::StatusEffectCategory_Root_Generic;
                uint32_t root_duration_ms = 2500;
                float root_radius = 3.0f;
            } rooting_vines_props;

            struct NaturePactEffectParams {
                bool apply_shield_on_exit = true;
                float shield_percent_of_max_health = 0.05f;
                uint32_t shield_duration_ms = 5000;
                bool apply_minor_healing_aura = false;
                float healing_aura_amount_per_tick = 5.0f;
                uint32_t healing_aura_duration_ms = 3000;
                uint32_t healing_aura_tick_interval_ms = 1000;
                float healing_aura_radius = 3.0f;
            } nature_pact_props;

            struct RapidConsecutiveParams {
                int max_additional_steps = 1;
                float subsequent_step_cooldown_sec = 0.25f;
                float subsequent_step_distance_multiplier = 0.75f;
                uint32_t activation_window_ms = 1000;
            } rapid_consecutive_props;

            struct StealthParams {
                uint32_t stealth_duration_ms = 3000;
                RiftForged::Networking::Shared::StatusEffectCategory stealth_buff_category = RiftForged::Networking::Shared::StatusEffectCategory::StatusEffectCategory_Buff_Stealth;
            } stealth_props;

            std::string default_start_vfx_id;
            std::string default_travel_vfx_id;
            std::string default_end_vfx_id;

            RiftStepDefinition() : // Ensure all members are initialized
                type(ERiftStepType::None), name_tag("Uninitialized RiftStep"),
                max_travel_distance(0.0f), base_cooldown_sec(999.0f),
                solar_explosion_props{}, solar_blind_props{}, glacial_freeze_props{},
                glacial_chill_props{}, rooting_vines_props{}, nature_pact_props{},
                rapid_consecutive_props{}, stealth_props{},
                default_start_vfx_id(""), default_travel_vfx_id(""), default_end_vfx_id("") {
            }

            static RiftStepDefinition CreateBasicRiftStep() {
                RiftStepDefinition def;
                def.type = ERiftStepType::Basic;
                def.name_tag = "Basic RiftStep";
                def.max_travel_distance = 15.0f;
                def.base_cooldown_sec = 1.25f;
                def.default_start_vfx_id = "vfx_riftstep_basic_start";
                def.default_travel_vfx_id = "vfx_riftstep_basic_travel";
                def.default_end_vfx_id = "vfx_riftstep_basic_end";
                return def;
            }
        };

        // Defines the outcome of a RiftStep attempt
        struct RiftStepOutcome {
            bool success = false;
            std::string failure_reason_code;
            ERiftStepType type_executed = ERiftStepType::None;
            uint64_t instigator_entity_id = 0;

            // MEMBER TYPES CHANGED
            RiftForged::Utilities::Math::Vec3 actual_start_position{ 0.0f, 0.0f, 0.0f };
            RiftForged::Utilities::Math::Vec3 intended_target_position{ 0.0f, 0.0f, 0.0f };
            RiftForged::Utilities::Math::Vec3 calculated_target_position{ 0.0f, 0.0f, 0.0f };
            RiftForged::Utilities::Math::Vec3 actual_final_position{ 0.0f, 0.0f, 0.0f };

            float travel_duration_sec = 0.05f;

            // GameplayEffectInstance now uses GLM-based Vec3 for center_position
            std::vector<GameplayEffectInstance> entry_effects_data;
            std::vector<GameplayEffectInstance> exit_effects_data;

            std::string start_vfx_id;
            std::string travel_vfx_id;
            std::string end_vfx_id;

            // Default constructor to ensure GLM vectors are initialized if not using C++17 member init for them
            RiftStepOutcome() : success(false), type_executed(ERiftStepType::None), instigator_entity_id(0),
                actual_start_position(0.0f), intended_target_position(0.0f),
                calculated_target_position(0.0f), actual_final_position(0.0f),
                travel_duration_sec(0.05f) {
            }
        };

    } // namespace GameLogic
} // namespace RiftForged