// File: Gameplay/ActivePlayer.cpp
// RiftForged Game Development Team
// Copyright (c) 2023-2025 RiftForged Game Development Team

#include <RiftForged/GameEngine/ActivePlayer/ActivePlayer.h> // For ActivePlayer class
#include <RiftForged/Utilities/MathUtils/MathUtils.h> // For GLM-based Vec3, Quaternion
#include <RiftForged/Utilities/Logger/Logger.h> // For logging macros

// RiftStepLogic.h is included via ActivePlayer.h (ensure RiftStepOutcome uses new math types)
// FlatBuffer headers for network enums (DamageType, AnimationState) are included via ActivePlayer.h

namespace RiftForged {
    namespace GameLogic {

        // --- Constructor ---
        // Parameters startPos and startOrientation are now expected to be GLM-based types
        ActivePlayer::ActivePlayer(uint64_t pId,
            const Utilities::Math::Vec3& startPos, // Changed from Networking::Shared::Vec3
            const Utilities::Math::Quaternion& startOrientation, // Changed from Networking::Shared::Quaternion
            float cap_radius, float cap_half_height)
            : playerId(pId),
            position(startPos), // Directly assign, as it's now glm::vec3
            orientation(RiftForged::Utilities::Math::NormalizeQuaternion(startOrientation)), // Normalize the incoming glm::quat
            capsule_radius(cap_radius),
            capsule_half_height(cap_half_height),
            currentHealth(250), maxHealth(250),
            currentWill(100), maxWill(100),
            base_ability_cooldown_modifier(1.0f),
            base_critical_hit_chance_percent(5.0f),
            base_critical_hit_damage_multiplier(2.0f),
            base_accuracy_rating_percent(75.0f),
            base_basic_attack_cooldown_sec(1.0f),
            flat_physical_damage_reduction(10), percent_physical_damage_reduction(0.0f),
            flat_radiant_damage_reduction(0), percent_radiant_damage_reduction(0.0f),
            flat_frost_damage_reduction(0), percent_frost_damage_reduction(0.0f),
            flat_shock_damage_reduction(0), percent_shock_damage_reduction(0.0f),
            flat_necrotic_damage_reduction(0), percent_necrotic_damage_reduction(0.0f),
            flat_void_damage_reduction(0), percent_void_damage_reduction(-0.15f),
            flat_cosmic_damage_reduction(0), percent_cosmic_damage_reduction(0.0f),
            flat_poison_damage_reduction(0), percent_poison_damage_reduction(0.0f),
            flat_nature_damage_reduction(0), percent_nature_damage_reduction(0.0f),
            flat_aetherial_damage_reduction(0), percent_aetherial_damage_reduction(-0.50f),
            current_rift_step_definition(RiftStepDefinition::CreateBasicRiftStep()),
            current_weapon_category(EquippedWeaponCategory::Unarmed),
            equipped_weapon_definition_id(0),
            movementState(PlayerMovementState::Idle),
            animationStateId(static_cast<uint32_t>(RiftForged::Networking::Shared::AnimationState::AnimationState_Idle)),
            isDirty(true),
            last_processed_movement_intent(0.f, 0.f, 0.f), // Initialize glm::vec3
            was_sprint_intended(false) {
            RF_GAMELOGIC_DEBUG("ActivePlayer {} constructed. Initial RiftStep: '{}'. Pos:({:.1f},{:.1f},{:.1f})",
                playerId, current_rift_step_definition.name_tag,
                position.x, position.y, position.z); // Changed to .x, .y, .z
        }

        void ActivePlayer::MarkDirty() {
            isDirty.store(true, std::memory_order_release);
        }

        // --- State Modification Methods ---

        // newPosition is now expected to be a GLM-based Utilities::Math::Vec3
        void ActivePlayer::SetPosition(const Utilities::Math::Vec3& newPosition) {
            const float POSITION_EPSILON_SQUARED = 0.0001f * 0.0001f;
            // Utilities::Math::DistanceSquared now takes two GLM Vec3
            if (RiftForged::Utilities::Math::DistanceSquared(position, newPosition) > POSITION_EPSILON_SQUARED) {
                position = newPosition; // position is now glm::vec3
                MarkDirty();
            }
        }

