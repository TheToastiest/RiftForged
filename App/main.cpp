// File: GameServer/Main.cpp
// RiftForged Game Development Team
// Copyright (c) 2023-2025 RiftForged Game Development Team

#include <iostream>
#include <string>
#include <vector>
#include <atomic>
#include <chrono> // For std::chrono
#include <memory>
#include <thread> // For std::this_thread::sleep_for

// Core Engine Components - Only include what's strictly necessary for this physics/terrain test
#include <RiftForged/Core/TerrainManager/TerrainManager.h>
#include <RiftForged/Core/TerrainData/TerrainData.h> // For HeightmapData, TerrainMeshData, etc.

// Physics Engine
#include <RiftForged/Physics/PhysicsEngine/PhysicsEngine.h>

// Utilities
#include <RiftForged/Utilities/Logger/Logger.h>
// AssetLoader is used internally by TerrainManager, so not directly included here
// #include <RiftForged/Utilities/AssetLoader.h>

// Forward declarations for types that are part of other modules, but not included in this minimal test
// For ServerEngine constructor stub, if you keep the GameServerEngine instance declaration
// Otherwise, remove these if you're truly only instantiating PhysicsEngine and TerrainManager
//namespace RiftForged { namespace GameLogic { class PlayerManager; } }
//namespace RiftForged { namespace Gameplay { class GameplayEngine; } }
//namespace RiftForged { namespace Server { class ServerEngine; } } // Renamed from GameServerEngine


// Global flag for simulation loop (can be local if desired)
std::atomic<bool> g_isServerRunning = true;

