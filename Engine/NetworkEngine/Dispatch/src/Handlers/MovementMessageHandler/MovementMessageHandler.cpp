// File: MovementMessageHandler.cpp
// RiftForged Development Team
// Copyright (c) 2023-2024 RiftForged Development Team

#include <RiftForged/Dispatch/Handlers/MovementMessageHandler/MovementMessageHandler.h>

// Game Logic & Engine systems we interact with
#include <RiftForged/GameLogic/PlayerManager/PlayerManager.h>
#include <RiftForged/GameEngine/GameEngine/GameEngine.h>

// Utilities
#include <RiftForged/Utilities/Logger/Logger.h>
#include <RiftForged/Utilities/MathUtils/MathUtils.h> // For Math::Vec3

// Standard library for safe variant access
#include <variant>

namespace RiftForged {
    namespace Dispatch {

        // Constructor implementation is unchanged.
        MovementMessageHandler::MovementMessageHandler(
            RiftForged::GameLogic::PlayerManager& playerManager,
            RiftForged::Gameplay::GameplayEngine& gameplayEngine,
            RiftForged::Utilities::Threading::TaskThreadPool* taskPool)
            : m_playerManager(playerManager),
            m_gameplayEngine(gameplayEngine),
            m_taskThreadPool(taskPool)
        {
            RF_NETWORK_INFO("MovementMessageHandler: Constructed.");
        }

        void MovementMessageHandler::Process(const RiftForged::GameLogic::Commands::GameCommand& command) {

            // 1. Safely get the specific MovementInput data from the command.
            const auto* moveData = std::get_if<RiftForged::GameLogic::Commands::MovementInput>(&command.data);
            if (!moveData) {
                RF_NETWORK_ERROR("MovementMessageHandler: Received wrong command type for PlayerID {}!", command.originatingPlayerID);
                return;
            }

            // 2. Get the player object using the PlayerID.
            auto* player = m_playerManager.FindPlayer(command.originatingPlayerID);
            if (!player) {
                RF_NETWORK_WARN("MovementMessageHandler: Null player pointer for PlayerID {}. Discarding.", command.originatingPlayerID);
                return;
            }

            RF_NETWORK_TRACE("Player {} sent MovementInput. LocalDir: ({:.2f},{:.2f},{:.2f}), Sprint: {}",
                player->playerId,
                moveData->localDirectionIntent.x,
                moveData->localDirectionIntent.y,
                moveData->localDirectionIntent.z,
                moveData->isSprinting);

            // 3. Call the GameplayEngine with the clean data.
            // The placeholder for delta time would be passed in by the main loop that calls this handler.
            const float placeholder_delta_time_sec = 1.0f / 60.0f;
            m_gameplayEngine.ProcessMovement(
                player,
                moveData->localDirectionIntent,
                moveData->isSprinting,
                placeholder_delta_time_sec
            );

            // Optional: The thread pool logic remains similar, but now captures the clean data struct.
            if (m_taskThreadPool) {
                auto taskData = *moveData;
                uint64_t playerId_copy = player->playerId;

                m_taskThreadPool->enqueue([playerId_copy, taskData]() {
                    // Perform async analytics or other non-critical tasks here
                    // using the clean 'taskData' struct.
                    });
            }
        }

    } // namespace Dispatch
} // namespace RiftForged