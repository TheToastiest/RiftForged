#include <RiftForged/Dispatch/Formatters/S2C_CombatEventFormatter/S2C_CombatEventFormatter.h>

// Required includes for this formatter's job
#include <RiftForged/Events/GameEventBus/GameEventBus.h>
#include <RiftForged/GameLogic/Events/CombatEvents/CombatEvents.h>
#include <RiftForged/Server/ServerEngine/ServerEngine.h>
#include <RiftForged/Network/INetworkIO/INetworkIO.h>

// This class is now the ONLY place that builds these specific S2C messages
#include <RiftForged/Dispatch/GeneratedProtocols/V0.0.5/riftforged_s2c_udp_messages_generated.h>
#include <flatbuffers/flatbuffers.h>

namespace RiftForged {
    namespace Dispatch {
        namespace Formatters {

            S2C_CombatEventFormatter::S2C_CombatEventFormatter(
                Events::GameEventBus& eventBus,
                Networking::INetworkIO& networkEngine,
                Server::ServerEngine& serverEngine)
                : m_networkEngine(networkEngine), m_serverEngine(serverEngine)
            {
                // Subscribe to all the events this formatter is responsible for
                eventBus.Subscribe<GameLogic::Events::EntityDealtDamage>(
                    [this](const auto& ev) { this->OnEntityDealtDamage(ev); }
                );
                eventBus.Subscribe<GameLogic::Events::PlayerAbilityFailed>(
                    [this](const auto& ev) { this->OnPlayerAbilityFailed(ev); }
                );
                // ... subscribe to other failure events
            }

            void S2C_CombatEventFormatter::OnEntityDealtDamage(const std::any& eventData) {
                const auto& damageEvent = std::any_cast<const GameLogic::Events::EntityDealtDamage&>(eventData);

                // --- FlatBufferBuilder logic to create S2C_CombatEventMsg lives here ---
                // This is the logic we CUT from the old handlers.
                // It translates the pure 'damageEvent' into a network message.
            }

            void S2C_CombatEventFormatter::OnPlayerAbilityFailed(const std::any& eventData) {
                const auto& failureEvent = std::any_cast<const GameLogic::Events::PlayerAbilityFailed&>(eventData);

                // --- FlatBufferBuilder logic to create S2C_AbilityFailedMsg lives here ---
                // It gets the player's endpoint from the ServerEngine and sends the message.
            }

        } // namespace Formatters
    } // namespace Dispatch
} // namespace RiftForged