// File: PhysicsEngine/PhysicsEngine.h
// Copyright (c) 2025-202* RiftForged Game Development Team
#pragma once


// PhysX Headers
#include <PxPhysicsAPI.h>       // Main PhysX header
#include <PxPhysics.h>           // For PxPhysics
#include <foundation/PxMath.h>   // For PxPi, etc.
#include <extensions/PxDefaultCpuDispatcher.h> // For PxDefaultCpuDispatcher
#include <characterkinematic/PxController.h>   // For PxController
#include <PxQueryFiltering.h>   // For PxQueryFilterData, PxQueryFilterCallback
#include <cudamanager/PxCudaContextManager.h> // For PxCudaContextManager

// Standard Library Headers
#include <map>
#include <vector>
#include <string>
#include <mutex> // For std::mutex and std::lock_guard

// Project-Specific Headers
// For FlatBuffers types like DamageInstance (used in ProjectileGameData)
//#include "../FlatBuffers/Versioning/V0.0.5/riftforged_common_types_generated.h" 
// For GLM-based math type aliases (RiftForged::Utilities::Math::Vec3, etc.)
#include <RiftForged/Utilities/MathUtils/MathUtils.h>
#include <RiftForged/Utilities/Logger/Logger.h>
// For other physics-related types (e.g., EPhysicsObjectType, potentially CollisionFilterData if moved)
#include <RiftForged/Physics/PhysicsTypes/PhysicsTypes.h>
#include <RiftForged/Core/TerrainData/TerrainData.h> // For TerrainHeightData, ProcessedHeightfieldData

// Forward declarations for PhysX types (good practice)
namespace physx {
    class PxShape;
    class PxGeometry;
    class PxRigidActor;
    class PxRigidBody;
    class PxRigidStatic;
    class PxRigidDynamic;
    class PxMaterial;
    class PxScene;
    // PxPhysics forward declared by PxPhysicsAPI.h
    // PxDefaultCpuDispatcher forward declared by its include
    class PxFoundation;
    class PxPvd;
    class PxPvdTransport;
    class PxControllerManager;
    // PxController forward declared by its include
    class PxTriangleMesh;
    class PxConvexMesh;
    class PxHeightField;
    // PxCudaContextManager forward declared by its include
    // PxQueryFilterData defined in PxQueryFiltering.h
} // namespace physx

namespace RiftForged {
    namespace Physics {

        // --- Use GLM-based types from Utilities::Math ---
        // These aliases ensure that SharedVec3 and SharedQuaternion within the Physics namespace
        // refer to the GLM types (glm::vec3, glm::quat) provided by MathUtil.h.
        using SharedVec3 = RiftForged::Utilities::Math::Vec3;
        using SharedQuaternion = RiftForged::Utilities::Math::Quaternion;
        // inline physx::PxVec3 ToPxVec3(const SharedVec3& v);
   // inline SharedVec3 ToGlmVec3(const physx::PxVec3& v);
   // inline physx::PxQuat ToPxQuat(const SharedQuaternion& q);
   // inline SharedQuaternion ToGlmQuat(const physx::PxQuat& q);
        // --- Conversion Helper Functions (GLM <-> PhysX) ---

        physx::PxFilterFlags CustomFilterShader(
            physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
            physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
            physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize);

        class PhysicsEngine {
        public:
            PhysicsEngine();
            ~PhysicsEngine();

            // Non-copyable and non-movable
            PhysicsEngine(const PhysicsEngine&) = delete;
            PhysicsEngine& operator=(const PhysicsEngine&) = delete;
            PhysicsEngine(PhysicsEngine&&) = delete;
            PhysicsEngine& operator=(PhysicsEngine&&) = delete;

            // Initialization and Shutdown
            // Gravity vector defaults to (0,0,-9.81) using GLM-based SharedVec3
            bool Initialize(const SharedVec3& gravity = SharedVec3(0.0f, 0.0f, -9.81f), bool connect_to_pvd = true);
            void Shutdown();
            void StepSimulation(float delta_time_sec);

            // Material Management
            physx::PxMaterial* CreateMaterial(float static_friction, float dynamic_friction, float restitution);

