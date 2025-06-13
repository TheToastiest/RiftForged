#include <RiftForged/Physics/PhysicsEngine/PhysicsEngine.h>
#include <RiftForged/Utilities/Logger/Logger.h> // Simplified include path

// PhysX headers needed for initialization and lifecycle management
#include <extensions/PxDefaultErrorCallback.h>
#include <extensions/PxDefaultAllocator.h>
#include <extensions/PxDefaultCpuDispatcher.h>
#include <extensions/PxExtensionsAPI.h>
#include <pvd/PxPvd.h>
#include <cudamanager/PxCudaContextManager.h>
#include <PxSceneDesc.h>

namespace RiftForged {
    namespace Physics {

        static physx::PxDefaultErrorCallback gDefaultErrorCallback;
        static physx::PxDefaultAllocator gDefaultAllocatorCallback;

        PhysicsEngine::PhysicsEngine()
            : m_foundation(nullptr), m_physics(nullptr), m_dispatcher(nullptr), m_scene(nullptr),
            m_default_material(nullptr), m_controller_manager(nullptr), m_pvd(nullptr),
            m_pvd_transport(nullptr), m_cudaContextManager(nullptr)
        {
            RF_CORE_INFO("PhysicsEngine: Constructed.");
        }

        PhysicsEngine::~PhysicsEngine() {
            RF_CORE_INFO("PhysicsEngine: Destructor called. Ensuring Shutdown.");
            Shutdown();
        }

        bool PhysicsEngine::Initialize(const SharedVec3& gravityVec, bool connect_to_pvd) {
            RF_PHYSICS_INFO("PhysicsEngine: Initializing PhysX SDK version {}.{}.{}",
                PX_PHYSICS_VERSION_MAJOR, PX_PHYSICS_VERSION_MINOR, PX_PHYSICS_VERSION_BUGFIX);

            std::lock_guard<std::mutex> init_lock(m_physicsMutex);

            if (m_foundation) {
                RF_PHYSICS_WARN("PhysicsEngine: Already initialized. Please call Shutdown() first if re-initialization is intended.");
                return true;
            }

            m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
            if (!m_foundation) {
                RF_PHYSICS_CRITICAL("PhysicsEngine: PxCreateFoundation failed!");
                return false;
            }
            RF_PHYSICS_INFO("PhysicsEngine: PxFoundation created successfully.");

            if (connect_to_pvd) {
                RF_PHYSICS_INFO("PhysicsEngine: Attempting to connect to PhysX Visual Debugger (PVD)...");
                if (!m_pvd_transport) { // Only create if not already set (e.g., externally)
                    m_pvd_transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
                    if (!m_pvd_transport) {
                        RF_PHYSICS_WARN("PhysicsEngine: PxDefaultPvdSocketTransportCreate failed. PVD connection skipped.");
                    }
                }

                if (m_pvd_transport) {
                    if (!m_pvd) m_pvd = physx::PxCreatePvd(*m_foundation);
                    if (m_pvd) {
                        if (m_pvd->connect(*m_pvd_transport, physx::PxPvdInstrumentationFlag::eALL)) {
                            RF_PHYSICS_INFO("PhysicsEngine: PVD connection successful.");
                        }
                        else {
                            RF_PHYSICS_WARN("PhysicsEngine: PVD connect failed. PVD will not be available.");
                            // Consider releasing m_pvd if connect fails and we created it
                            // m_pvd->release(); m_pvd = nullptr; 
                        }
                    }
                    else {
                        RF_PHYSICS_WARN("PhysicsEngine: PxCreatePvd failed. PVD unavailable.");
                        // If PVD creation failed, release the transport if we created it
                        // This logic needs refinement if transport can be set externally and PVD creation fails
                        // For simplicity, if PVD creation fails, we might not want to keep the transport around
                        // m_pvd_transport->release(); m_pvd_transport = nullptr;
                    }
                }
            }
            else {
                RF_PHYSICS_INFO("PhysicsEngine: PVD connection explicitly disabled.");
            }

            physx::PxTolerancesScale tolerances_scale; // Use default scale
            bool recordMemoryAllocations = true; // Useful for debugging with PVD
            m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, tolerances_scale, recordMemoryAllocations, m_pvd);
            if (!m_physics) {
                RF_PHYSICS_CRITICAL("PhysicsEngine: PxCreatePhysics failed!");
                // Perform partial shutdown for resources created so far
                if (m_pvd) { if (m_pvd->isConnected()) m_pvd->disconnect(); m_pvd->release(); m_pvd = nullptr; }
                if (m_pvd_transport) { m_pvd_transport->release(); m_pvd_transport = nullptr; } // Assuming we own it if PxCreatePhysics fails
                if (m_foundation) { m_foundation->release(); m_foundation = nullptr; }
                return false;
            }
            RF_PHYSICS_INFO("PhysicsEngine: PxPhysics created successfully.");

