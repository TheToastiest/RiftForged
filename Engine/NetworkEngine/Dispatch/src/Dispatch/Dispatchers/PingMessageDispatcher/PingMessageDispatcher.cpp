// File: NetworkEngine/UDP/C2S/PingMessageHandler.cpp
#include <RiftForged/Dispatch/Handlers/PingMessageHandler/PingMessageHandler.h>
#include <RiftForged/Utilities/Logger/Logger.h>

// S2C and Builder includes are still needed for this handler.
#include <RiftForged/Dispatch/GeneratedProtocols/V0.0.5/riftforged_s2c_udp_messages_generated.h>
#include <flatbuffers/flatbuffers.h>
#include <variant>
#include <chrono>

namespace RiftForged {
    namespace Dispatch {

        PingMessageHandler::PingMessageHandler(RiftForged::Utilities::Threading::TaskThreadPool* taskPool)
            : m_taskThreadPool(taskPool)
        {
            RF_NETWORK_INFO("PingMessageHandler: Constructed.");
        }

        std::optional<Networking::S2C_Response> PingMessageHandler::Process(
            const GameLogic::Commands::GameCommand& command,
            const Networking::NetworkEndpoint& senderEndpoint)
        {
            // 1. Safely get the specific Ping data from the command.
            const auto* pingData = std::get_if<GameLogic::Commands::Ping>(&command.data);
            if (!pingData) {
                RF_NETWORK_ERROR("PingMessageHandler: Received wrong command type for PlayerID {}!", command.originatingPlayerID);
                return std::nullopt;
            }

            RF_NETWORK_INFO("PingMessageHandler: Received Ping from PlayerID {}. Client Timestamp: {}.",
                command.originatingPlayerID, pingData->clientTimestampMs);

            // 2. Build the S2C_PongMsg response directly. This is this handler's main job.
            flatbuffers::FlatBufferBuilder builder(256);
            uint64_t server_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

            auto pong_payload = Networking::UDP::S2C::CreateS2C_PongMsg(
                builder,
                pingData->clientTimestampMs, // Use the timestamp from our clean command struct
                server_timestamp
            );

            auto root_msg = Networking::UDP::S2C::CreateRoot_S2C_UDP_Message(
                builder,
                Networking::UDP::S2C::S2C_UDP_Payload_Pong,
                pong_payload.Union()
            );
            builder.Finish(root_msg);

            // 3. Prepare the S2C_Response struct to be returned to the network layer.
            Networking::S2C_Response response;
            response.data = builder.Release();
            response.flatbuffer_payload_type = Networking::UDP::S2C::S2C_UDP_Payload_Pong;
            response.broadcast = false;
            response.specific_recipient = senderEndpoint;

            return response;
        }

    } // namespace Dispatch
} // namespace RiftForged