// File: Server/ShardEngine.h (New)
#pragma once

#include <memory>
#include <vector>
#include <deque>
#include <mutex>

// We need the clean GameCommand definition, as shards will have their own command queues.
#include <RiftForged/GameLogic/GameCommands/GameCommands.hh>

// Forward declarations for the systems this shard will OWN
namespace RiftForged {
    namespace Gameplay { class GameplayEngine; }
    namespace Physics { class PhysicsEngine; }
    namespace GameLogic { class PlayerManager; }
    namespace Core { class TerrainManager; } // Needed for loading zones
}

namespace RiftForged {
    namespace Server {

        class ShardEngine {
        public:
            /**
             * @brief Constructs a new ShardEngine instance.
             * @param shardId A unique identifier for this world instance.
             * @param gameplayEngine The gameplay engine for this world.
             * @param physicsEngine The isolated physics simulation for this world.
             * @param playerManager The manager for players only within this shard.
             */
            ShardEngine(
                uint32_t shardId,
                std::unique_ptr<Gameplay::GameplayEngine> gameplayEngine,
                std::unique_ptr<Physics::PhysicsEngine> physicsEngine,
                std::unique_ptr<GameLogic::PlayerManager> playerManager
            );
            ~ShardEngine();

            // Delete copy/assignment operators
            ShardEngine(const ShardEngine&) = delete;
            ShardEngine& operator=(const ShardEngine&) = delete;

            /**
             * @brief The main update method, called by ServerEngine's master SimulationTick.
             * @param deltaTime The time elapsed since the last tick.
             */
            void Update(float deltaTime);

            /**
             * @brief Queues a game command for a player in this shard.
             * This will be called by the MessageDispatcher.
             * @param command The command to be processed on the next tick for this shard.
             */
            void PushCommand(const GameLogic::Commands::GameCommand& command);

            /**
             * @brief Loads the visual and physics mesh for a zone into this shard.
             * @param zoneName The name of the terrain asset to load.
             * @param worldPosition The position to place the terrain.
             * @param zoneId A unique identifier for the zone.
             * @param terrainManager A reference to the global terrain manager service.
             * @return True if successful, false otherwise.
             */
            bool LoadZone(const std::string& zoneName, const glm::vec3& worldPosition, uint64_t zoneId, Core::TerrainManager& terrainManager);

            // Provides access to the PlayerManager for this specific shard.
            GameLogic::PlayerManager& GetPlayerManager();


        private:
            void ProcessPlayerCommands();
            void PublishDirtyStatesToCache(); // Placeholder for S2C logic

            uint32_t m_shardId;

            //--- Simulation System Ownership ---
            // The ShardEngine now OWNS the core systems for its isolated world.
            std::unique_ptr<Gameplay::GameplayEngine> m_gameEngine;
            std::unique_ptr<Physics::PhysicsEngine> m_physicsEngine;
            std::unique_ptr<GameLogic::PlayerManager> m_playerManager;

            //--- Shard-Specific Command Queue ---
            std::deque<GameLogic::Commands::GameCommand> m_commandQueue;
            std::mutex m_commandQueueMutex;
        };

    } // namespace Server
} // namespace RiftForged