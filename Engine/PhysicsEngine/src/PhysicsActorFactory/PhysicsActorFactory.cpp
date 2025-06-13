#include <RiftForged/Physics/PhysicsEngine/PhysicsEngine.h>
#include <RiftForged/Utilities/Logger/Logger.h>

#include "cooking/PxCooking.h"
#include "cooking/PxTriangleMeshDesc.h"
#include "extensions/PxDefaultStreams.h"
#include "extensions/PxRigidActorExt.h"
#include "geometry/PxBoxGeometry.h"
#include "geometry/PxCapsuleGeometry.h"
#include "geometry/PxSphereGeometry.h"
#include "geometry/PxPlaneGeometry.h"
#include "geometry/PxTriangleMeshGeometry.h"
#include "geometry/PxMeshScale.h"

namespace RiftForged {
    namespace Physics {

        physx::PxMaterial* PhysicsEngine::CreateMaterial(float static_friction, float dynamic_friction, float restitution) {
            std::lock_guard<std::mutex> lock(m_physicsMutex);
            if (!m_physics) { RF_PHYSICS_ERROR("CreateMaterial: PxPhysics not initialized."); return nullptr; }
            return m_physics->createMaterial(static_friction, dynamic_friction, restitution);
        }

        physx::PxController* PhysicsEngine::CreateCharacterController(uint64_t player_id, const SharedVec3& initial_position, float radius, float height, physx::PxMaterial* material, void* user_data_for_controller_actor) {
            if (!m_controller_manager) { RF_PHYSICS_ERROR("CreateCharacterController: PxControllerManager not initialized."); return nullptr; }
            physx::PxCapsuleControllerDesc desc;
            desc.height = height;
            desc.radius = radius;
            desc.position = physx::PxExtendedVec3(initial_position.x, initial_position.y, initial_position.z);
            desc.material = material ? material : m_default_material;
            desc.stepOffset = 0.5f;
            desc.slopeLimit = cos(glm::radians(45.0f));
            desc.upDirection = physx::PxVec3(0.0f, 0.0f, 1.0f);
            if (!desc.isValid()) { RF_PHYSICS_ERROR("CreateCharacterController: PxCapsuleControllerDesc is invalid."); return nullptr; }
            physx::PxController* controller = m_controller_manager->createController(desc);
            if (controller) {
                physx::PxRigidActor* actor = controller->getActor();
                if (actor) {
                    SetActorUserData(actor, user_data_for_controller_actor ? user_data_for_controller_actor : reinterpret_cast<void*>(player_id));
                    physx::PxShape* shape;
                    if (actor->getShapes(&shape, 1) == 1) {
                        CollisionFilterData filter_data;
                        filter_data.word0 = static_cast<physx::PxU32>(EPhysicsObjectType::PLAYER_CHARACTER);
                        SetupShapeFiltering(shape, filter_data);
                    }
                }
                RegisterPlayerController(player_id, controller);
            }
            else { RF_PHYSICS_ERROR("CreateCharacterController: createController failed."); }
            return controller;
        }