        // newOrientation is now expected to be a GLM-based Utilities::Math::Quaternion
        void ActivePlayer::SetOrientation(const Utilities::Math::Quaternion& newOrientation) {
            // Utilities::Math::NormalizeQuaternion now takes a GLM Quaternion
            Utilities::Math::Quaternion normalizedNewOrientation = RiftForged::Utilities::Math::NormalizeQuaternion(newOrientation);
            // Utilities::Math::AreQuaternionsClose now takes two GLM Quaternions
            if (!RiftForged::Utilities::Math::AreQuaternionsClose(orientation, normalizedNewOrientation, 0.99999f)) {
                orientation = normalizedNewOrientation; // orientation is now glm::quat
                MarkDirty();
            }
        }

        void ActivePlayer::SetWill(int32_t value) {
            int32_t newWill = std::max(0, std::min(value, static_cast<int32_t>(maxWill)));
            if (currentWill != newWill) {
                currentWill = newWill;
                MarkDirty();
            }
        }

        void ActivePlayer::DeductWill(int32_t amount) {
            if (amount <= 0) return;
            SetWill(currentWill - amount);
        }

        void ActivePlayer::AddWill(int32_t amount) {
            if (amount <= 0) return;
            SetWill(currentWill + amount);
        }

        void ActivePlayer::SetHealth(int32_t value) {
            int32_t newHealth = std::max(0, std::min(value, static_cast<int32_t>(maxHealth)));
            if (currentHealth != newHealth) {
                currentHealth = newHealth;
                MarkDirty();
                if (currentHealth == 0 && movementState != PlayerMovementState::Dead) {
                    SetMovementState(PlayerMovementState::Dead);
                    RF_GAMEPLAY_INFO("Player {} health reached 0. Marked as Dead.", playerId);
                }
            }
        }

        void ActivePlayer::HealDamage(int32_t amount) {
            if (amount <= 0 || movementState == PlayerMovementState::Dead) return;
            SetHealth(currentHealth + amount);
        }

        // DamageType is still a FlatBuffers enum, which is fine
        int32_t ActivePlayer::TakeDamage(int32_t raw_damage_amount, RiftForged::Networking::Shared::DamageType damage_type) {
            if (raw_damage_amount <= 0 || movementState == PlayerMovementState::Dead) return 0;

            float percentage_reduction = 0.0f;
            int32_t flat_reduction = 0;

            // Damage calculation logic remains the same
            switch (damage_type) {
            case RiftForged::Networking::Shared::DamageType::DamageType_Physical:  percentage_reduction = percent_physical_damage_reduction; flat_reduction = flat_physical_damage_reduction; break;
            case RiftForged::Networking::Shared::DamageType::DamageType_Radiant:   percentage_reduction = percent_radiant_damage_reduction;  flat_reduction = flat_radiant_damage_reduction;  break;
            case RiftForged::Networking::Shared::DamageType::DamageType_Frost:     percentage_reduction = percent_frost_damage_reduction;    flat_reduction = flat_frost_damage_reduction;    break;
            case RiftForged::Networking::Shared::DamageType::DamageType_Lightning: percentage_reduction = percent_shock_damage_reduction;    flat_reduction = flat_shock_damage_reduction;    break;
            case RiftForged::Networking::Shared::DamageType::DamageType_Necrotic:  percentage_reduction = percent_necrotic_damage_reduction; flat_reduction = flat_necrotic_damage_reduction; break;
            case RiftForged::Networking::Shared::DamageType::DamageType_Void:      percentage_reduction = percent_void_damage_reduction;     flat_reduction = flat_void_damage_reduction;     break;
            case RiftForged::Networking::Shared::DamageType::DamageType_Cosmic:    percentage_reduction = percent_cosmic_damage_reduction;   flat_reduction = flat_cosmic_damage_reduction;   break;
            case RiftForged::Networking::Shared::DamageType::DamageType_Poison:    percentage_reduction = percent_poison_damage_reduction;   flat_reduction = flat_poison_damage_reduction;   break;
            case RiftForged::Networking::Shared::DamageType::DamageType_Nature:    percentage_reduction = percent_nature_damage_reduction;   flat_reduction = flat_nature_damage_reduction;   break;
            case RiftForged::Networking::Shared::DamageType::DamageType_Aetherial: percentage_reduction = percent_aetherial_damage_reduction;flat_reduction = flat_aetherial_damage_reduction;break;
            case RiftForged::Networking::Shared::DamageType::DamageType_None:
            default: RF_GAMEPLAY_WARN("Player {} TakeDamage: Unhandled or 'None' damage type ({}) received. No reductions applied.", playerId, static_cast<int>(damage_type)); break;
            }

            int32_t damage_after_flat_reduction = std::max(0, raw_damage_amount - flat_reduction);
            // Ensure percentage reduction is clamped between 0 and 1 for calculation
            float effective_percent_reduction = std::max(0.0f, std::min(1.0f, percentage_reduction / 100.0f));
            int32_t final_damage = static_cast<int32_t>(static_cast<float>(damage_after_flat_reduction) * (1.0f - effective_percent_reduction));
            final_damage = std::max(0, final_damage);


            RF_GAMEPLAY_INFO("Player {} taking {} raw damage of type {}. FlatRed: {}, PctRedVal: {:.2f}. Final: {}.",
                playerId, raw_damage_amount, static_cast<int>(damage_type),
                flat_reduction, percentage_reduction, final_damage);

            int32_t health_before_damage = currentHealth;
            SetHealth(currentHealth - final_damage);
            return health_before_damage - currentHealth; // Actual damage dealt
        }

