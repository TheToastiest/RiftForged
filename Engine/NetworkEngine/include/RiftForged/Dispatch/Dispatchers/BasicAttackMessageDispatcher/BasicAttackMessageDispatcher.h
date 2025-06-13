#pragma once
#include <RiftForged/GameLogic/GameCommands/GameCommands.h>

// Forward declarations...
namespace RiftForged {
    namespace GameLogic { class PlayerManager; }
    namespace Gameplay { class GameplayEngine; }
    // ...

    namespace Dispatch {

        class BasicAttackMessageHandler {
        public:
            // Constructor is the same
            BasicAttackMessageHandler(...);

            // The Process method now takes a GameCommand and returns VOID.
            // Its only job is to start the action, not create the response.
            void Process(const RiftForged::GameLogic::Commands::GameCommand& command);

        private:
            RiftForged::GameLogic::PlayerManager& m_playerManager;
            RiftForged::Gameplay::GameplayEngine& m_gameplayEngine;
            RiftForged::Utilities::Threading::TaskThreadPool* m_taskThreadPool;
        };

    } // namespace Dispatch
} // namespace RiftForged