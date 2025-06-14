#include <RiftForged/Dispatch/Handlers/AbilityMessageHandler/AbilityMessageHandler.h>

// We include the full definitions for the systems we use
#include <RiftForged/GameLogic/PlayerManager/PlayerManager.h>
#include <RiftForged/Gameplay/GameplayEngine.h>
#include <RiftForged/Utilities/Logger/Logger.h>
#include <variant> // Needed for std::get

namespace RiftForged {
    namespace Dispatch {

        // Constructor implementation is unchanged
        AbilityMessageHandler::AbilityMessageHandler(
            RiftForged::GameLogic::PlayerManager& playerManager,
            RiftForged::Gameplay::GameplayEngine& gameplayEngine,
            RiftForged::Utilities::Threading::TaskThreadPool* taskPool)
            : m_playerManager(playerManager),
            m_gameplayEngine(gameplayEngine),
            m_taskThreadPool(taskPool)
        {
            RF_NETWORK_INFO("AbilityMessageHandler: Constructed.");
        }

        void AbilityMessageHandler::Process(const RiftForged::GameLogic::Commands::GameCommand& command) {

            // Step 1: Safely get the specific command data from the "smart box" (the variant).
            // If the command was for Movement instead of UseAbility, this line would
            // safely fail instead of causing weird bugs.
            const auto* abilityData = std::get_if<RiftForged::GameLogic::Commands::UseAbility>(&command.data);
            if (!abilityData) {
                RF_NETWORK_ERROR("AbilityMessageHandler: Received wrong command type!");
                return;
            }

            // Step 2: Get the player using the clean 'PlayerID', not the NetworkEndpoint.
            auto* player = m_playerManager.FindPlayer(command.originatingPlayerID);
            if (!player) {
                RF_NETWORK_WARN("AbilityMessageHandler: Player not found for ID {}. Cannot process ability.",
                    command.originatingPlayerID);
                return;
            }

            RF_NETWORK_INFO("AbilityMessageHandler: Player {} using ability {}.",
                player->playerId, abilityData->abilityId);

            // Step 3: Use the clean data to call GameLogic.
            // The GameplayEngine doesn't know where this data came from (network, console command, AI, etc.)
            // which makes it pure and easy to test.
            //
            // bool canUse = m_gameplayEngine.CanPlayerUseAbility(player, abilityData->abilityId);
            // ...
            // m_gameplayEngine.ExecutePlayerAbility(player, abilityData->abilityId, abilityData->targetEntityId, abilityData->targetPosition);

            // --- Example of Thread Pool Usage with the new, clean data ---
            if (m_taskThreadPool) {
                // We capture the clean struct by value. This is much safer and simpler
                // than capturing individual fields from a FlatBuffer pointer.
                auto taskData = *abilityData;
                uint64_t playerId_copy = player->playerId;

                m_taskThreadPool->enqueue([playerId_copy, taskData]() {
                    RF_NETWORK_DEBUG("AbilityMessageHandler (ThreadPool): Async processing for Player {} using Ability {}.",
                        playerId_copy, taskData.abilityId);

                    // Any async logic uses the clean 'taskData' struct here.
                    });
            }

            // The handler is no longer responsible for creating network responses directly.
            // Its job is done once it has passed the command to the GameLogic.
        }

    } // namespace Dispatch
} // namespace RiftForged