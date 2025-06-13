// File: Dispatch/PacketProcessor.cpp (Refactored)

#include <RiftForged/Dispatch/PacketProcessor/PacketProcessor.h>
#include <RiftForged/Dispatch/MessageDispatcher/MessageDispatcher.h>
#include <RiftForged/Server/ServerEngine/ServerEngine.h>
#include <RiftForged/GameLogic/PlayerManager/PlayerManager.h>
#include <RiftForged/Utilities/Logger/Logger.h>

// This class is now the ONLY place in the dispatch pipeline that includes the raw message definitions.
#include <RiftForged/Dispatch/GeneratedProtocols/V0.0.5/riftforged_c2s_udp_messages_generated.h>
#include <RiftForged/Dispatch/GeneratedProtocols/V0.0.5/riftforged_common_types_generated.h>

//#include "../FlatBuffers/Versioning/V0.0.5/riftforged_c2s_udp_messages_generated.h"

// We now include the clean command definitions to translate INTO.
#include <RiftForged/GameLogic/GameCommands/GameCommands.h>


namespace RiftForged {
    namespace Dispatch {

        PacketProcessor::PacketProcessor(MessageDispatcher& dispatcher, RiftForged::Server::GameServerEngine& gameServerEngine)
            : m_messageDispatcher(dispatcher),
            m_gameServerEngine(gameServerEngine)
        {
            RF_NETWORK_INFO("PacketProcessor: Initialized.");
        }

        void PacketProcessor::ProcessIncomingPacket(
            const Networking::NetworkEndpoint& sender_endpoint,
            const uint8_t* data,
            uint16_t size)
        {
            // --- 1. Verify and unpack the root FlatBuffer message ---
            flatbuffers::Verifier verifier(data, size);
            if (!Networking::UDP::C2S::VerifyRoot_C2S_UDP_MessageBuffer(verifier)) {
                RF_NETWORK_WARN("PacketProcessor: Incoming packet from {} failed verification. Discarding.", sender_endpoint.ToString());
                return;
            }
            const auto* root_message = Networking::UDP::C2S::GetRoot_C2S_UDP_Message(data);
            if (!root_message || !root_message->payload()) {
                RF_NETWORK_WARN("PacketProcessor: Packet from {} has no payload. Discarding.", sender_endpoint.ToString());
                return;
            }
            const auto payload_type = root_message->payload_type();

            // --- 2. Get the PlayerID for the sender ---
            // The PlayerManager is now our source for identity, not the NetworkEndpoint itself.
            auto& playerManager = m_gameServerEngine.GetPlayerManager();
            auto maybePlayerID = playerManager.FindPlayerID(sender_endpoint);

            // --- 3. Handle JoinRequest (the only message an unknown player can send) ---
            if (payload_type == Networking::UDP::C2S::C2S_UDP_Payload_JoinRequest) {
                // ... (The logic to handle a join request would go here, as it's a special case)
                // For now, we'll focus on commands from existing players.
                return;
            }

            // If the message is NOT a join request, the player MUST exist.
            if (!maybePlayerID.has_value()) {
                RF_NETWORK_WARN("PacketProcessor: Dropping packet from unassociated endpoint {}.", sender_endpoint.ToString());
                return;
            }

            GameLogic::Commands::GameCommand command_to_dispatch;
            command_to_dispatch.originatingPlayerID = *maybePlayerID;

            // --- 4. THE TRANSLATION SWITCH ---
            // This is the core responsibility of this class.
            // It translates the FlatBuffer payload into a clean GameCommand.
            switch (payload_type) {

            case Networking::UDP::C2S::C2S_UDP_Payload_UseAbility: {
                const auto* msg = root_message->payload_as_UseAbility();
                if (!msg) break; // Malformed packet

                // Create our clean data struct
                GameLogic::Commands::UseAbility cmdData;
                cmdData.clientTimestampMs = msg->client_timestamp_ms();
                cmdData.abilityId = msg->ability_id();
                cmdData.targetEntityId = msg->target_entity_id();
                if (msg->target_position()) {
                    // Note the translation from a Networking::Vec3 to a Math::Vec3
                    cmdData.targetPosition = { msg->target_position()->x(), msg->target_position()->y(), msg->target_position()->z() };
                }

                // Put the specific data into our command's "smart box" (the variant)
                command_to_dispatch.data.emplace<GameLogic::Commands::UseAbility>(cmdData);
                break;
            }

            case Networking::UDP::C2S::C2S_UDP_Payload_MovementInput: {
                // TODO: Translate C2S_MovementInputMsg into GameLogic::Commands::MovementInput
                break;
            }

            case Networking::UDP::C2S::C2S_UDP_Payload_TurnIntent: {
                // TODO: Translate C2S_TurnIntentMsg into GameLogic::Commands::TurnIntent
                break;
            }

                                                                 // ... cases for all other message types ...

            default: {
                RF_NETWORK_WARN("PacketProcessor: Received unhandled message type from player {}.", command_to_dispatch.originatingPlayerID);
                return; // Don't dispatch anything
            }
            }

            // --- 5. Dispatch the Clean Command ---
            // We pass the clean, fully translated GameCommand to the dispatcher.
            // The dispatcher and handlers below this point have NO KNOWLEDGE of FlatBuffers.
            m_messageDispatcher.DispatchGameCommand(command_to_dispatch);
        }

    } // namespace Dispatch
} // namespace RiftForged