        physx::PxRigidStatic* PhysicsEngine::CreateStaticTriangleMesh(uint64_t entity_id, const std::vector<SharedVec3>& vertices, const std::vector<uint32_t>& indices, EPhysicsObjectType object_type, const SharedVec3& scale_vec, physx::PxMaterial* material, void* user_data) {
            (void)object_type;
            std::lock_guard<std::mutex> lock(m_physicsMutex);
            if (!m_physics || !m_scene || vertices.empty() || indices.empty()) return nullptr;
            std::vector<physx::PxVec3> px_vertices;
            px_vertices.reserve(vertices.size());
            for (const auto& v : vertices) px_vertices.push_back(ToPxVec3(v));
            physx::PxTriangleMeshDesc mesh_desc;
            mesh_desc.points.count = static_cast<physx::PxU32>(px_vertices.size()); // Line 64: ADD static_cast
            mesh_desc.points.stride = sizeof(physx::PxVec3);
            mesh_desc.points.data = px_vertices.data();
            mesh_desc.triangles.count = static_cast<physx::PxU32>(indices.size() / 3); // Line 67: ADD static_cast
            mesh_desc.triangles.stride = 3 * sizeof(uint32_t);
            mesh_desc.triangles.data = indices.data();
            physx::PxCookingParams params(m_physics->getTolerancesScale());
            params.meshPreprocessParams = physx::PxMeshPreprocessingFlag::eWELD_VERTICES;
            params.meshWeldTolerance = 0.01f;
            physx::PxDefaultMemoryOutputStream write_buffer;
            if (!PxCookTriangleMesh(params, mesh_desc, write_buffer)) { RF_PHYSICS_ERROR("PxCookTriangleMesh FAILED."); return nullptr; }
            physx::PxDefaultMemoryInputData read_buffer(write_buffer.getData(), write_buffer.getSize());
            physx::PxTriangleMesh* tri_mesh = m_physics->createTriangleMesh(read_buffer);
            if (!tri_mesh) { RF_PHYSICS_ERROR("createTriangleMesh FAILED."); return nullptr; }
            physx::PxMaterial* mat = material ? material : m_default_material;
            physx::PxMeshScale mesh_scale(ToPxVec3(scale_vec));
            physx::PxTriangleMeshGeometry geometry(tri_mesh, mesh_scale);
            physx::PxRigidStatic* actor = m_physics->createRigidStatic(physx::PxTransform(physx::PxIdentity));
            physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(*actor, geometry, *mat);
            tri_mesh->release();
            CollisionFilterData filter_data;
            filter_data.word0 = static_cast<physx::PxU32>(object_type);
            SetupShapeFiltering(shape, filter_data);
            SetActorUserData(actor, user_data ? user_data : reinterpret_cast<void*>(entity_id));
            m_scene->addActor(*actor);
            RegisterRigidActor(entity_id, actor);
            return actor;
        }

        physx::PxRigidStatic* PhysicsEngine::CreateStaticPlane(const SharedVec3& normal, float distance, EPhysicsObjectType object_type, physx::PxMaterial* material) {
			(void)object_type; // Not used in this function, but kept for consistency
            std::lock_guard<std::mutex> lock(m_physicsMutex);
            if (!m_physics || !m_scene) return nullptr;
            physx::PxMaterial* mat = material ? material : m_default_material;
            physx::PxRigidStatic* actor = physx::PxCreatePlane(*m_physics, physx::PxPlane(ToPxVec3(normal), distance), *mat);
            m_scene->addActor(*actor);
            return actor;
        }

        physx::PxRigidStatic* PhysicsEngine::CreateStaticBox(uint64_t entity_id, const SharedVec3& position, const SharedQuaternion& orientation, const SharedVec3& half_extents, EPhysicsObjectType object_type, physx::PxMaterial* material, void* user_data) {
            std::lock_guard<std::mutex> lock(m_physicsMutex);
            if (!m_physics || !m_scene) return nullptr;
            physx::PxMaterial* mat = material ? material : m_default_material;
            physx::PxRigidStatic* actor = m_physics->createRigidStatic(physx::PxTransform(ToPxVec3(position), ToPxQuat(orientation)));
            physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(*actor, physx::PxBoxGeometry(ToPxVec3(half_extents)), *mat);
            CollisionFilterData filter_data;
            filter_data.word0 = static_cast<physx::PxU32>(object_type);
            SetupShapeFiltering(shape, filter_data);
            SetActorUserData(actor, user_data ? user_data : reinterpret_cast<void*>(entity_id));
            m_scene->addActor(*actor);
            RegisterRigidActor(entity_id, actor);
            return actor;
        }

        physx::PxRigidStatic* PhysicsEngine::CreateStaticSphere(uint64_t entity_id, const SharedVec3& position, float radius, EPhysicsObjectType object_type, physx::PxMaterial* material, void* user_data) {
            std::lock_guard<std::mutex> lock(m_physicsMutex);
            if (!m_physics || !m_scene) return nullptr;
            physx::PxMaterial* mat = material ? material : m_default_material;
            physx::PxRigidStatic* actor = m_physics->createRigidStatic(physx::PxTransform(ToPxVec3(position)));
            physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(*actor, physx::PxSphereGeometry(radius), *mat);
            CollisionFilterData filter_data;
            filter_data.word0 = static_cast<physx::PxU32>(object_type);
            SetupShapeFiltering(shape, filter_data);
            SetActorUserData(actor, user_data ? user_data : reinterpret_cast<void*>(entity_id));
            m_scene->addActor(*actor);
            RegisterRigidActor(entity_id, actor);
            return actor;
        }