            // Character Controller Management
            physx::PxController* CreateCharacterController(uint64_t player_id, const SharedVec3& initial_position, float radius, float height, physx::PxMaterial* material = nullptr, void* user_data_for_controller_actor = nullptr);
            void RegisterPlayerController(uint64_t player_id, physx::PxController* controller);
            void UnregisterPlayerController(uint64_t player_id);
            physx::PxController* GetPlayerController(uint64_t player_id) const;
            bool SetCharacterControllerOrientation(uint64_t player_id, const SharedQuaternion& orientation);
            uint32_t MoveCharacterController(physx::PxController* controller, const SharedVec3& world_space_displacement, float delta_time_sec, const std::vector<physx::PxController*>& other_controllers_to_ignore = {});
            void SetCharacterControllerPose(physx::PxController* controller, const SharedVec3& world_position);
            SharedVec3 GetCharacterControllerPosition(physx::PxController* controller) const;

            physx::PxRigidStatic* CreateTerrain(
                uint64_t zone_id, // A unique ID for this terrain zone
                const std::vector<SharedVec3>& vertices,
                const std::vector<uint32_t>& indices,
                EPhysicsObjectType object_type,
                physx::PxMaterial* material
            );

            // Actor User Data
            void SetActorUserData(physx::PxActor* actor, void* userData);


            physx::PxRigidDynamic* CreatePhysicsProjectileActor(
                const RiftForged::Physics::ProjectilePhysicsProperties& properties,
                EPhysicsObjectType projectile_type,
                const SharedVec3& startPosition,
                const SharedVec3& initialVelocity,
                physx::PxMaterial* material = nullptr,
                void* userData = nullptr // <-- We now pass an opaque user data pointer
            );

            physx::PxRigidStatic* CreateHeightField(
                uint64_t terrainId,
                uint32_t numRows,
                uint32_t numCols,
                const std::vector<int16_t>& heightData, // Assuming height data is converted to 16-bit integers
                float heightScale,      // Vertical scaling
                float rowAndColScale,   // Horizontal scaling
                physx::PxMaterial* material
            );

            // Scene Queries (Raycasts, Sweeps, Overlaps)
            bool CapsuleSweepSingle(
                const SharedVec3& start_pos,
                const SharedQuaternion& orientation,
                float radius,
                float half_height,
                const SharedVec3& unit_direction,
                float max_distance,
                HitResult& out_hit_result, // HitResult now contains glm::vec3
                physx::PxRigidActor* actor_to_ignore = nullptr, // Default to nullptr
                const physx::PxQueryFilterData& filter_data = physx::PxQueryFilterData(
                    physx::PxQueryFlags(physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::ePREFILTER) // Sensible default flags
                ),
                physx::PxQueryFilterCallback* filter_callback = nullptr
            );

            bool RaycastSingle(
                const SharedVec3& start,
                const SharedVec3& unit_direction,
                float max_distance,
                HitResult& out_hit,
                const physx::PxQueryFilterData& filter_data = physx::PxQueryFilterData(physx::PxQueryFlags(physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::ePREFILTER)),
                physx::PxQueryFilterCallback* filter_callback = nullptr
            );

            std::vector<HitResult> RaycastMultiple(
                const SharedVec3& start,
                const SharedVec3& unit_direction,
                float max_distance,
                uint32_t max_hits,
                const physx::PxQueryFilterData& filter_data = physx::PxQueryFilterData(physx::PxQueryFlags(physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::ePREFILTER | physx::PxQueryFlag::eNO_BLOCK)),
                physx::PxQueryFilterCallback* filter_callback = nullptr
            );

            std::vector<HitResult> OverlapMultiple(
                const physx::PxGeometry& geometry,
                const physx::PxTransform& pose,
                uint32_t max_hits,
                const physx::PxQueryFilterData& filter_data = physx::PxQueryFilterData(physx::PxQueryFlags(physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::ePREFILTER | physx::PxQueryFlag::eNO_BLOCK | physx::PxQueryFlag::eANY_HIT)),
                physx::PxQueryFilterCallback* filter_callback = nullptr
            );

            // Static Actor Creation
            physx::PxRigidStatic* CreateStaticBox(uint64_t entity_id, const SharedVec3& position, const SharedQuaternion& orientation, const SharedVec3& half_extents, EPhysicsObjectType object_type, physx::PxMaterial* material = nullptr, void* user_data = nullptr);
            physx::PxRigidStatic* CreateStaticSphere(uint64_t entity_id, const SharedVec3& position, float radius, EPhysicsObjectType object_type, physx::PxMaterial* material = nullptr, void* user_data = nullptr);
            physx::PxRigidStatic* CreateStaticCapsule(uint64_t entity_id, const SharedVec3& position, const SharedQuaternion& orientation, float radius, float half_height, EPhysicsObjectType object_type, physx::PxMaterial* material = nullptr, void* user_data = nullptr);
            physx::PxRigidStatic* CreateStaticPlane(const SharedVec3& normal, float distance, EPhysicsObjectType object_type, physx::PxMaterial* material = nullptr);
            physx::PxRigidStatic* CreateStaticTriangleMesh(uint64_t entity_id, const std::vector<SharedVec3>& vertices, const std::vector<uint32_t>& indices, EPhysicsObjectType object_type, const SharedVec3& scale_vec = SharedVec3(1.0f, 1.0f, 1.0f), physx::PxMaterial* material = nullptr, void* user_data = nullptr);

