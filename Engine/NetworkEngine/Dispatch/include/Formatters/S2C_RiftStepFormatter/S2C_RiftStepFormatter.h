#pragma once

// Forward declarations
namespace RiftForged {
    namespace Events { class GameEventBus; }
    namespace GameLogic { class PlayerManager; }
    namespace Networking { class INetworkIO; }
    namespace Server { class ServerEngine; } // To get global info if needed
}

namespace RiftForged {
    namespace Dispatch {
        namespace Formatters {

            class S2C_RiftStepFormatter {
            public:
                // The constructor subscribes to the event bus.
                S2C_RiftStepFormatter(
                    Events::GameEventBus& eventBus,
                    GameLogic::PlayerManager& playerManager,
                    Networking::INetworkIO& networkEngine,
                    Server::ServerEngine& serverEngine
                );

            private:
                // This is the function that will be called by the event bus
                void OnRiftStepExecuted(const std::any& eventData);

                // --- Member References to other systems ---
                GameLogic::PlayerManager& m_playerManager;
                Networking::INetworkIO& m_networkEngine;
                Server::ServerEngine& m_serverEngine;
            };

        } // namespace Formatters
    } // namespace Dispatch
} // namespace RiftForged