// File: GameServer/GameServerEngine.h
// RiftForged Game Development Team
// Copyright (c) 2025-2028 RiftForged Game Development Team

#pragma once

#include <vector>
#include <chrono>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <any>      // For storing various command types
#include <deque>
#include <map>
#include <string>
#include <optional> // For GetEndpointForPlayerId
#include <memory>   // For std::unique_ptr

// Core Game Logic/Engine Includes
#include "../GameEngine/GameplayEngine.h"
#include "../GameEngine/PlayerManager.h"
#include "../PhysicsEngine/PhysicsEngine.h"

// Networking
#include "../NetworkEngine/UDPPacketHandler.h"
#include "../NetworkEngine/NetworkEndpoint.h"

// Database/Cache Service
#include "CacheService.h" 

// FlatBuffers Declarations
#include "../FlatBuffers/Versioning/V0.0.5/riftforged_common_types_generated.h"
#include "../FlatBuffers/Versioning/V0.0.5/riftforged_c2s_udp_messages_generated.h"
#include "../FlatBuffers/Versioning/V0.0.5/riftforged_s2c_udp_messages_generated.h"
#include "../FlatBuffers/Versioning/V0.0.5/riftforged_player_state_cache_generated.h"

// Utilities
#include "../Utilities/Logger.h"
#include "../Utilities/MathUtil.h"
#include "../Utilities/ThreadPool.h"

// Aliases
namespace RF_C2S = RiftForged::Networking::UDP::C2S;
namespace RF_S2C = RiftForged::Networking::UDP::S2C;
namespace RF_Shared = RiftForged::Networking::Shared;
namespace RF_Net = RiftForged::Networking;
namespace RF_GameLogic = RiftForged::GameLogic;
namespace RF_Physics = RiftForged::Physics;
namespace RF_ThreadPool = RiftForged::Utilities::Threading;
namespace RiftForged { namespace Core { class TerrainManager; } }

namespace RiftForged {
    namespace Server {

        class GameServerEngine {
        public:
            GameServerEngine(
                RiftForged::GameLogic::PlayerManager& playerManager,
                RiftForged::Gameplay::GameplayEngine& gameplayEngine,
                RiftForged::Physics::PhysicsEngine& physicsEngine,
                Core::TerrainManager& terrainManager,
                size_t numThreadPoolThreads = 0,
                std::chrono::milliseconds tickInterval = std::chrono::milliseconds(5)
            );

            // Explicitly define destructor in .h and implement in .cpp to handle unique_ptr to incomplete type
            ~GameServerEngine();

            GameServerEngine(const GameServerEngine&) = delete;
            GameServerEngine& operator=(const GameServerEngine&) = delete;

            bool Initialize();
            void StartSimulationLoop();
            void StopSimulationLoop();
            void Shutdown();

            // --- Session Management ---
            uint64_t OnClientAuthenticatedAndJoining(const Networking::NetworkEndpoint& newEndpoint, const std::string& characterIdToLoad = "");
            void OnClientDisconnected(const Networking::NetworkEndpoint& endpoint);
            uint64_t GetPlayerIdForEndpoint(const Networking::NetworkEndpoint& endpoint) const;
            std::optional<Networking::NetworkEndpoint> GetEndpointForPlayerId(uint64_t playerId) const;
            void QueueClientJoinRequest(const Networking::NetworkEndpoint& endpoint, const std::string& characterIdToLoad);
            std::vector<Networking::NetworkEndpoint> GetAllActiveSessionEndpoints() const;

            // --- Incoming Command Submission ---
            void SubmitPlayerCommand(uint64_t playerId, std::any commandPayload);

            // --- Getters ---
            GameLogic::PlayerManager& GetPlayerManager();
            const GameLogic::PlayerManager& GetPlayerManager() const;
            RF_ThreadPool::TaskThreadPool& GetGameLogicThreadPool();
            const RF_ThreadPool::TaskThreadPool& GetGameLogicThreadPool() const;
            bool isSimulating() const;
            uint16_t GetServerTickRateHz() const;
            Core::TerrainManager& GetTerrainManager() { return m_terrainManager; }


            void SetPacketHandler(Networking::UDPPacketHandler* handler);

        private:
            // --- Main Loop and Processing ---
            void SimulationTick();
            void ProcessPlayerCommands();
            void ProcessJoinRequests();
            void ProcessDisconnectRequests();

            // --- NEW: Asynchronous Cache Publishing ---
            void PublishDirtyPlayerStates();
            void CacheUpdateWorker();

            // --- Data Structures for Queued Operations ---
            struct QueuedPlayerCommand {
                uint64_t playerId;
                std::any commandPayload;
            };
            struct ClientJoinRequest {
                Networking::NetworkEndpoint endpoint;
                std::string characterIdToLoad;
            };

            // --- Core Components (Dependencies) ---
            GameLogic::PlayerManager& m_playerManager;
            Gameplay::GameplayEngine& m_gameplayEngine;
            Physics::PhysicsEngine& m_physicsEngine;
            Core::TerrainManager& m_terrainManager;
            Networking::UDPPacketHandler* m_packetHandlerPtr;

            // --- Threading and Task Management ---
            RF_ThreadPool::TaskThreadPool m_gameLogicThreadPool;
            std::atomic<bool> m_isSimulatingThread;
            std::thread m_simulationThread;
            std::chrono::milliseconds m_tickIntervalMs;
            std::mutex m_shutdownThreadMutex;
            std::condition_variable m_shutdownThreadCv;

            // --- Session and Command Queues ---
            std::deque<QueuedPlayerCommand> m_incomingCommandQueue;
            std::mutex m_commandQueueMutex;

            std::deque<ClientJoinRequest> m_joinRequestQueue;
            std::mutex m_joinRequestQueueMutex;

            std::deque<Networking::NetworkEndpoint> m_disconnectRequestQueue;
            std::mutex m_disconnectRequestQueueMutex;

            // --- Session Mapping ---
            std::map<std::string, uint64_t> m_endpointKeyToPlayerIdMap;
            std::map<uint64_t, Networking::NetworkEndpoint> m_playerIdToEndpointMap;
            mutable std::mutex m_sessionMapsMutex;

            // --- NEW: Cache Service and Queue ---
            std::unique_ptr<CacheService> m_cacheService;
            std::deque<std::vector<uint8_t>> m_playerStateUpdateQueue;
            std::mutex m_playerStateUpdateQueueMutex;

            bool m_timerResolutionWasSet;

            bool LoadInitialZone();

        };

    } // namespace Server
} // namespace RiftForged