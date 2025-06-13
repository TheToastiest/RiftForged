// File: GameServer/GameServerEngine.cpp
// RiftForged Game Development Team
// Copyright (c) 2025-2028 RiftForged Game Development Team

#include "GameServerEngine.h"
#include "CacheService.h"
#include <sstream>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>
#pragma comment(lib, "Winmm.lib")
#endif

// FlatBuffers includes
#include "../FlatBuffers/Versioning/V0.0.5/riftforged_c2s_udp_messages_generated.h"
#include "../FlatBuffers/Versioning/V0.0.5/riftforged_s2c_udp_messages_generated.h"
#include "../FlatBuffers/Versioning/V0.0.5/riftforged_common_types_generated.h"
#include "../FlatBuffers/Versioning/V0.0.5/riftforged_player_state_cache_generated.h"

// Project includes
#include "../Utilities/MathUtil.h"
#include "../GameEngine/PlayerManager.h"
#include "../GameEngine/GameplayEngine.h"
#include "../PhysicsEngine/PhysicsEngine.h"
#include "../TerrainManager.h" // Now include the full header
#include "../PhysicsEngine/PhysicsEngine.h" // For EPhysicsObjectType

namespace RiftForged {
    namespace Server {

        GameServerEngine::GameServerEngine(
            RiftForged::GameLogic::PlayerManager& playerManager,
            RiftForged::Gameplay::GameplayEngine& gameplayEngine,
            RiftForged::Physics::PhysicsEngine& physicsEngine,
            Core::TerrainManager& terrainManager,
            size_t numThreadPoolThreads,
            std::chrono::milliseconds tickInterval)
            : m_playerManager(playerManager),
            m_gameplayEngine(gameplayEngine),
            m_packetHandlerPtr(nullptr),
            m_physicsEngine(physicsEngine),
            m_terrainManager(terrainManager),
            m_gameLogicThreadPool(numThreadPoolThreads),
            m_isSimulatingThread(false),
            m_tickIntervalMs(tickInterval),
            m_timerResolutionWasSet(false)
        {
            RF_CORE_INFO("GameServerEngine: Constructed. Tick Interval: {}ms", m_tickIntervalMs.count());

            const std::string DRAGONFLY_CONNECTION_STRING = "tcp://127.0.0.1:6379";
            m_cacheService = std::make_unique<CacheService>(DRAGONFLY_CONNECTION_STRING);

            if (!m_cacheService->IsConnected()) {
                RF_CORE_ERROR("GameServerEngine: Failed to connect to CacheService (DragonflyDB). State publishing will be disabled.");
            }
        }

        GameServerEngine::~GameServerEngine() = default;

        bool GameServerEngine::Initialize() {
            RF_CORE_INFO("GameServerEngine: Initializing...");
            RF_CORE_INFO("GameServerEngine: GameLogicThreadPool active with {} threads.", m_gameLogicThreadPool.getThreadCount());
            RF_CORE_INFO("GameServerEngine: Loading initial game world state...");
            LoadInitialZone();
            if (!LoadInitialZone()) {
                RF_CORE_CRITICAL("GameServerEngine initialization failed because the initial zone could not be loaded.");
                return false; // Propagate the failure
            }

            return true; // Success
        }
        
        bool GameServerEngine::LoadInitialZone() {
            RF_CORE_INFO("GameServerEngine: Loading zone 'FractalMountains_Main'...");

            std::string assetToUse = "ridged_terrain    ";
            glm::vec3 worldPosition = glm::vec3(0.0f, 0.0f, 0.0f);
            uint64_t zoneId = 1;

            Core::TerrainMeshData meshData = m_terrainManager.GenerateSingleTerrainMesh(assetToUse, worldPosition);

            if (meshData.vertices.empty()) {
                // We already log the critical error inside the if block
                return false; // <<< RETURN FAILURE
            }

            physx::PxMaterial* terrainMaterial = m_physicsEngine.GetDefaultMaterial();
            if (!terrainMaterial) {
                RF_CORE_CRITICAL("Failed to get default material from PhysicsEngine. Cannot create terrain.");
                return false; // <<< RETURN FAILURE
            }

            m_physicsEngine.CreateTerrain(
                zoneId,
                meshData.vertices,
                meshData.indices,
                Physics::EPhysicsObjectType::STATIC_IMPASSABLE,
                terrainMaterial
            );

            RF_CORE_INFO("GameServerEngine: Zone 'FractalMountains_Main' loaded successfully.");
            return true; // <<< RETURN SUCCESS
        }
    