        // AnimationState is still a FlatBuffers enum
        void ActivePlayer::SetAnimationState(RiftForged::Networking::Shared::AnimationState newState) {
            SetAnimationStateId(static_cast<uint32_t>(newState));
        }

        void ActivePlayer::SetAnimationStateId(uint32_t newStateId) {
            if (animationStateId != newStateId) {
                animationStateId = newStateId;
                MarkDirty();
            }
        }

        void ActivePlayer::SetMovementState(PlayerMovementState newState) {
            if (movementState != newState) {
                PlayerMovementState oldState = movementState;
                movementState = newState;
                MarkDirty();
                RF_GAMELOGIC_TRACE("Player {} movement state changed from {} to {}", playerId, static_cast<int>(oldState), static_cast<int>(newState));

                // Animation state changes based on movement state
                switch (newState) {
                case PlayerMovementState::Idle:      SetAnimationStateId(static_cast<uint32_t>(RiftForged::Networking::Shared::AnimationState::AnimationState_Idle)); break;
                case PlayerMovementState::Walking:   SetAnimationStateId(static_cast<uint32_t>(RiftForged::Networking::Shared::AnimationState::AnimationState_Walking)); break;
                case PlayerMovementState::Sprinting: SetAnimationStateId(static_cast<uint32_t>(RiftForged::Networking::Shared::AnimationState::AnimationState_Running)); break; // Assuming Running animation for sprint
                case PlayerMovementState::Dead:      SetAnimationStateId(static_cast<uint32_t>(RiftForged::Networking::Shared::AnimationState::AnimationState_Dead)); break;
                case PlayerMovementState::Stunned:   SetAnimationStateId(static_cast<uint32_t>(RiftForged::Networking::Shared::AnimationState::AnimationState_Stunned)); break;
                    // Rifting, Ability_In_Use, Rooted might have specific animations or keep the previous one
                case PlayerMovementState::Rifting:          /* May need specific animation or handled by ability */ break;
                case PlayerMovementState::Ability_In_Use:   /* Animation handled by ability system */ break;
                case PlayerMovementState::Rooted:           /* May overlay on current animation or have specific one */ break;
                default: RF_GAMELOGIC_WARN("Player {}: SetMovementState called with unhandled new state {}", playerId, static_cast<int>(newState)); break;
                }
            }
        }

        // --- Ability Cooldown Management ---
        bool ActivePlayer::IsAbilityOnCooldown(uint32_t abilityId) const {
            std::lock_guard<std::mutex> lock(m_internalDataMutex);
            auto it = abilityCooldowns.find(abilityId);
            if (it != abilityCooldowns.end()) {
                return std::chrono::steady_clock::now() < it->second;
            }
            return false;
        }