        physx::PxRigidStatic* PhysicsEngine::CreateStaticCapsule(uint64_t entity_id, const SharedVec3& position, const SharedQuaternion& orientation, float radius, float half_height, EPhysicsObjectType object_type, physx::PxMaterial* material, void* user_data) {
            std::lock_guard<std::mutex> lock(m_physicsMutex);
            if (!m_physics || !m_scene) return nullptr;
            physx::PxMaterial* mat = material ? material : m_default_material;
            physx::PxRigidStatic* actor = m_physics->createRigidStatic(physx::PxTransform(ToPxVec3(position), ToPxQuat(orientation)));
            physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(*actor, physx::PxCapsuleGeometry(radius, half_height), *mat);
            CollisionFilterData filter_data;
            filter_data.word0 = static_cast<physx::PxU32>(object_type);
            SetupShapeFiltering(shape, filter_data);
            SetActorUserData(actor, user_data ? user_data : reinterpret_cast<void*>(entity_id));
            m_scene->addActor(*actor);
            RegisterRigidActor(entity_id, actor);
            return actor;
        }

        physx::PxRigidDynamic* PhysicsEngine::CreateDynamicBox(uint64_t entity_id, const SharedVec3& position, const SharedQuaternion& orientation, const SharedVec3& half_extents, float density, EPhysicsObjectType object_type, physx::PxMaterial* material, void* user_data) {
            std::lock_guard<std::mutex> lock(m_physicsMutex);
            if (!m_physics || !m_scene) return nullptr;
            physx::PxMaterial* mat = material ? material : m_default_material;
            physx::PxRigidDynamic* actor = m_physics->createRigidDynamic(physx::PxTransform(ToPxVec3(position), ToPxQuat(orientation)));
            physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(*actor, physx::PxBoxGeometry(ToPxVec3(half_extents)), *mat);
            CollisionFilterData filter_data;
            filter_data.word0 = static_cast<physx::PxU32>(object_type);
            SetupShapeFiltering(shape, filter_data);
            if (density > 0.0f) { physx::PxRigidBodyExt::updateMassAndInertia(*actor, density); }
            else { actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true); }
            SetActorUserData(actor, user_data ? user_data : reinterpret_cast<void*>(entity_id));
            m_scene->addActor(*actor);
            RegisterRigidActor(entity_id, actor);
            return actor;
        }

