#pragma once

#include <RiftForged/GameLogic/GameCommands/GameCommands.h>
#include <RiftForged/Network/NetworkCommon/NetworkCommon.h> // For S2C_Response
#include <optional>

// Forward declarations
namespace RiftForged {
    namespace Networking { class NetworkEndpoint; }
    namespace GameLogic { class PlayerManager; }
    namespace Utilities { namespace Threading { class TaskThreadPool; } }

    namespace Dispatch {

        class PingMessageHandler {
        public:
            // The constructor no longer needs the PlayerManager if it's only used for logging.
            // If it's needed for anything else, it can be kept. For now, we'll remove it for simplicity.
            explicit PingMessageHandler(RiftForged::Utilities::Threading::TaskThreadPool* taskPool = nullptr);

            /**
             * @brief Processes a ping command and generates a pong response.
             * @param command The GameCommand containing the Ping data.
             * @param senderEndpoint The network endpoint of the original sender for the response.
             * @return An optional S2C_Response containing the S2C_PongMsg.
             */
            std::optional<Networking::S2C_Response> Process(
                const GameLogic::Commands::GameCommand& command,
                const Networking::NetworkEndpoint& senderEndpoint
            );

        private:
            RiftForged::Utilities::Threading::TaskThreadPool* m_taskThreadPool;
        };

    } // namespace Dispatch
} // namespace RiftForged