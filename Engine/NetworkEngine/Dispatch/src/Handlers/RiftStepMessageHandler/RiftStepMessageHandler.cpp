// File: NetworkEngine/UDP/C2S/RiftStepMessageHandler.cpp
#include <RiftForged/Dispatch/Handlers/RiftStepMessageHandler/RiftStepMessageHandler.h>

// Game Logic & Engine includes
#include <RiftForged/GameLogic/PlayerManager/PlayerManager.h>
#include <RiftForged/GameEngine/GameEngine/GameEngine.h>

// Utilities
#include <RiftForged/Utilities/Logger/Logger.h>

// Standard library for safe variant access
#include <variant>

namespace RiftForged {
    namespace Dispatch {

        // Constructor implementation is unchanged.
        RiftStepMessageHandler::RiftStepMessageHandler(
            RiftForged::GameLogic::PlayerManager& playerManager,
            RiftForged::Gameplay::GameplayEngine& gameplayEngine,
            RiftForged::Utilities::Threading::TaskThreadPool* taskPool)
            : m_playerManager(playerManager),
            m_gameplayEngine(gameplayEngine),
            m_taskThreadPool(taskPool)
        {
            RF_NETWORK_INFO("RiftStepMessageHandler: Constructed.");
        }

        void RiftStepMessageHandler::Process(const RiftForged::GameLogic::Commands::GameCommand& command) {

            // 1. Safely get the specific command data from the variant.
            const auto* riftStepData = std::get_if<RiftForged::GameLogic::Commands::RiftStepActivation>(&command.data);
            if (!riftStepData) {
                RF_NETWORK_ERROR("RiftStepMessageHandler: Received wrong command type for PlayerID {}!", command.originatingPlayerID);
                return;
            }

            // 2. Get the player object from the PlayerID.
            auto* player = m_playerManager.FindPlayer(command.originatingPlayerID);
            if (!player) {
                RF_NETWORK_WARN("RiftStepMessageHandler: Null player pointer for PlayerID {}. Discarding.", command.originatingPlayerID);
                return;
            }

            RF_NETWORK_DEBUG("RiftStepMessageHandler: Calling GameplayEngine for PlayerID: {} with intent: {}",
                player->playerId,
                static_cast<int>(riftStepData->directionalIntent));

            // 3. Execute the core game logic and get the outcome.
            // The handler's primary responsibility is now fulfilled.
            m_gameplayEngine.ExecuteRiftStep(player, riftStepData->directionalIntent);

            // THE HANDLER'S JOB IS DONE.
            //
            // The complex `PopulateFlatBufferEffectsFromOutcome` helper function and all
            // `FlatBufferBuilder` logic has been removed from this file.
            //
            // In Phase 2, the `GameplayEngine` will be responsible for taking the `RiftStepOutcome`
            // and publishing a "RiftStepExecuted" event. A new, separate "Response System"
            // will listen for that event and perform the complex task of building the
            // S2C_RiftStepInitiatedMsg. This cleanly separates the two responsibilities.
        }

    } // namespace Dispatch
} // namespace RiftForged