            if (!PxInitExtensions(*m_physics, m_pvd)) {
                RF_PHYSICS_CRITICAL("PhysicsEngine: PxInitExtensions failed!");
                if (m_physics) { m_physics->release(); m_physics = nullptr; }
                if (m_pvd) { if (m_pvd->isConnected()) m_pvd->disconnect(); m_pvd->release(); m_pvd = nullptr; }
                if (m_pvd_transport) { m_pvd_transport->release(); m_pvd_transport = nullptr; }
                if (m_foundation) { m_foundation->release(); m_foundation = nullptr; }
                return false;
            }
            RF_PHYSICS_INFO("PhysicsEngine: PxExtensions initialized successfully.");

            m_default_material = m_physics->createMaterial(0.5f, 0.5f, 0.1f); // staticFriction, dynamicFriction, restitution
            if (!m_default_material) {
                RF_PHYSICS_CRITICAL("PhysicsEngine: Default PxMaterial creation failed!");
                Shutdown();
                return false;
            }
            RF_PHYSICS_INFO("PhysicsEngine: Default PxMaterial created.");

            bool gpuContextIsValid = false;
            physx::PxCudaContextManagerDesc cudaContextManagerDesc;
            m_cudaContextManager = PxCreateCudaContextManager(*m_foundation, cudaContextManagerDesc, PxGetProfilerCallback());
            // Using nullptr for PxProfilerCallback as it was in the original code snippet
            m_cudaContextManager = PxCreateCudaContextManager(*m_foundation, cudaContextManagerDesc, nullptr);
            if (m_cudaContextManager) {
                RF_PHYSICS_INFO("PhysicsEngine: PxCudaContextManager created.");
                if (m_cudaContextManager->contextIsValid()) {
                    RF_PHYSICS_INFO("PhysicsEngine: CUDA context is VALID. GPU acceleration will be attempted.");
                    gpuContextIsValid = true;
                }
                else {
                    RF_PHYSICS_WARN("PhysicsEngine: CUDA context created but is NOT valid. Releasing CudaManager. GPU acceleration disabled.");
                    m_cudaContextManager->release();
                    m_cudaContextManager = nullptr;
                }
            }
            else {
                RF_PHYSICS_WARN("PhysicsEngine: PxCreateCudaContextManager failed. GPU acceleration disabled.");
            }

            // Use std::thread::hardware_concurrency() if available, otherwise fallback
            // uint32_t num_hardware_threads = std::thread::hardware_concurrency(); 
            uint32_t num_hardware_threads = 4; // Original hardcoded value
            uint32_t num_threads_for_dispatcher = (num_hardware_threads > 1) ? num_hardware_threads - 1 : 1; // Ensure at least 1 thread
            if (num_hardware_threads == 0) { // Should not happen with a fixed value, but good check if using hardware_concurrency()
                RF_PHYSICS_WARN("PhysicsEngine: Could not determine hardware concurrency or 0 reported, defaulting CPU dispatcher to 1 thread.");
                num_threads_for_dispatcher = 1;
            }
            m_dispatcher = physx::PxDefaultCpuDispatcherCreate(num_threads_for_dispatcher);
            if (!m_dispatcher) {
                RF_PHYSICS_CRITICAL("PhysicsEngine: PxDefaultCpuDispatcherCreate failed!");
                Shutdown();
                return false;
            }
            RF_PHYSICS_INFO("PhysicsEngine: PxDefaultCpuDispatcher created with {} threads (Hardware reported/used: {}).",
                num_threads_for_dispatcher, num_hardware_threads);

            physx::PxSceneDesc scene_desc(m_physics->getTolerancesScale());
            scene_desc.gravity = ToPxVec3(gravityVec); // ToPxVec3 is from PhysicsEngine.h, takes SharedVec3 (glm::vec3)
            scene_desc.cpuDispatcher = m_dispatcher;
            scene_desc.filterShader = CustomFilterShader;
            // scene_desc.simulationEventCallback = this; // Requires PhysicsEngine to inherit from PxSimulationEventCallback

            scene_desc.flags |= physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;
            scene_desc.flags |= physx::PxSceneFlag::eENABLE_PCM;          // Persistent Contact Manifold
            scene_desc.flags |= physx::PxSceneFlag::eENABLE_STABILIZATION; // Helps stabilize stacks

            if (gpuContextIsValid && m_cudaContextManager) {
                scene_desc.cudaContextManager = m_cudaContextManager;
                scene_desc.flags |= physx::PxSceneFlag::eENABLE_GPU_DYNAMICS;
                scene_desc.broadPhaseType = physx::PxBroadPhaseType::eGPU;
                RF_PHYSICS_INFO("PhysicsEngine: PxSceneDesc configured for GPU simulation.");
            }
            else {
                scene_desc.broadPhaseType = physx::PxBroadPhaseType::ePABP; // A common CPU broadphase
                // scene_desc.flags &= ~physx::PxSceneFlag::eENABLE_GPU_DYNAMICS; // This flag isn't set by default
                RF_PHYSICS_INFO("PhysicsEngine: PxSceneDesc configured for CPU simulation.");
            }

