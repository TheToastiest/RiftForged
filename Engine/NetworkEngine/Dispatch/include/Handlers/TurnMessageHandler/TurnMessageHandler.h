#pragma once

#include <RiftForged/GameLogic/GameCommands/GameCommands.h>

// Forward declarations for GameLogic systems
namespace RiftForged {
    namespace GameLogic {
        class PlayerManager;
    }
    namespace Gameplay {
        class GameplayEngine;
    }
    namespace Utilities {
        namespace Threading {
            class TaskThreadPool;
        }
    }
    namespace Dispatch {

        class TurnMessageHandler {
        public:
            // Constructor is unchanged
            TurnMessageHandler(
                RiftForged::GameLogic::PlayerManager& playerManager,
                RiftForged::Gameplay::GameplayEngine& gameplayEngine,
                RiftForged::Utilities::Threading::TaskThreadPool* taskPool = nullptr
            );

            /**
             * @brief Processes a command for a player to turn.
             * @param command A generic command object containing TurnIntent data.
             */
            void Process(const RiftForged::GameLogic::Commands::GameCommand& command);

        private:
            RiftForged::GameLogic::PlayerManager& m_playerManager;
            RiftForged::Gameplay::GameplayEngine& m_gameplayEngine;
            RiftForged::Utilities::Threading::TaskThreadPool* m_taskThreadPool;
        };

    } // namespace Dispatch
} // namespace RiftForged