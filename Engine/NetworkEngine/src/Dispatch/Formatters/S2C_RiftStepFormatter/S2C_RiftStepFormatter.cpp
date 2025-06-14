#include <RiftForged/Dispatch/Formatters/S2C_RiftStepFormatter/S2C_RiftStepFormatter.h>

// System includes
#include <RiftForged/Events/GameEventBus/GameEventBus.h>
#include <RiftForged/GameLogic/Events/MovementEvents/MovementEvents.h> // The event we listen for
#include <RiftForged/Server/ServerEngine/ServerEngine.h>
#include <RiftForged/Network/INetworkIO/INetworkIO.h>

// This class is now the ONLY place this S2C message is built.
#include <RiftForged/Dispatch/GeneratedProtocols/V0.0.5/riftforged_s2c_udp_messages_generated.h>
#include <flatbuffers/flatbuffers.h>

namespace RiftForged {
    namespace Dispatch {
        namespace Formatters {

            S2C_RiftStepFormatter::S2C_RiftStepFormatter(
                Events::GameEventBus& eventBus,
                GameLogic::PlayerManager& playerManager,
                Networking::INetworkIO& networkEngine,
                Server::ServerEngine& serverEngine)
                : m_playerManager(playerManager),
                m_networkEngine(networkEngine),
                m_serverEngine(serverEngine)
            {
                // Subscribe to the event bus when this object is created.
                eventBus.Subscribe<GameLogic::Events::RiftStepExecuted>(
                    [this](const std::any& eventData) { this->OnRiftStepExecuted(eventData); }
                );
            }

            void S2C_RiftStepFormatter::OnRiftStepExecuted(const std::any& eventData) {
                // Safely cast the generic event data to our specific event type.
                const auto& riftStepEvent = std::any_cast<const GameLogic::Events::RiftStepExecuted&>(eventData);

                // --- ALL THE LOGIC FROM THE OLD HANDLER LIVES HERE ---
                // This is the code we cut from RiftStepMessageHandler.cpp
                flatbuffers::FlatBufferBuilder builder(1024);

                // ... logic to populate effects, Vfx Ids, etc., from the riftStepEvent data ...
                // auto start_vfx_fb_str = builder.CreateString(riftStepEvent.startVfxId);

                // ... use a helper or local code to build the effect vectors ...

                auto s2c_payload_offset = Networking::UDP::S2C::CreateS2C_RiftStepInitiatedMsg(...);
                auto root_msg = Networking::UDP::S2C::CreateRoot_S2C_UDP_Message(builder, ..., s2c_payload_offset.Union());
                builder.Finish(root_msg);

                // Get all players who should receive this message
                auto recipients = m_playerManager.GetAllPlayersInVicinity(riftStepEvent.instigatorEntityId);
                // or just get all players for a broadcast
                // auto endpoints = m_serverEngine.GetAllActiveSessionEndpoints();

                // Send the data via the network engine
                m_networkEngine.SendData(...); // This would take recipients and the builder data
            }

        } // namespace Formatters
    } // namespace Dispatch
} // namespace RiftForged