            m_scene = m_physics->createScene(scene_desc);
            if (!m_scene) {
                RF_PHYSICS_CRITICAL("PhysicsEngine: m_physics->createScene failed!");
                Shutdown();
                return false;
            }
            // CORRECTED: Access .x, .y, .z for glm::vec3 (SharedVec3)
            RF_PHYSICS_INFO("PhysicsEngine: PxScene created. Gravity: ({:.2f}, {:.2f}, {:.2f})", gravityVec.x, gravityVec.y, gravityVec.z);

            if (m_pvd && m_pvd->isConnected() && m_scene) {
                physx::PxPvdSceneClient* pvdClient = m_scene->getScenePvdClient();
                if (pvdClient) {
                    pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
                    pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
                    pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
                }
            }

            m_controller_manager = PxCreateControllerManager(*m_scene);
            if (!m_controller_manager) {
                RF_PHYSICS_CRITICAL("PhysicsEngine: PxCreateControllerManager failed!");
                Shutdown();
                return false;
            }
            RF_PHYSICS_INFO("PhysicsEngine: PxControllerManager created.");

            m_default_query_filter_data = physx::PxQueryFilterData();
            m_default_query_filter_data.flags = physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::ePREFILTER;

            RF_PHYSICS_INFO("PhysicsEngine: Initialization successful.");
            return true;
        }

        void PhysicsEngine::Shutdown() {
            RF_PHYSICS_INFO("PhysicsEngine: Shutting down...");
            std::lock_guard<std::mutex> shutdown_lock(m_physicsMutex); // Ensure shutdown is also thread-safe

            if (m_controller_manager) { m_controller_manager->release(); m_controller_manager = nullptr; RF_PHYSICS_INFO("PhysicsEngine: PxControllerManager released."); }
            // Release actors and controllers stored in maps before releasing the scene or physics
            // This prevents issues if actors/controllers try to access a released scene/physics object
            {
                std::lock_guard<std::mutex> pc_lock(m_playerControllersMutex);
                for (auto& pair : m_playerControllers) {
                    if (pair.second) pair.second->release();
                }
                m_playerControllers.clear();
                RF_PHYSICS_INFO("PhysicsEngine: All player controllers released and map cleared.");
            }
            {
                std::lock_guard<std::mutex> ea_lock(m_entityActorsMutex);
                for (auto& pair : m_entityActors) {
                    if (pair.second) {
                        if (m_scene && pair.second->getScene()) { // Check if in scene before removing
                            m_scene->removeActor(*(pair.second), false); // Wake on Lost Touch = false
                        }
                        pair.second->release();
                    }
                }
                m_entityActors.clear();
                RF_PHYSICS_INFO("PhysicsEngine: All entity actors removed from scene, released, and map cleared.");
            }


            if (m_default_material) { m_default_material->release(); m_default_material = nullptr; RF_PHYSICS_INFO("PhysicsEngine: Default PxMaterial released."); }
            if (m_scene) { m_scene->release(); m_scene = nullptr; RF_PHYSICS_INFO("PhysicsEngine: PxScene released."); }
            if (m_dispatcher) { m_dispatcher->release(); m_dispatcher = nullptr; RF_PHYSICS_INFO("PhysicsEngine: PxDefaultCpuDispatcher released."); }
            if (m_cudaContextManager) { m_cudaContextManager->release(); m_cudaContextManager = nullptr; RF_PHYSICS_INFO("PhysicsEngine: PxCudaContextManager released."); }

            // PxCloseExtensions should be called before releasing PxPhysics.
            PxCloseExtensions(); RF_PHYSICS_INFO("PhysicsEngine: PxExtensions closed.");

            if (m_physics) { m_physics->release(); m_physics = nullptr; RF_PHYSICS_INFO("PhysicsEngine: PxPhysics released."); }

            // PVD cleanup
            if (m_pvd) {
                if (m_pvd->isConnected()) {
                    m_pvd->disconnect();
                }
                m_pvd->release();
                m_pvd = nullptr;
                RF_PHYSICS_INFO("PhysicsEngine: PVD released.");
            }
            if (m_pvd_transport) { // Release transport if it exists (could be set externally or created internally)
                m_pvd_transport->release();
                m_pvd_transport = nullptr;
                RF_PHYSICS_INFO("PhysicsEngine: PVD transport released.");
            }

            if (m_foundation) { m_foundation->release(); m_foundation = nullptr; RF_PHYSICS_INFO("PhysicsEngine: PxFoundation released."); }

            RF_PHYSICS_INFO("PhysicsEngine: Shutdown complete.");
        }

        void PhysicsEngine::StepSimulation(float delta_time_sec) {
            if (delta_time_sec <= 0.0f) {
                RF_PHYSICS_TRACE("PhysicsEngine::StepSimulation: delta_time_sec is zero or negative ({:.4f}s). Skipping simulation step.", delta_time_sec);
                return;
            }
            std::lock_guard<std::mutex> lock(m_physicsMutex); // Lock for scene operations
            if (!m_scene) {
                RF_PHYSICS_ERROR("PhysicsEngine::StepSimulation: Scene is null! Cannot simulate.");
                return;
            }
            m_scene->simulate(delta_time_sec);
            m_scene->fetchResults(true); // true = block until results are available
        }

    } // namespace Physics
} // namespace RiftForged