        physx::PxRigidDynamic* PhysicsEngine::CreateDynamicSphere(uint64_t entity_id, const SharedVec3& position, float radius, float density, EPhysicsObjectType object_type, physx::PxMaterial* material, void* user_data) {
            std::lock_guard<std::mutex> lock(m_physicsMutex);
            if (!m_physics || !m_scene) return nullptr;
            physx::PxMaterial* mat = material ? material : m_default_material;
            physx::PxRigidDynamic* actor = m_physics->createRigidDynamic(physx::PxTransform(ToPxVec3(position)));
            physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(*actor, physx::PxSphereGeometry(radius), *mat);
            CollisionFilterData filter_data;
            filter_data.word0 = static_cast<physx::PxU32>(object_type);
            SetupShapeFiltering(shape, filter_data);
            if (density > 0.0f) { physx::PxRigidBodyExt::updateMassAndInertia(*actor, density); }
            else { actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true); }
            SetActorUserData(actor, user_data ? user_data : reinterpret_cast<void*>(entity_id));
            m_scene->addActor(*actor);
            RegisterRigidActor(entity_id, actor);
            return actor;
        }

        physx::PxRigidDynamic* PhysicsEngine::CreateDynamicCapsule(uint64_t entity_id, const SharedVec3& position, const SharedQuaternion& orientation, float radius, float half_height, float density, EPhysicsObjectType object_type, physx::PxMaterial* material, void* user_data) {
            std::lock_guard<std::mutex> lock(m_physicsMutex);
            if (!m_physics || !m_scene) return nullptr;
            physx::PxMaterial* mat = material ? material : m_default_material;
            physx::PxRigidDynamic* actor = m_physics->createRigidDynamic(physx::PxTransform(ToPxVec3(position), ToPxQuat(orientation)));
            physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(*actor, physx::PxCapsuleGeometry(radius, half_height), *mat);
            CollisionFilterData filter_data;
            filter_data.word0 = static_cast<physx::PxU32>(object_type);
            SetupShapeFiltering(shape, filter_data);
            if (density > 0.0f) { physx::PxRigidBodyExt::updateMassAndInertia(*actor, density); }
            else { actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true); }
            SetActorUserData(actor, user_data ? user_data : reinterpret_cast<void*>(entity_id));
            m_scene->addActor(*actor);
            RegisterRigidActor(entity_id, actor);
            return actor;
        }

        // REFRACTORED to remove ProjectileGameData dependency
        physx::PxRigidDynamic* PhysicsEngine::CreatePhysicsProjectileActor(const ProjectilePhysicsProperties& properties, EPhysicsObjectType projectile_type, const SharedVec3& startPosition, const SharedVec3& initialVelocity, physx::PxMaterial* material, void* userData) {
            std::lock_guard<std::mutex> lock(m_physicsMutex);
            if (!m_physics || !m_scene) return nullptr;
            physx::PxMaterial* matToUse = material ? material : m_default_material;
            if (!matToUse) return nullptr;
            physx::PxRigidDynamic* projectileActor = m_physics->createRigidDynamic(physx::PxTransform(ToPxVec3(startPosition)));
            if (!projectileActor) return nullptr;
            physx::PxShape* projectileShape = nullptr;
            if (properties.halfHeight > 0.0f && properties.radius > 0.0f) {
                projectileShape = physx::PxRigidActorExt::createExclusiveShape(*projectileActor, physx::PxCapsuleGeometry(properties.radius, properties.halfHeight), *matToUse);
            }
            else if (properties.radius > 0.0f) {
                projectileShape = physx::PxRigidActorExt::createExclusiveShape(*projectileActor, physx::PxSphereGeometry(properties.radius), *matToUse);
            }
            else { projectileActor->release(); return nullptr; }
            if (!projectileShape) { projectileActor->release(); return nullptr; }
            CollisionFilterData filter_data;
            filter_data.word0 = static_cast<physx::PxU32>(projectile_type);
            SetupShapeFiltering(projectileShape, filter_data);
            if (properties.mass > 0.0f) { physx::PxRigidBodyExt::updateMassAndInertia(*projectileActor, properties.mass); }
            else { physx::PxRigidBodyExt::updateMassAndInertia(*projectileActor, 0.01f); }
            projectileActor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, !properties.enableGravity);
            if (properties.enableCCD) { projectileActor->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, true); }
            projectileActor->userData = userData;
            projectileActor->setLinearVelocity(ToPxVec3(initialVelocity));
            m_scene->addActor(*projectileActor);
            return projectileActor;
        }

        physx::PxRigidStatic* PhysicsEngine::CreateHeightField(
            uint64_t terrainId,
            uint32_t numRows,
            uint32_t numCols,
            const std::vector<int16_t>& heightData,
            float heightScale,
            float rowAndColScale,
            physx::PxMaterial* material
        ) {
            std::lock_guard<std::mutex> physics_lock(m_physicsMutex);
            if (!m_physics || !m_scene) {
                RF_PHYSICS_ERROR("CreateHeightField: Physics system not initialized for ID {}.", terrainId);
                return nullptr;
            }
            // Added static_cast<size_t> for robustness against C4267 warnings, if not already present
            if (heightData.size() != static_cast<size_t>(numRows) * numCols) {
                RF_PHYSICS_ERROR("CreateHeightField: Height data size ({}) does not match dimensions ({}x{}) for ID {}.",
                    heightData.size(), numRows, numCols, terrainId);
                return nullptr;
            }

            // 1. Describe the Heightfield properties
            physx::PxHeightFieldDesc hfDesc;
            hfDesc.nbRows = numRows;
            hfDesc.nbColumns = numCols;
            hfDesc.format = physx::PxHeightFieldFormat::eS16_TM;
            hfDesc.samples.data = heightData.data();
            hfDesc.samples.stride = sizeof(int16_t);

            hfDesc.flags = physx::PxHeightFieldFlags(); // <--- Set flags to an empty set.

            // --- DEBUG: Log a sample of the int16_t height data ---
            RF_PHYSICS_INFO("CreateHeightField: Debugging hfDesc samples before cooking.");
            size_t sample_count = static_cast<size_t>(hfDesc.nbRows) * hfDesc.nbColumns;
            if (sample_count > 0 && hfDesc.samples.data) {
                const int16_t* samples_ptr = static_cast<const int16_t*>(hfDesc.samples.data);

                // Log min/max of the actual int16_t data for sanity check
                int16_t min_h = std::numeric_limits<int16_t>::max();
                int16_t max_h = std::numeric_limits<int16_t>::min();
                for (size_t i = 0; i < sample_count; ++i) {
                    if (samples_ptr[i] < min_h) min_h = samples_ptr[i];
                    if (samples_ptr[i] > max_h) max_h = samples_ptr[i];
                }
                RF_PHYSICS_INFO("HF Data Range (int16_t): Min={}, Max={}", min_h, max_h);

                // Log a few corner samples
                RF_PHYSICS_INFO("Sample Heights (Top-Left, Top-Right, Bottom-Left, Bottom-Right):");
                if (hfDesc.nbRows >= 1 && hfDesc.nbColumns >= 1) {
                    RF_PHYSICS_INFO("  TL: {}", samples_ptr[0]);
                }
                if (hfDesc.nbRows >= 1 && hfDesc.nbColumns >= 1 && hfDesc.nbColumns > 1) {
                    RF_PHYSICS_INFO("  TR: {}", samples_ptr[hfDesc.nbColumns - 1]);
                }
                if (hfDesc.nbRows > 1 && hfDesc.nbColumns >= 1) {
                    RF_PHYSICS_INFO("  BL: {}", samples_ptr[(hfDesc.nbRows - 1) * hfDesc.nbColumns]);
                }
                if (hfDesc.nbRows > 1 && hfDesc.nbColumns > 1) {
                    RF_PHYSICS_INFO("  BR: {}", samples_ptr[(hfDesc.nbRows - 1) * hfDesc.nbColumns + (hfDesc.nbColumns - 1)]);
                }
            }
            else {
                RF_PHYSICS_ERROR("CreateHeightField: hfDesc.samples.data is null or sample_count is zero before cooking.");
            }

            // 2. Cook the heightfield description into a memory buffer
            physx::PxDefaultMemoryOutputStream writeBuffer;

            // --- Cooking Parameters based on YOUR PxCooking.h ---
            physx::PxCookingParams cookingParams(m_physics->getTolerancesScale());

            // Based on your PxCooking.h, valid flags for PxMeshPreprocessingFlag::Enum include:
            // eWELD_VERTICES
            // eDISABLE_CLEAN_MESH
            // eDISABLE_ACTIVE_EDGES_PRECOMPUTE
            // eFORCE_32BIT_INDICES
            // eENABLE_VERT_MAPPING
            // eENABLE_INERTIA

            // For heightfields, eWELD_VERTICES is commonly used to clean up mesh issues,
            // and meshWeldTolerance controls the welding distance.
            cookingParams.meshPreprocessParams = physx::PxMeshPreprocessingFlag::eWELD_VERTICES;
            cookingParams.meshWeldTolerance = 0.01f; // A small positive value is typical for welding

            // Call PxCookHeightField with explicit parameters (correct syntax)
            bool status = PxCookHeightField(hfDesc, writeBuffer); // <--- CORRECTED CALL SYNTAX (ONLY 2 ARGUMENTS)

            if (!status) {
                RF_PHYSICS_ERROR("CreateHeightField: PxCookHeightField failed for ID {}. Check heightfield descriptor data directly.", terrainId);
                RF_PHYSICS_ERROR("HF Desc: Rows={}, Cols={}, Format={}. Sample count={}", hfDesc.nbRows, hfDesc.nbColumns, static_cast<int>(hfDesc.format), hfDesc.nbRows * hfDesc.nbColumns);
                // You cannot log cookingParams here as they are not used in the call.
                return nullptr;
            }

            // 3. Create the runtime PxHeightField object by reading from the memory buffer
            physx::PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
            physx::PxHeightField* heightField = m_physics->createHeightField(readBuffer);
            if (!heightField) {
                RF_PHYSICS_ERROR("CreateHeightField: m_physics->createHeightField from stream failed for ID {}.", terrainId);
                return nullptr;
            }

            // 4. Create the Geometry instance for the shape
            physx::PxHeightFieldGeometry hfGeometry(
                heightField,
                physx::PxMeshGeometryFlags(), // No specific flags for geometry (e.g., eDOUBLE_SIDED)
                heightScale,
                rowAndColScale,
                rowAndColScale
            );

            // 5. Create the static actor
            physx::PxRigidStatic* actor = m_physics->createRigidStatic(physx::PxTransform(physx::PxIdentity));
            if (!actor) {
                RF_PHYSICS_ERROR("CreateHeightField: createRigidStatic failed for ID {}.", terrainId);
                heightField->release();
                return nullptr;
            }

            // 6. Create and attach the shape
            physx::PxMaterial* mat_to_use = material ? material : m_default_material;
            if (!mat_to_use) { // Added null check for material
                RF_PHYSICS_ERROR("CreateHeightField: Material is null for ID {}. Cannot create shape.", terrainId);
                actor->release();
                heightField->release();
                return nullptr;
            }
            physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(*actor, hfGeometry, *mat_to_use);
            if (!shape) {
                RF_PHYSICS_ERROR("CreateHeightField: createExclusiveShape failed for ID {}.", terrainId);
                actor->release();
                heightField->release();
                return nullptr;
            }

            heightField->release(); // Release heightfield object as it's now owned by the geometry/shape

            // 7. Final setup
            CollisionFilterData sim_filter_data;
            // Hardcoding to STATIC_IMPASSABLE, as object_type is not a parameter for this function.
            // If you need to pass object type, add it to the function signature.
            sim_filter_data.word0 = static_cast<physx::PxU32>(EPhysicsObjectType::STATIC_IMPASSABLE);
            SetupShapeFiltering(shape, sim_filter_data);

            SetActorUserData(actor, reinterpret_cast<void*>(terrainId));
            m_scene->addActor(*actor);
            RegisterRigidActor(terrainId, actor);

            RF_PHYSICS_INFO("PhysicsEngine: PxHeightField for entity ID {} created and registered. Dims: {}x{}, HScale: {}, RCScale: {}. ObjectType: {}",
                terrainId, numCols, numRows, heightScale, rowAndColScale, static_cast<int>(EPhysicsObjectType::STATIC_IMPASSABLE));
            return actor;
        }

        physx::PxRigidStatic* PhysicsEngine::CreateTerrain(
            uint64_t zone_id,
            const std::vector<SharedVec3>& vertices, // SharedVec3 is glm::vec3
            const std::vector<uint32_t>& indices,
            EPhysicsObjectType object_type,
            physx::PxMaterial* material
        ) {
            RF_PHYSICS_INFO("PhysicsEngine: Received request to create terrain for zone ID {}.", zone_id);

            // We simply call your existing, robust triangle mesh creation function.
            // This reuses your code and respects your threading model (m_physicsMutex).
            return CreateStaticTriangleMesh(
                zone_id,
                vertices,
                indices,
                object_type,
                SharedVec3(1.0f, 1.0f, 1.0f), // scale_vec, already applied in TerrainManager
                material,
                reinterpret_cast<void*>(zone_id) // user_data
            );
        }

    } // namespace Physics
} // namespace RiftForged