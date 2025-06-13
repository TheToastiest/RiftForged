#include <RiftForged/Dispatch/Handlers/JoinRequestMessageHandler/JoinRequestMessageHandler.h>
#include <RiftForged/Server/ServerEngine/ServerEngine.h>
#include <RiftForged/Utilities/Logger/Logger.h>

// S2C and Builder includes are still needed HERE for this special case handler.
#include <RiftForged/Dispatch/GeneratedProtocols/V0.0.5/riftforged_s2c_udp_messages_generated.h>
#include <flatbuffers/flatbuffers.h>
#include <variant>

namespace RiftForged {
    namespace Dispatch {

        JoinRequestMessageHandler::JoinRequestMessageHandler(RiftForged::Server::GameServerEngine& gameServerEngine)
            : m_gameServerEngine(gameServerEngine) {
            RF_NETWORK_INFO("JoinRequestMessageHandler: Initialized.");
        }

        std::optional<Networking::S2C_Response> JoinRequestMessageHandler::Process(
            const GameLogic::Commands::GameCommand& command,
            const Networking::NetworkEndpoint& senderEndpoint)
        {
            // Get the specific command data from the variant
            const auto* joinData = std::get_if<GameLogic::Commands::JoinRequest>(&command.data);
            if (!joinData) {
                RF_NETWORK_ERROR("JoinRequestMessageHandler: Received wrong command type!");
                // You could build a generic failure response here if needed
                return std::nullopt;
            }

            RF_NETWORK_INFO("Processing JoinRequest for character '{}' from endpoint {}.",
                joinData->characterIdToLoad, senderEndpoint.ToString());

            // Delegate the actual join process to the GameServerEngine.
            uint64_t newPlayerId = m_gameServerEngine.OnClientAuthenticatedAndJoining(
                senderEndpoint, joinData->characterIdToLoad
            );

            // Build the appropriate S2C response based on the outcome.
            flatbuffers::FlatBufferBuilder builder(256);
            Networking::S2C_Response response;
            response.specific_recipient = senderEndpoint;
            response.broadcast = false;

            if (newPlayerId != 0) { // Success
                auto welcome_msg = builder.CreateString("Welcome to RiftForged!");
                auto payload = Networking::UDP::S2C::CreateS2C_JoinSuccessMsg(builder, newPlayerId, welcome_msg, m_gameServerEngine.GetServerTickRateHz());
                auto root_msg = Networking::UDP::S2C::CreateRoot_S2C_UDP_Message(builder, Networking::UDP::S2C::S2C_UDP_Payload_S2C_JoinSuccessMsg, payload.Union());
                builder.Finish(root_msg);
                response.flatbuffer_payload_type = Networking::UDP::S2C::S2C_UDP_Payload_S2C_JoinSuccessMsg;
            }
            else { // Failure
                auto reason_msg = builder.CreateString("Server failed to process join request.");
                auto payload = Networking::UDP::S2C::CreateS2C_JoinFailedMsg(builder, reason_msg, 2);
                auto root_msg = Networking::UDP::S2C::CreateRoot_S2C_UDP_Message(builder, Networking::UDP::S2C::S2C_UDP_Payload_S2C_JoinFailedMsg, payload.Union());
                builder.Finish(root_msg);
                response.flatbuffer_payload_type = Networking::UDP::S2C::S2C_UDP_Payload_S2C_JoinFailedMsg;
            }

            response.data = builder.Release();
            return response;
        }

    } // namespace Dispatch
} // namespace RiftForged