        void GameServerEngine::Shutdown() {
            RF_CORE_INFO("GameServerEngine: Shutting down...");
            StopSimulationLoop();

            RF_CORE_INFO("GameServerEngine: Stopping GameLogicThreadPool...");
            m_gameLogicThreadPool.stop();
            RF_CORE_INFO("GameServerEngine: GameLogicThreadPool stopped.");
        }

        void GameServerEngine::StartSimulationLoop() {
            if (m_isSimulatingThread.load()) {
                RF_CORE_WARN("GameServerEngine: Simulation loop already running.");
                return;
            }
            RF_CORE_INFO("GameServerEngine: Starting simulation loop...");

#ifdef _WIN32
            if (timeBeginPeriod(1) == TIMERR_NOERROR) {
                m_timerResolutionWasSet = true;
                RF_CORE_INFO("GameServerEngine: Timer resolution successfully set to 1ms.");
            }
            else {
                RF_CORE_WARN("GameServerEngine: Failed to set timer resolution to 1ms.");
            }
#endif

            m_isSimulatingThread.store(true);
            try {
                m_simulationThread = std::thread(&GameServerEngine::SimulationTick, this);

                if (m_cacheService && m_cacheService->IsConnected()) {
                    RF_CORE_INFO("GameServerEngine: Starting CacheUpdateWorker thread...");
                    m_gameLogicThreadPool.enqueue(&GameServerEngine::CacheUpdateWorker, this);
                }

            }
            catch (const std::system_error& e) {
                RF_CORE_CRITICAL("GameServerEngine: Failed to create simulation thread: {}", e.what());
                m_isSimulatingThread.store(false);
#ifdef _WIN32
                if (m_timerResolutionWasSet) { timeEndPeriod(1); m_timerResolutionWasSet = false; }
#endif
            }
        }

        void GameServerEngine::StopSimulationLoop() {
            bool wasSimulating = m_isSimulatingThread.exchange(false);
            if (wasSimulating) {
                RF_CORE_INFO("GameServerEngine: Signaling simulation loop and workers to stop...");
                m_shutdownThreadCv.notify_one();
            }

            if (m_simulationThread.joinable() && m_simulationThread.get_id() != std::this_thread::get_id()) {
                m_simulationThread.join();
                RF_CORE_INFO("GameServerEngine: Simulation loop thread successfully joined.");
            }

#ifdef _WIN32
            if (m_timerResolutionWasSet) {
                if (timeEndPeriod(1) != TIMERR_NOERROR) { RF_CORE_ERROR("GameServerEngine: Failed to restore timer resolution."); }
                else { RF_CORE_INFO("GameServerEngine: Timer resolution successfully restored."); }
                m_timerResolutionWasSet = false;
            }
#endif
        }

        RF_ThreadPool::TaskThreadPool& GameServerEngine::GetGameLogicThreadPool() { return m_gameLogicThreadPool; }
        const RF_ThreadPool::TaskThreadPool& GameServerEngine::GetGameLogicThreadPool() const { return m_gameLogicThreadPool; }
        RF_GameLogic::PlayerManager& GameServerEngine::GetPlayerManager() { return m_playerManager; }
        const RF_GameLogic::PlayerManager& GameServerEngine::GetPlayerManager() const { return m_playerManager; }
        void GameServerEngine::SetPacketHandler(RF_Net::UDPPacketHandler* handler) { m_packetHandlerPtr = handler; }
        bool GameServerEngine::isSimulating() const { return m_isSimulatingThread.load(); }
        uint16_t GameServerEngine::GetServerTickRateHz() const {
            return (m_tickIntervalMs.count() > 0) ? static_cast<uint16_t>(1000 / m_tickIntervalMs.count()) : 0;
        }

