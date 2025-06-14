// File: Dispatch/PacketProcessor.h (Refactored)
#pragma once

#include <cstdint>

// Forward declarations
namespace RiftForged {
    // We will need our new clean command structure
    namespace GameLogic { namespace Commands { struct GameCommand; } }

    namespace Networking {
        class MessageDispatcher;
        class NetworkEndpoint;
    }
    namespace Server {
        class GameServerEngine;
    }

    namespace Dispatch {

        // The PacketProcessor is no longer a generic "IMessageHandler",
        // it has a very specific role, so we can make it a concrete class.
        class PacketProcessor {
        public:
            PacketProcessor(MessageDispatcher& dispatcher,
                RiftForged::Server::GameServerEngine& gameServerEngine);

            /**
             * @brief Processes a raw network datagram.
             * This is the entry point from the network layer. This function's sole
             * responsibility is to translate the raw bytes into a clean GameCommand
             * and pass it to the MessageDispatcher.
             */
            void ProcessIncomingPacket(
                const Networking::NetworkEndpoint& sender_endpoint,
                const uint8_t* data,
                uint16_t size
            );

        private:
            MessageDispatcher& m_messageDispatcher;
            RiftForged::Server::GameServerEngine& m_gameServerEngine; // Needed to access the PlayerManager
        };

    } // namespace Dispatch
} // namespace RiftForged