        void ActivePlayer::StartAbilityCooldown(uint32_t abilityId, float base_duration_sec) {
            std::lock_guard<std::mutex> lock(m_internalDataMutex);
            if (base_duration_sec <= 0.0f) {
                abilityCooldowns.erase(abilityId); // Remove cooldown if duration is zero or negative
                RF_GAMELOGIC_TRACE("Player {} cooldown for ability {} cleared (duration <= 0).", playerId, abilityId);
            }
            else {
                float modified_duration_sec = base_duration_sec * base_ability_cooldown_modifier;
                modified_duration_sec = std::max(0.05f, modified_duration_sec); // Minimum cooldown
                abilityCooldowns[abilityId] = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(static_cast<long long>(modified_duration_sec * 1000.0f));
                RF_GAMELOGIC_TRACE("Player {} cooldown for ability {} set to {:.2f}s (modified from {:.2f}s base).", playerId, abilityId, modified_duration_sec, base_duration_sec);
            }
        }

        // --- Action Specific Logic Helpers ---
        void ActivePlayer::UpdateActiveRiftStepDefinition(const RiftStepDefinition& new_definition) {
            current_rift_step_definition = new_definition;
            RF_GAMELOGIC_INFO("Player {} active RiftStep updated to: {}", playerId, current_rift_step_definition.name_tag);
        }

        bool ActivePlayer::CanPerformRiftStep() const {
            if (movementState == PlayerMovementState::Stunned ||
                movementState == PlayerMovementState::Rooted ||
                movementState == PlayerMovementState::Dead ||
                movementState == PlayerMovementState::Ability_In_Use) { // Assuming general ability usage blocks riftstep
                RF_PLAYERMGR_TRACE("Player {} cannot RiftStep due to movement state: {}", playerId, static_cast<int>(movementState));
                return false;
            }
            if (IsAbilityOnCooldown(RIFTSTEP_ABILITY_ID)) { // RIFTSTEP_ABILITY_ID should be a defined constant
                RF_PLAYERMGR_TRACE("Player {} cannot RiftStep: ability {} on cooldown.", playerId, RIFTSTEP_ABILITY_ID);
                return false;
            }
            // Check Will cost if applicable
            // if (currentWill < current_rift_step_definition.will_cost) {
            //     RF_PLAYERMGR_TRACE("Player {} cannot RiftStep: insufficient Will ({} < {}).", playerId, currentWill, current_rift_step_definition.will_cost);
            //     return false;
            // }
            return true;
        }

