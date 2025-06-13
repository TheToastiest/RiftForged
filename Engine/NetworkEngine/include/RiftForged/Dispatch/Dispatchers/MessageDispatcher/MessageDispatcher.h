#pragma once

#include <RiftForged/GameLogic/GameCommands/GameCommands.h>
#include <functional>
#include <map>
#include <typeindex>
#include <memory>

namespace RiftForged {
    namespace Dispatch {

        // Forward declare the IMessageHandler interface we defined.
        class IMessageHandler;

        /**
         * @brief The central command router.
         *
         * This class takes a GameCommand and dispatches it to the appropriate
         * registered handler. It has no knowledge of networking or FlatBuffers.
         */
        class MessageDispatcher {
        public:
            /**
             * @brief Registers a handler for a specific command data type.
             * @tparam T The specific command data struct (e.g., Commands::UseAbility).
             * @param handler A shared pointer to an object that implements IMessageHandler.
             */
            template<typename T>
            void RegisterHandler(std::shared_ptr<IMessageHandler> handler) {
                m_handlers[std::type_index(typeid(T))] = handler;
            }

            /**
             * @brief Dispatches a GameCommand to its registered handler.
             * @param command The command to be dispatched.
             */
            void DispatchGameCommand(const GameLogic::Commands::GameCommand& command);

        private:
            // A map from a C++ type's unique ID to the handler for that type.
            std::map<std::type_index, std::shared_ptr<IMessageHandler>> m_handlers;
        };

    } // namespace Dispatch
} // namespace RiftForged