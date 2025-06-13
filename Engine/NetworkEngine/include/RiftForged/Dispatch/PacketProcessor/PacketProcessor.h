#include <RiftForged/Dispatch/PacketProcessor/PacketProcessor.h>

// System-level includes
#include <RiftForged/Dispatch/Dispatchers/MessageDispatcher.h> // The new "smart router"
#include <RiftForged/Server/ServerEngine/ServerEngine.h>
#include <RiftForged/GameLogic/PlayerManager/PlayerManager.h>
#include <RiftForged/Utilities/Logger/Logger.h>
#include <RiftForged/Utilities/MathUtils/MathUtils.h> // For your math types

// This class is the ONLY place in the dispatch pipeline that includes the raw C2S message definitions.
#include <RiftForged/Dispatch/GeneratedProtocols/V0.0.5/riftforged_c2s_udp_messages_generated.h>
#include <RiftForged/Dispatch/GeneratedProtocols/V0.0.5/riftforged_common_types_generated.h>

// We include the clean command definitions that we will be translating INTO.
#include <RiftForged/GameLogic/GameCommands/GameCommands.h>

namespace RiftForged {
    namespace Dispatch {

        PacketProcessor::PacketProcessor(MessageDispatcher& dispatcher, RiftForged::Server::GameServerEngine& gameServerEngine)
            : m_messageDispatcher(dispatcher),
            m_gameServerEngine(gameServerEngine)
        {
            RF_NETWORK_INFO("PacketProcessor: Constructed.");
        }

        void PacketProcessor::ProcessIncomingPacket(
            const Networking::NetworkEndpoint& sender_endpoint,
            const uint8_t* data,
            uint16_t size)
        {
            // --- Step 1: Verify the FlatBuffer ---
            // This logic is the same as your old MessageDispatcher, ensuring data integrity.
            flatbuffers::Verifier verifier(data, size);
            if (!Networking::UDP::C2S::VerifyRoot_C2S_UDP_MessageBuffer(verifier)) {
                RF_NETWORK_WARN("PacketProcessor: Packet from {} failed FlatBuffer verification.", sender_endpoint.ToString());
                return;
            }
            const auto* root_message = Networking::UDP::C2S::GetRoot_C2S_UDP_Message(data);
            if (!root_message || !root_message->payload()) {
                RF_NETWORK_WARN("PacketProcessor: Packet from {} has no payload.", sender_endpoint.ToString());
                return;
            }

            // --- Step 2: Identify the Player ---
            // We get the PlayerID from the PlayerManager. This is a critical session management step.
            auto& playerManager = m_gameServerEngine.GetPlayerManager();
            std::optional<GameLogic::Commands::PlayerID> maybePlayerID;

            const auto payload_type = root_message->payload_type();

            // JoinRequest is special: the player does not have an ID yet.
            // We will assign a temporary or invalid ID for the command, but the handler will know what to do.
            if (payload_type == Networking::UDP::C2S::C2S_UDP_Payload_JoinRequest) {
                maybePlayerID = 0; // Use 0 or another invalid ID to signify a new connection.
            }
            else {
                maybePlayerID = playerManager.FindPlayerID(sender_endpoint);
            }

            if (!maybePlayerID.has_value()) {
                RF_NETWORK_WARN("PacketProcessor: Dropping packet from unassociated endpoint {}.", sender_endpoint.ToString());
                return;
            }

            // --- Step 3: Create and Translate the Command ---
            GameLogic::Commands::GameCommand command_to_dispatch;
            command_to_dispatch.originatingPlayerID = *maybePlayerID;

            // This switch translates the specific FlatBuffer message into our clean command struct.
            switch (payload_type) {

            case Networking::UDP::C2S::C2S_UDP_Payload_UseAbility: {
                const auto* msg = root_message->payload_as_UseAbility();
                if (!msg) break;

                GameLogic::Commands::UseAbility cmdData;
                cmdData.clientTimestampMs = msg->client_timestamp_ms();
                cmdData.abilityId = msg->ability_id();
                cmdData.targetEntityId = msg->target_entity_id();
                if (msg->target_position()) {
                    cmdData.targetPosition = { msg->target_position()->x(), msg->target_position()->y(), msg->target_position()->z() };
                }
                command_to_dispatch.data.emplace<GameLogic::Commands::UseAbility>(cmdData);
                break;
            }

            case Networking::UDP::C2S::C2S_UDP_Payload_MovementInput: {
                const auto* msg = root_message->payload_as_MovementInput();
                if (!msg || !msg->local_direction_intent()) break;

                GameLogic::Commands::MovementInput cmdData;
                cmdData.clientTimestampMs = msg->client_timestamp_ms();
                cmdData.isSprinting = msg->is_sprinting();
                cmdData.localDirectionIntent = { msg->local_direction_intent()->x(), msg->local_direction_intent()->y(), msg->local_direction_intent()->z() };
                command_to_dispatch.data.emplace<GameLogic::Commands::MovementInput>(cmdData);
                break;
            }

                                                                    // ... cases for TurnIntent, BasicAttack, RiftStep, Ping, JoinRequest would follow the same pattern ...

            default: {
                RF_NETWORK_WARN("PacketProcessor: Received unhandled message type from player {}.", command_to_dispatch.originatingPlayerID);
                return; // Do not dispatch if we don't know the type.
            }
            }

            // --- Step 4: Dispatch the Clean Command ---
            // The PacketProcessor's job is done. It hands the standardized GameCommand
            // to the router and has no idea what will happen next.
            m_messageDispatcher.DispatchGameCommand(command_to_dispatch);
        }

    } // namespace Dispatch
} // namespace RiftForged