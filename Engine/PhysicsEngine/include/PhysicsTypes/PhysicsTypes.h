// File: PhysicsEngine/PhysicsTypes.h
// RiftForged Game Development Team
// Copyright (c) 2025-2028 RiftForged Game Development Team
#pragma once

// PhysX Headers
#include "foundation/PxFlags.h"     // For PxFlags for ECollisionGroup
#include "PxPhysicsAPI.h"         // General PhysX include (provides PxU32, PxVec3, PxQuat, PxActor, PxShape etc.)
// Specific includes if PxPhysicsAPI.h is too broad, but usually it's fine.
// #include "physx/PxPhysXConfig.h"     // For PxU32 and other PhysX types
// #include "physx/PxFiltering.h"       // For PxFilterData related types if not in PxPhysicsAPI
// #include "physx/PxQueryFiltering.h"  // For PxQueryFilterData if not in PxPhysicsAPI

// Project-Specific Headers
#include <RiftForged/Utilities/MathUtils/MathUtils.h>     // For RiftForged::Utilities::Math::Vec3/Quaternion (GLM aliases)
//#include "../FlatBuffers/Versioning/V0.0.5/riftforged_common_types_generated.h" // For RiftForged::Networking::Shared::DamageInstance

namespace RiftForged {
    namespace Physics {

        // --- GLM Aliases and Conversion Helpers ---
        // Use GLM-based types from Utilities::Math for physics calculations
        using SharedVec3 = RiftForged::Utilities::Math::Vec3;       // glm::vec3
        using SharedQuaternion = RiftForged::Utilities::Math::Quaternion; // glm::quat

        // Convert GLM Vec3 (SharedVec3) to PhysX PxVec3
        inline physx::PxVec3 ToPxVec3(const SharedVec3& v) {
            return physx::PxVec3(v.x, v.y, v.z);
        }

        // Convert PhysX PxVec3 to GLM Vec3 (SharedVec3)
        inline SharedVec3 FromPxVec3(const physx::PxVec3& pv) {
            return SharedVec3(pv.x, pv.y, pv.z);
        }

        // Convert GLM Quaternion (SharedQuaternion) to PhysX PxQuat
        inline physx::PxQuat ToPxQuat(const SharedQuaternion& q) {
            return physx::PxQuat(q.x, q.y, q.z, q.w); // PxQuat is x,y,z,w
        }

        // Convert PhysX PxQuat to GLM Quaternion (SharedQuaternion)
        inline SharedQuaternion FromPxQuat(const physx::PxQuat& pq) {
            return SharedQuaternion(pq.w, pq.x, pq.y, pq.z); // glm::quat is w,x,y,z
        }
        // --- End GLM Aliases and Conversion Helpers ---


        // --- Enums for Physics Identification and Filtering ---
        enum class EPhysicsObjectType : physx::PxU32 {
            UNDEFINED = 0,
            PLAYER_CHARACTER = 1,
            SMALL_ENEMY = 2,
            MEDIUM_ENEMY = 3,
            LARGE_ENEMY = 4,
            HUGE_ENEMY = 5,
            // Gap for future creature types
            RAID_BOSS = 7,
            // Gap
            VAELITH = 10,
            COMET = 11,
            MAGIC_PROJECTILE = 12,
            LIGHTNING_BOLT = 13,
            // Gap for other dynamic/special effects
            WALL = 20,
            IMPASSABLE_ROCK = 21,
            // Gap for other major static impassables
            LARGE_ROCK = 30,
            SMALL_ROCK = 31,
            // Gap for other minor/dynamic obstacles
            MELEE_WEAPON = 40,
            PROJECTILE = 50, // Generic/physical projectile
            INTERACTABLE_OBJECT = 60,
            // Gap
            STATIC_IMPASSABLE = 100 // General category for non-terrain unmovable world geometry
        };

        enum class ECollisionGroup : physx::PxU32 {
            GROUP_NONE = 0,
            GROUP_PLAYER = (1u << 0),
            GROUP_ENEMY = (1u << 1),
            GROUP_PLAYER_PROJECTILE = (1u << 2),
            GROUP_ENEMY_PROJECTILE = (1u << 3),
            GROUP_WORLD_STATIC = (1u << 4),    // Walls, ground, static impassable geometry
            GROUP_WORLD_DYNAMIC = (1u << 5),   // Pushable rocks, dynamic physics props
            GROUP_MELEE_HITBOX = (1u << 6),
            GROUP_COMET = (1u << 7),
            GROUP_VAELITH = (1u << 8),
            GROUP_RAID_BOSS = (1u << 9),
            GROUP_INTERACTABLE = (1u << 10),
            GROUP_TRIGGER_VOLUME = (1u << 11)  // For generic trigger volumes
            // Up to 32 groups (1u << 31)
        };

        // Helper for bitmask operations on ECollisionGroup
        typedef physx::PxFlags<ECollisionGroup, physx::PxU32> CollisionGroupFlags;

        // Define PxFlags operators for ECollisionGroup to enable bitwise logic like | & ~
        // Ensure these are defined only once (typically with the PxFlags typedef)
        // PX_CUDA_CALLABLE and PX_INLINE are from PhysX headers, good for compatibility
        PX_CUDA_CALLABLE PX_INLINE CollisionGroupFlags operator|(ECollisionGroup a, ECollisionGroup b) {
            return CollisionGroupFlags(a) | b;
        }
        PX_CUDA_CALLABLE PX_INLINE CollisionGroupFlags operator&(ECollisionGroup a, ECollisionGroup b) {
            return CollisionGroupFlags(a) & b;
        }
        PX_CUDA_CALLABLE PX_INLINE CollisionGroupFlags operator~(ECollisionGroup a) {
            return ~CollisionGroupFlags(a);
        }
        // --- End Enums ---


        // --- Common Physics Data Structures ---
        struct HitResult {
            uint64_t hit_entity_id = 0;
            physx::PxActor* hit_actor = nullptr;
            physx::PxShape* hit_shape = nullptr;
            SharedVec3 hit_point;       // This is now glm::vec3
            SharedVec3 hit_normal;      // This is now glm::vec3
            float distance = -1.0f;
            uint32_t hit_face_index = 0xFFFFFFFF; // Indicates an invalid or uncomputed face index

            // Default constructor
            HitResult() : hit_entity_id(0), hit_actor(nullptr), hit_shape(nullptr),
                hit_point(0.0f), hit_normal(0.0f), distance(-1.0f), hit_face_index(0xFFFFFFFF) {
            }
        };

        struct CollisionFilterData { // Used to set up PxFilterData for shapes
            physx::PxU32 word0 = 0; // Typically EPhysicsObjectType or primary ECollisionGroup
            physx::PxU32 word1 = 0; // Typically ECollisionGroup mask (collides with)
            physx::PxU32 word2 = 0; // Additional data/flags
            physx::PxU32 word3 = 0; // Additional data/flags
        };

        struct ProjectilePhysicsProperties {
            float radius = 0.05f;
            float halfHeight = 0.0f; // If > 0, it's a capsule with this half-height. If 0, it's a sphere.
            float mass = 0.2f;
            bool enableGravity = true;
            bool enableCCD = false;
        };

        // --- End Common Physics Data Structures ---

    } // namespace Physics
} // namespace RiftForged