        std::vector<RF_Net::NetworkEndpoint> GameServerEngine::GetAllActiveSessionEndpoints() const {
            std::lock_guard<std::mutex> lock(m_sessionMapsMutex);
            std::vector<RF_Net::NetworkEndpoint> endpoints;
            endpoints.reserve(m_playerIdToEndpointMap.size());
            for (const auto& pair : m_playerIdToEndpointMap) {
                endpoints.push_back(pair.second);
            }
            return endpoints;
        }

        uint64_t GameServerEngine::GetPlayerIdForEndpoint(const RF_Net::NetworkEndpoint& endpoint) const {
            std::lock_guard<std::mutex> lock(m_sessionMapsMutex);
            auto it = m_endpointKeyToPlayerIdMap.find(endpoint.ToString());
            return (it != m_endpointKeyToPlayerIdMap.end()) ? it->second : 0;
        }

        std::optional<RiftForged::Networking::NetworkEndpoint> GameServerEngine::GetEndpointForPlayerId(uint64_t playerId) const {
            // CORRECTED: Lock the actual mutex, not the map.
            std::lock_guard<std::mutex> lock(m_sessionMapsMutex);

            auto it = m_playerIdToEndpointMap.find(playerId);
            if (it != m_playerIdToEndpointMap.end()) {
                return it->second;
            }
            return std::nullopt;
        }

        void GameServerEngine::QueueClientJoinRequest(const Networking::NetworkEndpoint& endpoint, const std::string& characterIdToLoad) {
            std::lock_guard<std::mutex> lock(m_joinRequestQueueMutex);
            m_joinRequestQueue.push_back({ endpoint, characterIdToLoad });
        }

        void GameServerEngine::SubmitPlayerCommand(uint64_t playerId, std::any commandPayload) {
            if (playerId == 0) return;
            std::lock_guard<std::mutex> lock(m_commandQueueMutex);
            m_incomingCommandQueue.push_back({ playerId, std::move(commandPayload) });
        }

        void GameServerEngine::ProcessJoinRequests() {
            std::deque<ClientJoinRequest> requestsToProcess;
            {
                std::lock_guard<std::mutex> lock(m_joinRequestQueueMutex);
                if (m_joinRequestQueue.empty()) return;
                requestsToProcess.swap(m_joinRequestQueue);
            }
            for (const auto& req : requestsToProcess) {
                // Note: This logic assumes the JoinRequestMessageHandler has already handled the S2C response.
                // This function just creates the player.
                OnClientAuthenticatedAndJoining(req.endpoint, req.characterIdToLoad);
            }
        }

        void GameServerEngine::ProcessDisconnectRequests() {
            std::deque<Networking::NetworkEndpoint> requestsToProcess;
            {
                std::lock_guard<std::mutex> lock(m_disconnectRequestQueueMutex);
                if (m_disconnectRequestQueue.empty()) return;
                requestsToProcess.swap(m_disconnectRequestQueue);
            }
            for (const auto& ep : requestsToProcess) {
                OnClientDisconnected(ep);
            }
        }

