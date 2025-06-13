// File: Server/ServerEngine.cpp (Refactored)

#include <RiftForged/Server/ServerEngine/ServerEngine.h>
//#include <RiftForged/Shard/ShardEngine/ShardEngine.h> // Include the new ShardEngine definition
// ^ This doesn't exist yet

// Include headers for the systems it will CREATE and PASS to the shard
#include <RiftForged/GameEngine/GameplayEngine.h>
#include <RiftForged/Physics/PhysicsEngine/PhysicsEngine.h>
#include <RiftForged/GameLogic/PlayerManager/PlayerManager.h>
#include <RiftForged/Dispatch/PacketProcessor/PacketProcessor.h>
#include <RiftForged/Dispatch/Dispatchers/MessageDispatcher.h>
// ... and all the specific handlers for registration

#include <RiftForged/Server/CacheService/CacheService.h>
// ... other necessary includes

namespace RiftForged {
    namespace Server {

        // The constructor is now much simpler
        ServerEngine::ServerEngine(
            Core::TerrainManager& terrainManager,
            size_t numThreadPoolThreads,
            std::chrono::milliseconds tickInterval)
            : m_terrainManager(terrainManager),
            // ... other initializers for threads, queues, etc.
        {
            // Cache service is still a global service owned by the ServerEngine
            m_cacheService = std::make_unique<CacheService>(...);
        }

            ServerEngine::~ServerEngine() { /* ... */ }

        bool ServerEngine::Initialize() {
            RF_CORE_INFO("ServerEngine: Initializing...");

            // --- Create the C2S Dispatch Pipeline ---
            // The ServerEngine is responsible for creating and wiring up all the
            // global message handling systems.
            auto messageDispatcher = std::make_shared<Dispatch::MessageDispatcher>();

            // Create and register all handlers...
            // auto abilityHandler = std::make_shared<Dispatch::Handlers::AbilityMessageHandler>(...);
            // messageDispatcher->RegisterHandler<GameLogic::Commands::UseAbility>(abilityHandler);
            // ... etc for all handlers ...

            // The PacketProcessor needs the dispatcher to send commands to.
            auto packetProcessor = std::make_shared<Dispatch::PacketProcessor>(*messageDispatcher, *this);
            // Store the processor so the network layer can use it
            m_packetProcessor = packetProcessor;


            // --- Create the Main World Shard ---
            RF_CORE_INFO("ServerEngine: Creating main world shard...");

            auto playerManager = std::make_unique<GameLogic::PlayerManager>();
            auto physicsEngine = std::make_unique<Physics::PhysicsEngine>();
            // physicsEngine->Initialize(...); // Initialize physics

            // The GameplayEngine now takes raw pointers because its lifetime is managed by the shard
            auto gameplayEngine = std::make_unique<Gameplay::GameplayEngine>(*physicsEngine.get(), *playerManager.get());

            // Create the shard and give it ownership of its systems
            auto mainWorldShard = std::make_unique<ShardEngine>(
                std::move(gameplayEngine),
                std::move(physicsEngine),
                std::move(playerManager)
            );

            // Load the terrain into the shard's physics engine
            mainWorldShard->LoadZone("ridged_terrain", glm::vec3(0.0f), 1, m_terrainManager);

            m_shards.push_back(std::move(mainWorldShard));

            RF_CORE_INFO("ServerEngine: Main world shard created successfully.");
            return true;
        }

        void ServerEngine::SimulationTick() {
            // ... setup for tick timing ...

            while (m_isSimulatingThread.load()) {
                // ... calculate delta_time_sec ...

                // --- 1. Process Global Server Queues ---
                ProcessJoinRequests();
                ProcessDisconnectRequests();
                // This function now just de-queues raw packets and passes them to the PacketProcessor
                ProcessNetworkPackets();

                // --- 2. Update all active shards ---
                // This is the core change. The ServerEngine delegates the entire
                // simulation workload to its shards.
                for (auto& shard : m_shards) {
                    if (shard) {
                        shard->Update(delta_time_sec);
                    }
                }

                // --- ALL OTHER LOGIC IS GONE ---
                // The old steps for updating movement, stepping physics, reconciling state,
                // and sending S2C updates are GONE from this function. They are now
                // the responsibility of the ShardEngine's Update() loop and the
                // S2C event-driven systems we will build next.

                // ... control tick rate ...
            }
        }

        // The other functions like OnClientAuthenticated, OnClientDisconnected, etc.
        // would also be updated to find the correct shard (e.g., the first one for now)
        // and tell that shard's PlayerManager to create or remove the player.

    } // namespace Server
} // namespace RiftForged