            // Dynamic Actor Creation
            physx::PxRigidDynamic* CreateDynamicBox(uint64_t entity_id, const SharedVec3& position, const SharedQuaternion& orientation, const SharedVec3& half_extents, float density, EPhysicsObjectType object_type, physx::PxMaterial* material = nullptr, void* user_data = nullptr);
            physx::PxRigidDynamic* CreateDynamicSphere(uint64_t entity_id, const SharedVec3& position, float radius, float density, EPhysicsObjectType object_type, physx::PxMaterial* material = nullptr, void* user_data = nullptr);
            physx::PxRigidDynamic* CreateDynamicCapsule(uint64_t entity_id, const SharedVec3& position, const SharedQuaternion& orientation, float radius, float half_height, float density, EPhysicsObjectType object_type, physx::PxMaterial* material = nullptr, void* user_data = nullptr);

            // Rigid Actor Management (Generic)
            void RegisterRigidActor(uint64_t entity_id, physx::PxRigidActor* actor);
            void UnregisterRigidActor(uint64_t entity_id);
            physx::PxRigidActor* GetRigidActor(uint64_t entity_id) const;

            // Forces and Impulses
            void ApplyForceToActor(physx::PxRigidBody* actor, const SharedVec3& force, physx::PxForceMode::Enum mode, bool wakeup = true);
            void ApplyForceToActorById(uint64_t entity_id, const SharedVec3& force, physx::PxForceMode::Enum mode, bool wakeup = true);

            // Conceptual Advanced Physics Features (Stubs)
            void CreateRadialForceField(uint64_t instigator_id, const SharedVec3& center, float strength, float radius, float duration_sec, bool is_push, float falloff = 1.0f);
            void ApplyLocalizedGravity(const SharedVec3& center, float strength, float radius, float duration_sec, const SharedVec3& gravity_direction);
            bool DeformTerrainRegion(const SharedVec3& impact_point, float radius, float depth_or_intensity, int deformation_type);

            // Accessors for Core PhysX Objects (use with caution)
            physx::PxScene* GetScene() const { return m_scene; }
            physx::PxPhysics* GetPhysics() const { return m_physics; }
            physx::PxFoundation* GetFoundation() const { return m_foundation; }

            // For external PVD connection management if needed
            void SetPvdTransport(physx::PxPvdTransport* transport) { m_pvd_transport = transport; }

            physx::PxMaterial* GetDefaultMaterial() const { return m_default_material; }

        private:
            // Core PhysX Objects
            physx::PxFoundation* m_foundation = nullptr;
            physx::PxPhysics* m_physics = nullptr;
            physx::PxDefaultCpuDispatcher* m_dispatcher = nullptr;
            physx::PxScene* m_scene = nullptr;
            physx::PxMaterial* m_default_material = nullptr;
            physx::PxControllerManager* m_controller_manager = nullptr;
            physx::PxPvd* m_pvd = nullptr;
            physx::PxPvdTransport* m_pvd_transport = nullptr;
            physx::PxCudaContextManager* m_cudaContextManager = nullptr;

            // Default query filter data (can be configured during initialization)
            // Many query functions now take PxQueryFilterData as a parameter with a default,
            // so this member might be for a more specific globally configurable default if needed.
            physx::PxQueryFilterData m_default_query_filter_data;

            // Actor and Controller Storage
            std::map<uint64_t, physx::PxController*> m_playerControllers;
            mutable std::mutex m_playerControllersMutex; // Protects m_playerControllers

            std::map<uint64_t, physx::PxRigidActor*> m_entityActors;
            mutable std::mutex m_entityActorsMutex;      // Protects m_entityActors

            // General Mutex for PhysX operations that need broader locking (e.g., scene writes)
            mutable std::mutex m_physicsMutex;

            // Private Helper Methods
            void SetupShapeFiltering(physx::PxShape* shape, const CollisionFilterData& filter_data);

        };

    } // namespace Physics
} // namespace RiftForged