        uint64_t GameServerEngine::OnClientAuthenticatedAndJoining(
            const RiftForged::Networking::NetworkEndpoint& newEndpoint,
            const std::string& characterIdToLoad) {

            std::string endpointKey = newEndpoint.ToString();
            RF_CORE_INFO("GameServerEngine: Client joining from endpoint [{}]. Character to load: '{}'", endpointKey, characterIdToLoad.empty() ? "New/Default" : characterIdToLoad);

            {
                std::lock_guard<std::mutex> lock(m_sessionMapsMutex);
                if (m_endpointKeyToPlayerIdMap.count(endpointKey)) {
                    RF_CORE_WARN("GameServerEngine: Endpoint [{}] already associated with PlayerId {}.", endpointKey, m_endpointKeyToPlayerIdMap[endpointKey]);
                    return 0; // Failure
                }
            }

            uint64_t newPlayerId = m_playerManager.GetNextAvailablePlayerID();
            if (newPlayerId == 0) {
                RF_CORE_CRITICAL("GameServerEngine: PlayerManager returned invalid new PlayerId (0).");
                return 0;
            }

            Utilities::Math::Vec3 spawnPos(0.f, 0.f, 1.5f);
            Utilities::Math::Quaternion spawnOrient(1.f, 0.f, 0.f, 0.f);

            GameLogic::ActivePlayer* player = m_playerManager.CreatePlayer(newPlayerId, spawnPos, spawnOrient);
            if (!player) {
                RF_CORE_ERROR("GameServerEngine: Failed to create ActivePlayer for PlayerId {}.", newPlayerId);
                return 0;
            }

            {
                std::lock_guard<std::mutex> lock(m_sessionMapsMutex);
                m_endpointKeyToPlayerIdMap[endpointKey] = newPlayerId;
                m_playerIdToEndpointMap[newPlayerId] = newEndpoint;
            }

            m_gameplayEngine.InitializePlayerInWorld(player, spawnPos, spawnOrient);
            RF_CORE_INFO("GameServerEngine: Player {} successfully created and initialized for endpoint [{}].", newPlayerId, endpointKey);
            return newPlayerId;
        }

        void GameServerEngine::OnClientDisconnected(const RiftForged::Networking::NetworkEndpoint& endpoint) {
            std::string endpointKey = endpoint.ToString();
            RF_CORE_INFO("GameServerEngine: Client disconnected from endpoint [{}]", endpointKey);

            uint64_t playerIdToDisconnect = 0;
            {
                std::lock_guard<std::mutex> lock(m_sessionMapsMutex);
                auto it = m_endpointKeyToPlayerIdMap.find(endpointKey);
                if (it != m_endpointKeyToPlayerIdMap.end()) {
                    playerIdToDisconnect = it->second;
                    m_endpointKeyToPlayerIdMap.erase(it);
                    m_playerIdToEndpointMap.erase(playerIdToDisconnect);
                }
                else {
                    RF_CORE_WARN("GameServerEngine: Received disconnect for unknown or already removed endpoint [{}].", endpointKey);
                    return;
                }
            }

            if (playerIdToDisconnect != 0) {
                RF_CORE_INFO("GameServerEngine: Processing disconnect for PlayerId {}.", playerIdToDisconnect);
                m_physicsEngine.UnregisterPlayerController(playerIdToDisconnect);
                m_physicsEngine.UnregisterRigidActor(playerIdToDisconnect);
                m_playerManager.RemovePlayer(playerIdToDisconnect);
            }
        }

        void GameServerEngine::ProcessPlayerCommands() {
            std::deque<QueuedPlayerCommand> commandsToProcess;
            {
                std::lock_guard<std::mutex> lock(m_commandQueueMutex);
                if (m_incomingCommandQueue.empty()) return;
                commandsToProcess.swap(m_incomingCommandQueue);
            }

            for (const auto& queuedCmd : commandsToProcess) {
                GameLogic::ActivePlayer* player = m_playerManager.FindPlayerById(queuedCmd.playerId);
                if (!player) continue;

                try {
                    if (queuedCmd.commandPayload.type() == typeid(RF_C2S::C2S_MovementInputMsgT)) {
                        const auto& cmd = std::any_cast<const RF_C2S::C2S_MovementInputMsgT&>(queuedCmd.commandPayload);
                        if (cmd.local_direction_intent) {
                            player->last_processed_movement_intent = Utilities::Math::Vec3(cmd.local_direction_intent->x(), cmd.local_direction_intent->y(), cmd.local_direction_intent->z());
                            player->was_sprint_intended = cmd.is_sprinting;
                        }
                    }
                    else if (queuedCmd.commandPayload.type() == typeid(RF_C2S::C2S_TurnIntentMsgT)) {
                        const auto& cmd = std::any_cast<const RF_C2S::C2S_TurnIntentMsgT&>(queuedCmd.commandPayload);
                        m_gameplayEngine.TurnPlayer(player, cmd.turn_delta_degrees);
                    }
                    // NOTE: Logic to build and send S2C responses for RiftStep, BasicAttack, etc.
                    // would live here, after calling the GameplayEngine. This is where you would
                    // take the 'AttackOutcome' or 'RiftStepOutcome' and use a FlatBufferBuilder
                    // just like we did in the individual message handler files.
                    // This logic is omitted here but belongs in this function.

                }
                catch (const std::bad_any_cast& e) {
                    RF_CORE_ERROR("GameServerEngine::ProcessPlayerCommands: Bad std::any_cast for player {}: {}", queuedCmd.playerId, e.what());
                }
            }
        }

