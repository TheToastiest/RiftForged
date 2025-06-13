// File: Server/ServerEngine.h (Refactored)
#pragma once

#include <vector>
#include <memory>
#include <chrono>
// ... other necessary includes for threading, session management etc.

// Forward declarations for systems it MANAGES or CREATES
namespace RiftForged {
    namespace Server {
        class ShardEngine;    // The new class for managing a world instance
        class CacheService;   // The existing cache service
    }
    namespace Core {
        class TerrainManager; // A dependency from the CoreExecutable
    }
    namespace Networking {
        class NetworkEndpoint;
    }
}

namespace RiftForged {
    namespace Server {

        class ServerEngine {
        public:
            // The constructor now only takes dependencies needed for global services and shard creation.
            ServerEngine(
                Core::TerrainManager& terrainManager,
                size_t numThreadPoolThreads = 0,
                std::chrono::milliseconds tickInterval = std::chrono::milliseconds(5)
            );
            ~ServerEngine();

            // Delete copy/assignment operators
            ServerEngine(const ServerEngine&) = delete;
            ServerEngine& operator=(const ServerEngine&) = delete;

            // --- Lifecycle Management ---
            bool Initialize();
            void StartSimulationLoop();
            void StopSimulationLoop();

            // --- Session Management (A Global Concern) ---
            uint64_t OnClientAuthenticatedAndJoining(const Networking::NetworkEndpoint& newEndpoint, const std::string& characterIdToLoad);
            void OnClientDisconnected(const Networking::NetworkEndpoint& endpoint);

        private:
            // --- The Master Clock ---
            void SimulationTick();

            // --- Global Services ---
            std::unique_ptr<CacheService> m_cacheService;
            Core::TerrainManager& m_terrainManager;

            // --- REMOVED ---
            // The ServerEngine no longer directly owns these simulation-specific systems.
            // GameLogic::PlayerManager& m_playerManager;
            // Gameplay::GameplayEngine& m_gameplayEngine;
            // Physics::PhysicsEngine& m_physicsEngine;

            // --- ADDED: Shard Management ---
            // It now owns and manages all the active game worlds.
            std::vector<std::unique_ptr<ShardEngine>> m_shards;

            // ... other existing members for session maps, threads, queues are appropriate here ...
        };

    } // namespace Server
} // namespace RiftForged