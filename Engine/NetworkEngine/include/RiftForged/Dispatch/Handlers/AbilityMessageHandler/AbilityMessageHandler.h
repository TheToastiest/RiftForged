#pragma once

// The ONLY dependency we need for the command contract is our new blueprint.
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
    // We create a new, clear namespace for our handlers
    namespace Dispatch {

        class AbilityMessageHandler {
        public:
            // Constructor doesn't need to change. It still needs access to GameLogic systems.
            AbilityMessageHandler(
                RiftForged::GameLogic::PlayerManager& playerManager,
                RiftForged::Gameplay::GameplayEngine& gameplayEngine,
                RiftForged::Utilities::Threading::TaskThreadPool* taskPool = nullptr
            );

            /**
             * @brief Processes a command to use an ability.
             * @param command A generic command object that we know contains UseAbility data.
             *
             * This signature is NOW DECOUPLED. It has no Network or FlatBuffer types.
             * It only depends on the clean GameCommand contract.
             */
            void Process(const RiftForged::GameLogic::Commands::GameCommand& command);

        private:
            RiftForged::GameLogic::PlayerManager& m_playerManager;
            RiftForged::Gameplay::GameplayEngine& m_gameplayEngine;
            RiftForged::Utilities::Threading::TaskThreadPool* m_taskThreadPool;
        };

    } // namespace Dispatch
} // namespace RiftForged