        void GameServerEngine::SimulationTick() {
            std::stringstream ss;
            ss << std::this_thread::get_id();
            RF_CORE_INFO("GameServerEngine: SimulationTick thread started (ID: {})", ss.str());
            auto last_tick_time = std::chrono::steady_clock::now();

            while (m_isSimulatingThread.load()) {
                auto current_tick_start_time = std::chrono::steady_clock::now();
                float delta_time_sec = std::chrono::duration<float>(current_tick_start_time - last_tick_time).count();
                last_tick_time = current_tick_start_time;

                if (delta_time_sec <= 0.0f) { delta_time_sec = 0.001f; }
                if (delta_time_sec > 0.2f) { delta_time_sec = 0.2f; }

                // 1. Process incoming operations
                ProcessJoinRequests();
                ProcessDisconnectRequests();
                ProcessPlayerCommands();

                // 2. Update all players' movement and game logic
                auto players_for_update = m_playerManager.GetAllActivePlayerPointersForUpdate();
                for (auto* player : players_for_update) {
                    if (player) {
                        m_gameplayEngine.ProcessMovement(player, player->last_processed_movement_intent, player->was_sprint_intended, delta_time_sec);
                    }
                }

                // 3. Step the physics simulation
                m_physicsEngine.StepSimulation(delta_time_sec);

                // 4. Reconcile game state with physics state
                for (auto* player : players_for_update) {
                    if (player && player->playerId != 0) {
                        if (auto* px_controller = m_physicsEngine.GetPlayerController(player->playerId)) {
                            player->SetPosition(m_physicsEngine.GetCharacterControllerPosition(px_controller));
                        }
                    }
                }

                // 5. Publish updated state to the cache for other servers
                PublishDirtyPlayerStates();

                // 6. Synchronize state to clients over the network
                auto players_for_sync = std::as_const(m_playerManager).GetAllActivePlayerPointersForUpdate();
                for (const auto* player : players_for_sync) {
                    if (player && player->isDirty.load()) {
                        if (auto endpointOpt = GetEndpointForPlayerId(player->playerId)) {
                            flatbuffers::FlatBufferBuilder builder(1024);

                            RF_Shared::Vec3 pos_val(player->position.x, player->position.y, player->position.z);
                            RF_Shared::Quaternion orient_val(player->orientation.x, player->orientation.y, player->orientation.z, player->orientation.w);

                            auto effects_vec = builder.CreateVector(reinterpret_cast<const uint32_t*>(player->activeStatusEffects.data()), player->activeStatusEffects.size());
                            uint64_t ts = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

                            auto payload = RF_S2C::CreateS2C_EntityStateUpdateMsg(builder, player->playerId, &pos_val, &orient_val, player->currentHealth, player->maxHealth, player->currentWill, player->maxWill, ts, player->animationStateId, effects_vec);
                            auto root_msg = RF_S2C::CreateRoot_S2C_UDP_Message(builder, RF_S2C::S2C_UDP_Payload_EntityStateUpdate, payload.Union());
                            builder.Finish(root_msg);

                            if (m_packetHandlerPtr) {
                                m_packetHandlerPtr->SendUnreliablePacket(endpointOpt.value(), RF_S2C::S2C_UDP_Payload::S2C_UDP_Payload_EntityStateUpdate, builder.Release());
                            }
                        }
                        const_cast<RF_GameLogic::ActivePlayer*>(player)->isDirty.store(false);
                    }
                }

                // 7. Control tick rate
                auto sleep_for = m_tickIntervalMs - (std::chrono::steady_clock::now() - current_tick_start_time);
                if (m_isSimulatingThread.load() && sleep_for > std::chrono::milliseconds(0)) {
                    std::unique_lock<std::mutex> lock(m_shutdownThreadMutex);
                    m_shutdownThreadCv.wait_for(lock, sleep_for, [this] { return !m_isSimulatingThread.load(); });
                }
            }
        }

