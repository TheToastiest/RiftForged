#pragma once

#include <RiftForged/GameLogic/GameCommands/GameCommands.h>
#include <RiftForged/Network/NetworkCommon/NetworkCommon.h> // For S2C_Response
#include <optional>

// Forward declarations
namespace RiftForged {
    namespace Server { class GameServerEngine; }
}

namespace RiftForged {
    namespace Dispatch {

        class JoinRequestMessageHandler {
        public:
            explicit JoinRequestMessageHandler(RiftForged::Server::GameServerEngine& gameServerEngine);

            /**
             * @brief Processes a join request command.
             * @param command The GameCommand containing the JoinRequest data.
             * @param senderEndpoint The network endpoint of the original sender.
             * @return An optional S2C_Response to be sent directly back to the client.
             */
            std::optional<Networking::S2C_Response> Process(
                const GameLogic::Commands::GameCommand& command,
                const Networking::NetworkEndpoint& senderEndpoint
            );

        private:
            RiftForged::Server::GameServerEngine& m_gameServerEngine;
        };

    } // namespace Dispatch
} // namespace RiftForged