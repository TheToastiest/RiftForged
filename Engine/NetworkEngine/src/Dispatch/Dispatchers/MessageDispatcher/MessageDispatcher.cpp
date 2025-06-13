#include <RiftForged/Dispatch/Dispatchers/MessageDispatcher.h> // Include the dispatcher header
#include <RiftForged/Dispatch/Handlers/IMessageHandler.h> // Include the interface definition
#include <RiftForged/Utilities/Logger/Logger.h>

namespace RiftForged {
    namespace Dispatch {

        void MessageDispatcher::DispatchGameCommand(const GameLogic::Commands::GameCommand& command) {
            // Use std::visit to safely and automatically call the code
            // for the type currently stored in the command.data variant.
            std::visit([&](const auto& specificCommandData) {

                const auto type_idx = std::type_index(typeid(specificCommandData));

                // Look for a handler registered for this specific type.
                auto it = m_handlers.find(type_idx);
                if (it != m_handlers.end()) {
                    // We found a handler, so we call its Process method.
                    it->second->Process(command);
                }
                else {
                    // This is a configuration error. We need to log it.
                    RF_NETWORK_WARN("MessageDispatcher: No handler registered for command type '{}'.", type_idx.name());
                }

                }, command.data);
        }

    } // namespace Dispatch
} // namespace RiftForged