        void GameServerEngine::PublishDirtyPlayerStates() {
            if (!m_cacheService || !m_cacheService->IsConnected()) {
                return;
            }

            auto players_to_check = m_playerManager.GetAllActivePlayerPointersForUpdate();

            for (const GameLogic::ActivePlayer* player : players_to_check) {
                if (player && player->isDirty.load()) {
                    flatbuffers::FlatBufferBuilder builder;

                    // 1. Prepare complex types (vectors) first to get their offsets
                    auto status_effects_vec_offset = builder.CreateVector(
                        reinterpret_cast<const uint32_t*>(player->activeStatusEffects.data()),
                        player->activeStatusEffects.size()
                    );

                    // 2. Create temporary FlatBuffer structs for the Vec3/Quats we'll need pointers to
                    auto pos = RF_Shared::Vec3(player->position.x, player->position.y, player->position.z);
                    auto orient = RF_Shared::Quaternion(player->orientation.x, player->orientation.y, player->orientation.z, player->orientation.w);
                    auto vel = RF_Shared::Vec3(0, 0, 0); // Placeholder for actual velocity

                    // 3. Use the PlayerStateCacheBuilder
                    RiftForged::Cache::PlayerStateCacheBuilder state_builder(builder);

                    // 4. Add all fields one by one
                    state_builder.add_player_id(player->playerId);
                    state_builder.add_position(&pos);
                    state_builder.add_orientation(&orient);
                    state_builder.add_velocity(&vel);
                    state_builder.add_current_health(player->currentHealth);
                    state_builder.add_max_health(player->maxHealth);
                    state_builder.add_active_status_effects(status_effects_vec_offset);
                    // No timestamp, as we deprecated it
                    state_builder.add_current_zone_id(1); // Placeholder for zone_id

                    // 5. Finalize the table and the buffer
                    auto state_cache_offset = state_builder.Finish();
                    builder.Finish(state_cache_offset); // This is the correct way to finish

                    // 6. Push the data to the queue
                    const uint8_t* buffer_ptr = builder.GetBufferPointer();
                    const size_t size = builder.GetSize();
                    std::vector<uint8_t> payload(buffer_ptr, buffer_ptr + size);

                    {
                        std::lock_guard<std::mutex> lock(m_playerStateUpdateQueueMutex);
                        m_playerStateUpdateQueue.push_back(std::move(payload));
                    }
                }
            }
        }
        void GameServerEngine::CacheUpdateWorker() {
            RF_CORE_INFO("CacheUpdateWorker thread started.");
            while (m_isSimulatingThread.load()) {
                std::vector<uint8_t> payload;
                {
                    std::lock_guard<std::mutex> lock(m_playerStateUpdateQueueMutex);
                    if (!m_playerStateUpdateQueue.empty()) {
                        payload = std::move(m_playerStateUpdateQueue.front());
                        m_playerStateUpdateQueue.pop_front();
                    }
                }

                if (!payload.empty()) {
                    auto state_cache = RiftForged::Cache::GetPlayerStateCache(payload.data());
                    m_cacheService->PublishPlayerState(state_cache->player_id(), payload);
                }
                else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
            RF_CORE_INFO("CacheUpdateWorker thread exiting.");
        }

    } // namespace Server
} // namespace RiftForged