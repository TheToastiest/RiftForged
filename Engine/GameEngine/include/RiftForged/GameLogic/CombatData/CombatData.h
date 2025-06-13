#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <RiftForged/Utilities/MathUtils/MathUtils.h> // For Vec3, Quaternion

// --- FlatBuffer includes are no longer needed ---
// #include "../FlatBuffers/Versioning/V0.0.5/riftforged_common_types_generated.h"
// #include "../FlatBuffers/Versioning/V0.0.5/riftforged_s2c_udp_messages_generated.h"

namespace RiftForged {
    namespace GameLogic {
        namespace Combat { // Nesting within a Combat namespace for clarity

            //==--------------------------------------------------------------------==//
            // 1. NEW: Pure GameLogic Enums (Decoupled from FlatBuffers)
            //==--------------------------------------------------------------------==//

            enum class DamageType : uint8_t {
                None,
                Physical,
                Fire,
                Ice,
                Lightning
                // ... etc
            };

            enum class CombatEventType : uint8_t {
                None,
                DamageDealt,
                HealReceived,
                Miss,
                Dodge
                // ... etc
            };

            //==--------------------------------------------------------------------==//
            // 2. NEW: Pure GameLogic Damage Struct (Decoupled from FlatBuffers)
            //==--------------------------------------------------------------------==//

            struct DamageInstance {
                float       amount = 0.0f;
                DamageType  type = DamageType::None;
                bool        wasCritical = false;
            };


            //==--------------------------------------------------------------------==//
            // 3. REFACTORED: Structs now use the pure GameLogic types
            //==--------------------------------------------------------------------==//

            struct DamageApplicationDetails {
                uint64_t                target_id = 0;
                uint64_t                source_id = 0;
                int32_t                 final_damage_dealt = 0;
                DamageType              damage_type = DamageType::None; // NOW uses our GameLogic enum
                bool                    was_crit = false;
                bool                    was_kill = false;
                Utilities::Math::Vec3   impact_point{ 0.0f, 0.0f, 0.0f };
            };

            struct AttackOutcome {
                bool                    success = true;
                std::string             failure_reason_code;
                bool                    is_basic_attack = false;
                CombatEventType         simulated_combat_event_type = CombatEventType::None; // NOW uses our GameLogic enum
                std::string             attack_animation_tag_for_caster;
                std::vector<DamageApplicationDetails> damage_events;

                // Projectile data
                bool                    spawned_projectile = false;
                uint64_t                projectile_id = 0;
                uint64_t                projectile_owner_id = 0;
                Utilities::Math::Vec3   projectile_start_position;
                Utilities::Math::Vec3   projectile_direction;
                float                   projectile_speed = 0.0f;
                float                   projectile_max_range = 0.0f;
                std::string             projectile_vfx_tag;
                DamageInstance          projectile_damage_on_hit; // NOW uses our GameLogic struct
            };

        } // namespace Combat
    } // namespace GameLogic
} // namespace RiftForged