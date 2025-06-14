// File: TurnMessageHandler.cpp
#include <RiftForged/Dispatch/Handlers/TurnMessageHandler/TurnMessageHandler.h>

// Game Logic & Engine includes
#include <RiftForged/GameLogic/PlayerManager/PlayerManager.h>
#include <RiftForged/GameEngine/GameEngine/GameEngine.h>
#include <RiftForged/Utilities/Logger/Logger.h>
#include <variant>

namespace RiftForged {
    namespace Dispatch {

        // Constructor implementation is unchanged.
        TurnMessageHandler::TurnMessageHandler(
            RiftForged::GameLogic::PlayerManager& playerManager,
            RiftForged::Gameplay::GameplayEngine& gameplayEngine,
            RiftForged::Utilities::Threading::TaskThreadPool* taskPool)
            : m_playerManager(playerManager),
            m_gameplayEngine(gameplayEngine),
            m_taskThreadPool(taskPool)
        {
            RF_NETWORK_INFO("TurnMessageHandler: Constructed.");
        }

        void TurnMessageHandler::Process(const RiftForged::GameLogic::Commands::GameCommand& command) {

            // 1. Safely get the specific TurnIntent data from the command.
            const auto* turnData = std::get_if<RiftForged::GameLogic::Commands::TurnIntent>(&command.data);
            if (!turnData) {
                RF_NETWORK_ERROR("TurnMessageHandler: Received wrong command type for PlayerID {}!", command.originatingPlayerID);
                return;
            }

            // 2. Get the player object using the PlayerID.
            auto* player = m_playerManager.FindPlayer(command.originatingPlayerID);
            if (!player) {
                RF_NETWORK_WARN("TurnMessageHandler: Null player pointer for PlayerID {}. Discarding.", command.originatingPlayerID);
                return;
            }

            RF_NETWORK_TRACE("Player {} sent TurnIntent: {:.2f} degrees.",
                player->playerId, turnData->turnDeltaDegrees);

            // 3. Call the GameplayEngine with the clean data.
            m_gameplayEngine.TurnPlayer(player, turnData->turnDeltaDegrees);

            // The handler's job is done. It's fire-and-forget.
        }

    } // namespace Dispatch
} // namespace RiftForged