int main() {
    std::cout << "RiftForged GameServer Starting (MINIMAL Physics/Terrain Test Mode)..." << std::endl;

    RiftForged::Utilities::Logger::Init();
    RF_CORE_INFO("Logger Initialized.");

    const unsigned short SERVER_PORT = 12345;
    const std::string LISTEN_IP_ADDRESS = "0.0.0.0";
    const size_t GAME_LOGIC_THREAD_POOL_SIZE = 12; // Example: 12 threads for the game logic pool
    const std::chrono::milliseconds GAME_TICK_INTERVAL_MS(5); // Approx 60 TPS

    // --- Declare only strictly necessary components for this test ---
    RiftForged::Physics::PhysicsEngine physicsEngine;
    RiftForged::Core::TerrainManager terrainManager;

    // test_box pointer, initialized after successful terrain creation
    physx::PxRigidDynamic* test_box = nullptr;

    try {
        RF_CORE_INFO("Initializing core systems...");

        if (!physicsEngine.Initialize()) {
            RF_CORE_CRITICAL("Server: PhysicsEngine initialization failed. Exiting.");
            return 1;
        }
        RF_CORE_INFO("PhysicsEngine initialized.");

        // --- TERRAIN ASSET REGISTRATION (Directly in main for test) ---
        RF_CORE_INFO("Registering terrain asset types for test...");
        // Define paths relative to your executable or working directory.
        // Assuming FileNameZX (no extension) is correct, and AssetLoader handles it.
        terrainManager.RegisterTerrainType("ridged_terrain", {
            /*filePath*/        "assets/Terrains/Binary Files/RidgeThroughTerrainXZ", // No extension, as per your tool
            /*numRows*/         1025,
            /*numCols*/         1025,
            /*heightScale*/     25.0f, // Scale for height
            /*horizontalScale*/ 20.0f  // Scale for X and Z dimensions
            });
        // You can add other terrain registrations here if you want to test them.
        RF_CORE_INFO("Terrain asset types registered.");

        // --- TERRAIN DEBUG TEST: Generate mesh and create actor using the PROVEN CreateTerrain ---
        RF_CORE_INFO("[DEBUG_TEST] Attempting to generate terrain mesh and create PhysX triangle mesh terrain actor.");

        const std::string assetName = "ridged_terrain";

        // Call your existing TerrainManager::GenerateSingleTerrainMesh function
        // It handles loading raw data, converting to int16_t, and then generating glm::vec3 vertices and uint32_t indices.
        // Note: GenerateSingleTerrainMesh takes a 'position' argument, often used for chunk offsets. For a single test terrain, pass (0,0,0).
        RiftForged::Core::TerrainMeshData terrainMesh = terrainManager.GenerateSingleTerrainMesh(
            assetName,
            glm::vec3(0.0f, 0.0f, 0.0f) // Origin for the terrain mesh.
        );

        if (terrainMesh.vertices.empty() || terrainMesh.indices.empty()) {
            RF_CORE_CRITICAL("[DEBUG_TEST] GenerateSingleTerrainMesh failed for '{}'. Physics terrain will not be created. Exiting.", assetName);
            return 1;
        }

        RF_CORE_INFO("[DEBUG_TEST] Terrain mesh data generated. Vertices: {}, Indices: {}. Attempting to create PhysX triangle mesh terrain actor.",
            terrainMesh.vertices.size(), terrainMesh.indices.size());

        // Call your proven PhysicsEngine::CreateTerrain (which uses CreateStaticTriangleMesh internally)
        physx::PxRigidStatic* terrain_actor_test = physicsEngine.CreateTerrain(
            1, // Unique ID for this test terrain
            terrainMesh.vertices, // Pass the generated vertices (glm::vec3)
            terrainMesh.indices,  // Pass the generated indices (uint32_t)
            RiftForged::Physics::EPhysicsObjectType::STATIC_IMPASSABLE, // Physics type for the terrain
            physicsEngine.GetDefaultMaterial() // Default PhysX material
        );

        if (!terrain_actor_test) {
            RF_CORE_CRITICAL("[DEBUG_TEST] CreateTerrain (triangle mesh) FAILED. Check PhysicsEngine::CreateStaticTriangleMesh implementation or generated mesh data.");
            // Create a fallback box if terrain creation fails
            physx::PxRigidDynamic* fallback_box = physicsEngine.CreateDynamicBox(
                100, // entity_id
                RiftForged::Physics::SharedVec3(0.0f, 0.0f, 50.0f), // position high above fallback
                RiftForged::Physics::SharedQuaternion(1.0f, 0.0f, 0.0f, 0.0f), // identity orientation
                RiftForged::Physics::SharedVec3(1.0f, 1.0f, 1.0f), // half-extents (1x1x1m cube)
                1.0f, // density (1kg/m^3)
                RiftForged::Physics::EPhysicsObjectType::INTERACTABLE_OBJECT,
                physicsEngine.GetDefaultMaterial()
            );
            if (fallback_box) {
                RF_CORE_INFO("Fallback test dynamic box created.");
                test_box = fallback_box; // Assign to test_box for simulation
            }
        }
        else {
            RF_CORE_INFO("[DEBUG_TEST] CreateTerrain (triangle mesh) SUCCEEDED! Terrain created. Visual check in PVD required.");
            // If terrain created, create a box to fall on it.
            test_box = physicsEngine.CreateDynamicBox(
                100, // entity_id
                RiftForged::Physics::SharedVec3(0.0f, 0.0, 20000.0f), // Start high above (Y is often up in PhysX)
                RiftForged::Physics::SharedQuaternion(1.0f, 0.0f, 0.0f, 0.0f), // identity orientation
                RiftForged::Physics::SharedVec3(1.0f, 1.0f, 1.0f), // half-extents (1x1x1m cube)
                1.0f, // density (1kg/m^3)
                RiftForged::Physics::EPhysicsObjectType::INTERACTABLE_OBJECT,
                physicsEngine.GetDefaultMaterial()
            );
            if (!test_box) {
                RF_CORE_ERROR("Failed to create test dynamic box on successful terrain creation.");
            }
        }

        // --- MAIN SIMULATION LOOP (Simplified for local physics/terrain test) ---
        bool simulation_running = true; // Local flag for this specific loop
        const float fixed_delta_time = 1.0f / 60.0f; // 60 FPS physics step
        auto last_frame_time = std::chrono::high_resolution_clock::now();

        RF_CORE_INFO("Starting simplified physics/terrain simulation loop...");
        while (simulation_running) {
            auto current_time = std::chrono::high_resolution_clock::now();
            physicsEngine.StepSimulation(fixed_delta_time);

            // --- Log box position ---
            if (test_box) {
                RiftForged::Physics::SharedVec3 pos = RiftForged::Physics::FromPxVec3(test_box->getGlobalPose().p);
                static auto last_log_time = current_time;
                if (std::chrono::duration<float>(current_time - last_log_time).count() > 0.5f) { // Log every 0.5 sec approx
                    RF_CORE_INFO("Box position: ({:.3f}, {:.3f}, {:.3f})", pos.x, pos.y, pos.z);
                    last_log_time = current_time;
                }

                // Basic condition to stop simulation if box falls very far or is stable
                if (pos.x < -1000.0f) { // Assuming Y is up in your world, if box falls very far below origin
                    RF_CORE_INFO("Box fell too far, ending simulation.");
                    simulation_running = false;
                }
            }
            else {
                // If there's no box (e.g., dynamic box creation failed), just run for a few seconds as a sanity check
                static float total_sim_time = 0.0f;
                total_sim_time += fixed_delta_time;
                if (total_sim_time > 10.0f) { // Run for 10 seconds without a box
                    simulation_running = false;
                }
            }

            // Add a small sleep to prevent 100% CPU usage in this simple loop
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        RF_CORE_INFO("Simplified physics/terrain simulation loop ended.");

    }
    catch (const std::exception& e) {
        RF_CORE_CRITICAL("Server: Unhandled standard exception during physics test: {}", e.what());
    }
    catch (...) {
        RF_CORE_CRITICAL("Server: An unknown, unhandled exception occurred during physics test.");
    }

    // --- Graceful Shutdown Sequence ---
    RF_CORE_INFO("MAIN: Initiating graceful server shutdown (physics test mode)...");

    physicsEngine.Shutdown(); // Explicitly shutdown physics engine
    RF_CORE_INFO("MAIN: Flushing and shutting down logger...");
    RiftForged::Utilities::Logger::FlushAll();
    RiftForged::Utilities::Logger::Shutdown();

    std::cout << "MAIN: Server shut down gracefully." << std::endl;
    return 0;
}