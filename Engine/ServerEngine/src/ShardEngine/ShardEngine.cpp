// File: Server/ShardEngine.cpp (New)

#include <RiftForged/Server/ShardEngine/ShardEngine.h>

// Include the full definitions of the systems it owns and orchestrates
#include <RiftForged/GameEngine/GameplayEngine.h>
#include <RiftForged/Physics/PhysicsEngine/PhysicsEngine.h>
#include <RiftForged/GameLogic/PlayerManager/PlayerManager.h>
#include <RiftForged/Core/TerrainManager/TerrainManager.h>
#include <RiftForged/Utilities/Logger/Logger.h>

namespace RiftForged {
    namespace Server {

        // The constructor takes ownership of the simulation systems via std::move
        ShardEngine::ShardEngine(
            uint32_t shardId,
            std::unique_ptr<Gameplay::GameplayEngine> gameEngine,
            std::unique_ptr<Physics::PhysicsEngine> physicsEngine,
            std::unique_ptr<GameLogic::PlayerManager> playerManager)
            : m_shardId(shardId),
            m_gameEngine(std::move(gameEngine)),
            m_physicsEngine(std::move(physicsEngine)),
            m_playerManager(std::move(playerManager))
        {
            RF_CORE_INFO("ShardEngine [{}]: Constructed and owns its simulation systems.", m_shardId);
        }

        ShardEngine::~ShardEngine() {
            RF_CORE_INFO("ShardEngine [{}]: Destructing.", m_shardId);
        }

        // The main update loop for this shard, driven by the ServerEngine's master tick
        void ShardEngine::Update(float deltaTime) {
            // 1. Process all commands that have been queued for this specific shard
            ProcessPlayerCommands();

            // 2. Update core game logic (e.g., AI, scripts, cooldowns) for this shard
            // This was formerly part of the main loop in GameServerEngine.cpp
            m_gameEngine->UpdateWorldState(*m_playerManager, deltaTime);

            // 3. Step this shard's isolated physics simulation
            m_physicsEngine->StepSimulation(deltaTime);

            // 4. Reconcile game state with the new physics state
            m_gameEngine->ReconcilePhysicsState(*m_playerManager, *m_physicsEngine);

            // 5. Publish events for any state changes
            // This is where our Phase 2 S2C event system will plug in.
            // For now, this can be a placeholder.
            // PublishDirtyStatesToCache(); 
        }

        // This is the entry point for the MessageDispatcher to give this shard work.
        void ShardEngine::PushCommand(const GameLogic::Commands::GameCommand& command) {
            std::lock_guard<std::mutex> lock(m_commandQueueMutex);
            m_commandQueue.push_back(command);
        }

        // Takes the terrain loading logic from the old GameServerEngine
        bool ShardEngine::LoadZone(const std::string& zoneName, const glm::vec3& worldPosition, uint64_t zoneId, Core::TerrainManager& terrainManager) {
            RF_CORE_INFO("ShardEngine [{}]: Loading zone '{}'...", m_shardId, zoneName);

            // This logic is moved directly from your old GameServerEngine::LoadInitialZone
            Core::TerrainMeshData meshData = terrainManager.GenerateSingleTerrainMesh(zoneName, worldPosition);
            if (meshData.vertices.empty()) {
                return false;
            }

            physx::PxMaterial* terrainMaterial = m_physicsEngine->GetDefaultMaterial();
            if (!terrainMaterial) {
                RF_CORE_CRITICAL("ShardEngine [{}]: Failed to get default material from PhysicsEngine.", m_shardId);
                return false;
            }

            m_physicsEngine->CreateTerrain(zoneId, meshData.vertices, meshData.indices, Physics::EPhysicsObjectType::STATIC_IMPASSABLE, terrainMaterial);

            RF_CORE_INFO("ShardEngine [{}]: Zone '{}' loaded successfully.", m_shardId, zoneName);
            return true;
        }

        GameLogic::PlayerManager& ShardEngine::GetPlayerManager() {
            return *m_playerManager;
        }

        // This private function processes all commands queued for this shard
        void ShardEngine::ProcessPlayerCommands() {
            std::deque<GameLogic::Commands::GameCommand> commandsToProcess;
            {
                std::lock_guard<std::mutex> lock(m_commandQueueMutex);
                if (m_commandQueue.empty()) return;
                commandsToProcess.swap(m_commandQueue);
            }

            for (const auto& command : commandsToProcess) {
                // Here, we would call the appropriate GameplayEngine function
                // based on the command type, similar to how the old ProcessPlayerCommands worked,
                // but now it's clean and shard-local.

                // Example for a basic attack:
                if (const auto* attackIntent = std::get_if<GameLogic::Commands::BasicAttackIntent>(&command.data)) {
                    auto* player = m_playerManager->FindPlayerById(command.originatingPlayerID);
                    if (player) {
                        m_gameEngine->ExecuteBasicAttack(player, attackIntent->aimDirection, attackIntent->targetEntityId);
                    }
                }
                // ... cases for all other command types ...
            }
        }

    } // namespace Server
} // namespace RiftForged