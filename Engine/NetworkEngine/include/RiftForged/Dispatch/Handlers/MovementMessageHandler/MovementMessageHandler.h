#pragma once

#include <RiftForged/GameLogic/GameCommands/GameCommands.h>

// Forward declarations for the systems this handler will TALK TO.
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

        class MovementMessageHandler {
        public:
            // Constructor remains the same.
            MovementMessageHandler(
                RiftForged::GameLogic::PlayerManager& playerManager,
                RiftForged::Gameplay::GameplayEngine& gameplayEngine,
                RiftForged::Utilities::Threading::TaskThreadPool* taskPool
            );

            /**
             * @brief Processes a command for player movement input.
             * @param command A generic command object containing MovementInput data.
             */
            void Process(const RiftForged::GameLogic::Commands::GameCommand& command);

        private:
            RiftForged::GameLogic::PlayerManager& m_playerManager;
            RiftForged::Gameplay::GameplayEngine& m_gameplayEngine;
            RiftForged::Utilities::Threading::TaskThreadPool* m_taskThreadPool;
        };

    } // namespace Dispatch
} // namespace RiftForged