        // IMPORTANT: RiftStepOutcome struct members (like actual_start_position, intended_target_position)
        // and GameplayEffectInstance members (like center_position) must also be updated to use GLM-based Vec3/Quaternion
        // in their respective definitions (likely in RiftStepLogic.h or ActivePlayer.h).
        RiftStepOutcome ActivePlayer::PrepareRiftStepOutcome(RiftForged::Networking::UDP::C2S::RiftStepDirectionalIntent directional_intent, ERiftStepType type) {
            RiftStepOutcome outcome; // Assumes RiftStepOutcome members are now GLM-based
            outcome.type_executed = current_rift_step_definition.type;
            outcome.actual_start_position = this->position; // position is now glm::vec3

            outcome.travel_duration_sec = 0.05f;

            Utilities::Math::Vec3 target_direction_vector; // Now glm::vec3
            Utilities::Math::Quaternion currentOrientationQuat = this->orientation; // orientation is now glm::quat

            // Utilities::Math functions now operate on and return GLM types
            Utilities::Math::Vec3 world_forward = Utilities::Math::GetWorldForwardVector(currentOrientationQuat);
            Utilities::Math::Vec3 world_right = Utilities::Math::GetWorldRightVector(currentOrientationQuat);

            switch (directional_intent) {
            case Networking::UDP::C2S::RiftStepDirectionalIntent::RiftStepDirectionalIntent_Intentional_Forward:  target_direction_vector = world_forward; break;
            case Networking::UDP::C2S::RiftStepDirectionalIntent::RiftStepDirectionalIntent_Intentional_Backward:
            case Networking::UDP::C2S::RiftStepDirectionalIntent::RiftStepDirectionalIntent_Default_Backward:      target_direction_vector = Utilities::Math::ScaleVector(world_forward, -1.0f); break;
            case Networking::UDP::C2S::RiftStepDirectionalIntent::RiftStepDirectionalIntent_Intentional_Left:       target_direction_vector = Utilities::Math::ScaleVector(world_right, -1.0f); break;
            case Networking::UDP::C2S::RiftStepDirectionalIntent::RiftStepDirectionalIntent_Intentional_Right:      target_direction_vector = world_right; break;
            default:
                RF_GAMELOGIC_WARN("Player {} used RiftStep with unknown directional_intent: {}. Defaulting to backward.", playerId, static_cast<int>(directional_intent));
                target_direction_vector = Utilities::Math::ScaleVector(world_forward, -1.0f);
                break;
            }
            target_direction_vector = Utilities::Math::NormalizeVector(target_direction_vector);

            float travel_distance = current_rift_step_definition.max_travel_distance;
            Utilities::Math::Vec3 scaled_direction = Utilities::Math::ScaleVector(target_direction_vector, travel_distance);
            outcome.intended_target_position = Utilities::Math::AddVectors(this->position, scaled_direction);
            outcome.calculated_target_position = outcome.intended_target_position;

            outcome.start_vfx_id = current_rift_step_definition.default_start_vfx_id;
            outcome.travel_vfx_id = current_rift_step_definition.default_travel_vfx_id;
            outcome.end_vfx_id = current_rift_step_definition.default_end_vfx_id;

            // Effect generation logic (assuming GameplayEffectInstance constructors/members are GLM-aware)
            switch (outcome.type_executed) {
            case ERiftStepType::Basic:
                RF_GAMEPLAY_DEBUG("Player {}: Basic RiftStep prepared.", playerId);
                break;
            case ERiftStepType::SolarExplosionExit: {
                const auto& params = current_rift_step_definition.solar_explosion_props;
                outcome.exit_effects_data.emplace_back(
                    outcome.intended_target_position, // center (glm::vec3)
                    params.explosion_radius,
                    params.damage_on_exit // RiftForged::Networking::Shared::DamageInstance (FlatBuffers type)
                );
                outcome.exit_effects_data.back().visual_effect_tag = "vfx_solar_explosion_exit";
                RF_GAMEPLAY_DEBUG("Player {}: SolarExplosionExit RiftStep prepared.", playerId);
                break;
            }
                                                  // ... (Similar updates for other ERiftStepType cases if they use positions/orientations) ...
                                                  // Example for GlacialChilledGroundExit needing a DamageInstance
            case ERiftStepType::GlacialChilledGroundExit: {
                const auto& params = current_rift_step_definition.glacial_chill_props;
                // Create a "none" damage instance if the constructor requires it.
                RiftForged::Networking::Shared::DamageInstance no_damage(0, RiftForged::Networking::Shared::DamageType::DamageType_None, false);
                outcome.exit_effects_data.emplace_back(
                    outcome.intended_target_position,    // center (glm::vec3)
                    params.chilled_ground_radius,
                    params.chilled_ground_duration_ms,
                    params.chilled_ground_vfx_tag,
                    no_damage,                           // periodic_damage_instance
                    params.slow_effect                   // periodic_effect_to_apply
                );
                RF_GAMEPLAY_DEBUG("Player {}: GlacialChilledGroundExit RiftStep prepared.", playerId);
                break;
            }
            case ERiftStepType::NatureShieldExit: {
                const auto& params = current_rift_step_definition.nature_pact_props;
                if (params.apply_shield_on_exit) {
                    outcome.exit_effects_data.emplace_back(
                        outcome.intended_target_position, // center (glm::vec3)
                        0.5f,
                        params.shield_duration_ms,
                        Networking::Shared::StatusEffectCategory::StatusEffectCategory_Buff_DamageAbsorption_Shield,
                        "vfx_nature_shield_exit"
                    );
                }
                if (params.apply_minor_healing_aura) {
                    RiftForged::Networking::Shared::DamageInstance no_damage(0, RiftForged::Networking::Shared::DamageType::DamageType_None, false);
                    outcome.exit_effects_data.emplace_back(
                        outcome.intended_target_position, // center (glm::vec3)
                        params.healing_aura_radius,
                        params.healing_aura_duration_ms,
                        "vfx_nature_healing_aura",
                        no_damage,
                        Networking::Shared::StatusEffectCategory::StatusEffectCategory_Buff_HealOverTime_Generic
                    );
                }
                RF_GAMEPLAY_DEBUG("Player {}: NatureShieldExit RiftStep prepared.", playerId);
                break;
            }
                                                // Ensure all other RiftStep types that emplace GameplayEffectInstances
                                                // correctly provide glm::vec3 for positions if the constructor expects it.
            default:
                RF_GAMEPLAY_WARN("Player {}: PrepareRiftStepOutcome - Unhandled ERiftStepType ({}) for specific effect generation.",
                    playerId, static_cast<int>(outcome.type_executed));
                break;
            }


            StartAbilityCooldown(RIFTSTEP_ABILITY_ID, current_rift_step_definition.base_cooldown_sec);

            outcome.success = true;
            RF_GAMELOGIC_DEBUG("Player {} prepared RiftStep. Type: {}. Target: ({:.1f},{:.1f},{:.1f}). Effects: Entry({}), Exit({})",
                playerId, static_cast<int>(outcome.type_executed),
                outcome.intended_target_position.x, outcome.intended_target_position.y, outcome.intended_target_position.z, // Changed to .x, .y, .z
                outcome.entry_effects_data.size(), outcome.exit_effects_data.size());
            return outcome;
        }

