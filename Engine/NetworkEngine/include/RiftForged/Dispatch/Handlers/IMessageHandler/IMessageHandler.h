#pragma once

// Forward declare the GameCommand struct. This is the standard "package" of work
// that all handlers will receive.
namespace RiftForged { namespace GameLogic { namespace Commands { struct GameCommand; } } }

namespace RiftForged {
    namespace Dispatch {

        /**
         * @brief The contract for all message handlers.
         *
         * This interface guarantees that any class claiming to be a handler
         * will have a Process method that accepts a GameCommand.
         */
        class IMessageHandler {
        public:
            virtual ~IMessageHandler() = default;

            /**
             * @brief The required function for all handlers. This is what the
             * MessageDispatcher will call.
             * @param command The command to be processed.
             */
            virtual void Process(const GameLogic::Commands::GameCommand& command) = 0;
        };

    } // namespace Dispatch
} // namespace RiftForged