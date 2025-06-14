#include <RiftForged/Dispatch/Formatters/S2C_EntityStateUpdateFormatter.h>

// Include the event it listens for and the systems it uses
#include <RiftForged/Events/GameEventBus/GameEventBus.h>
#include <RiftForged/GameLogic/Events/EntityEvents/EntityEvents.h>
#include <RiftForged/Server/ServerEngine/ServerEngine.h>
#include <RiftForged/Network/INetworkIO/INetworkIO.h>

// This is now the ONLY place where S2C_EntityStateUpdateMsg is built
#include <RiftForged/Dispatch/GeneratedProtocols/V0.0.5/riftforged_s2c_udp_messages_generated.h>
#include <flatbuffers/flatbuffers.h>

namespace RiftForged {
    namespace Dispatch {
        namespace Formatters {

            S2C_EntityStateUpdateFormatter::S2C_EntityStateUpdateFormatter(
                Events::GameEventBus& eventBus,
                Networking::INetworkIO& networkEngine,
                Server::ServerEngine& serverEngine)
                : m_networkEngine(networkEngine), m_serverEngine(serverEngine)
                // Subscribe to the EntityStateUpdated event
            {
                eventBus.Subscribe<GameLogic::Events::EntityStateUpdated>(
                    [this](const std::any& ev) { this->OnEntityStateUpdated(ev); }
                );
            }

            void S2C_EntityStateUpdateFormatter::OnEntityStateUpdated(const std::any& eventData) {
                // Safely get the specific event data
                const auto& stateEvent = std::any_cast<const GameLogic::Events::EntityStateUpdated&>(eventData);

                // This is the logic moved from your old GameServerEngine::SimulationTick
                flatbuffers::FlatBufferBuilder builder(256);

                // Convert the pure GameLogic/Math types into FlatBuffer structs
                Networking::Shared::Vec3 pos_val(stateEvent.position.x, stateEvent.position.y, stateEvent.position.z);
                Networking::Shared::Quaternion orient_val(stateEvent.orientation.x, stateEvent.orientation.y, stateEvent.orientation.z, stateEvent.orientation.w);

                // In a full implementation, the event would also contain health, etc.
                // For now, we can use placeholder values.
                auto payload = Networking::UDP::S2C::CreateS2C_EntityStateUpdateMsg(builder,
                    stateEvent.entityId,
                    &pos_val,
                    &orient_val
                    // ... other fields like health, will, etc.
                );

                auto root_msg = Networking::UDP::S2C::CreateRoot_S2C_UDP_Message(builder,
                    Networking::UDP::S2C::S2C_UDP_Payload_EntityStateUpdate,
                    payload.Union()
                );

                builder.Finish(root_msg);

                // Get all connected clients to broadcast the update
                auto all_endpoints = m_serverEngine.GetAllActiveSessionEndpoints();

                // NOTE: A more advanced implementation could batch all updates from one tick
                // into a single packet. For now, this sends one packet per updated entity.
                for (const auto& endpoint : all_endpoints) {
                    // We would have a function to send the data without waiting for a response
                    m_networkEngine.SendData(endpoint, builder.GetBufferPointer(), builder.GetSize());
                }
            }

        } // namespace Formatters
    } // namespace Dispatch
} // namespace RiftForged