        // --- Status Effect Management ---
        // (Status effect logic remains the same as it deals with enums)
        void ActivePlayer::AddStatusEffects(const std::vector<RiftForged::Networking::Shared::StatusEffectCategory>& effects_to_add) {
            bool changed = false;
            std::lock_guard<std::mutex> lock(m_internalDataMutex);
            for (const auto& effect : effects_to_add) {
                if (effect == RiftForged::Networking::Shared::StatusEffectCategory::StatusEffectCategory_None) continue;
                if (std::find(activeStatusEffects.begin(), activeStatusEffects.end(), effect) == activeStatusEffects.end()) {
                    activeStatusEffects.push_back(effect);
                    changed = true;
                    RF_GAMEPLAY_DEBUG("Player {}: Added status effect {}", playerId, static_cast<uint32_t>(effect));
                }
            }
            if (changed) MarkDirty();
        }

        void ActivePlayer::RemoveStatusEffects(const std::vector<RiftForged::Networking::Shared::StatusEffectCategory>& effects_to_remove) {
            bool changed = false;
            std::lock_guard<std::mutex> lock(m_internalDataMutex);
            for (const auto& effect_to_remove_item : effects_to_remove) {
                if (effect_to_remove_item == RiftForged::Networking::Shared::StatusEffectCategory::StatusEffectCategory_None) continue;
                auto it = std::remove(activeStatusEffects.begin(), activeStatusEffects.end(), effect_to_remove_item);
                if (it != activeStatusEffects.end()) {
                    activeStatusEffects.erase(it, activeStatusEffects.end());
                    changed = true;
                    RF_GAMEPLAY_DEBUG("Player {}: Removed status effect {}", playerId, static_cast<uint32_t>(effect_to_remove_item));
                }
            }
            if (changed) MarkDirty();
        }

        bool ActivePlayer::HasStatusEffect(Networking::Shared::StatusEffectCategory effect) const {
            std::lock_guard<std::mutex> lock(m_internalDataMutex);
            return std::find(activeStatusEffects.begin(), activeStatusEffects.end(), effect) != activeStatusEffects.end();
        }

        // --- Equipment ---
        void ActivePlayer::SetEquippedWeapon(uint32_t weapon_def_id, EquippedWeaponCategory category) {
            bool changed = false;
            if (equipped_weapon_definition_id != weapon_def_id) {
                equipped_weapon_definition_id = weapon_def_id;
                changed = true;
            }
            if (current_weapon_category != category) {
                current_weapon_category = category;
                changed = true;
            }
            if (changed) {
                MarkDirty();
                RF_GAMELOGIC_INFO("Player {} equipped weapon ID: {}, Category: {}", playerId, weapon_def_id, static_cast<int>(category));
            }
        }

        // --- Helpers ---
        // Returns a GLM-based Utilities::Math::Vec3
        Utilities::Math::Vec3 ActivePlayer::GetMuzzlePosition() const {
            Utilities::Math::Vec3 local_muzzle_offset(0.0f, 1.0f, 0.5f); // Example offset, adjust as needed
            // orientation is now glm::quat
            Utilities::Math::Quaternion currentOrientationQuat = this->orientation;
            // Utilities::Math::RotateVectorByQuaternion takes and returns GLM types
            Utilities::Math::Vec3 world_offset = Utilities::Math::RotateVectorByQuaternion(local_muzzle_offset, currentOrientationQuat);
            // this->position is glm::vec3, AddVectors takes and returns GLM types
            return Utilities::Math::AddVectors(this->position, world_offset);
        }

    } // namespace GameLogic
} // namespace RiftForged