#pragma once

// Forward declarations for its dependencies
namespace RiftForged {
    namespace Events { class GameEventBus; }
    namespace Networking { class INetworkIO; }
    namespace Server { class ServerEngine; }
}

namespace RiftForged {
    namespace Dispatch {
        namespace Formatters {

            /**
             * @brief Listens for entity state changes and formats them into
             * S2C_EntityStateUpdateMsg messages for network broadcast. This
             * handles all continuous movement updates.
             */
            class S2C_EntityStateUpdateFormatter {
            public:
                // Constructor subscribes this formatter to the event bus
                S2C_EntityStateUpdateFormatter(
                    Events::GameEventBus& eventBus,
                    Networking::INetworkIO& networkEngine,
                    Server::ServerEngine& serverEngine
                );

            private:
                // The function that will be called by the event bus
                void OnEntityStateUpdated(const std::any& eventData);

                // Systems needed to send the message
                Networking::INetworkIO& m_networkEngine;
                Server::ServerEngine& m_serverEngine;
            };

        } // namespace Formatters
    } // namespace Dispatch
} // namespace RiftForged