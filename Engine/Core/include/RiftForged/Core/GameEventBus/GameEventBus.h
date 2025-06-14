// File: GameEventBus.h
// Copyright (c) 2025-202* RiftForged Game Development Team

#pragma once

#include <functional>
#include <map>
#include <vector>
#include <any>
#include <typeindex>

namespace RiftForged {
    namespace Events {

        /**
         * @brief A generic function wrapper for any function that can handle an event.
         * The std::any parameter will hold the specific event data struct (e.g., ProjectileSpawned).
         */
        using EventHandlerFunction = std::function<void(const std::any&)>;

        class GameEventBus {
        public:
            /**
             * @brief Subscribes a listener to a specific event type.
             *
             * Example Usage:
             * eventBus.Subscribe<GameLogic::Events::ProjectileSpawned>(
             * [this](const std::any& eventData){ OnProjectileSpawned(eventData); }
             * );
             *
             * @tparam TEvent The event struct type to listen for.
             * @param handler The function that will be called when the event is published.
             */
            template<typename TEvent>
            void Subscribe(EventHandlerFunction handler) {
                // We use the C++ typeid system to get a unique key for each event struct.
                m_subscribers[std::type_index(typeid(TEvent))].push_back(handler);
            }

            /**
             * @brief Publishes an event to all registered listeners.
             *
             * Example Usage:
             * GameLogic::Events::ProjectileSpawned event;
             * // ... fill event data ...
             * eventBus.Publish(event);
             *
             * @tparam TEvent The event struct type being published.
             * @param event The event data itself.
             */
            template<typename TEvent>
            void Publish(const TEvent& event) {
                const auto type_idx = std::type_index(typeid(TEvent));

                // Check if any system has subscribed to this type of event.
                if (m_subscribers.count(type_idx)) {
                    // Convert the specific event (like ProjectileSpawned) into a generic
                    // std::any to be passed to the handler functions.
                    std::any eventData = event;

                    // Call every function that is subscribed to this event type.
                    for (const auto& handler : m_subscribers.at(type_idx)) {
                        handler(eventData);
                    }
                }
            }

        private:
            // The core of the event bus: a map from an event's type to a list of its listeners.
            std::map<std::type_index, std::vector<EventHandlerFunction>> m_subscribers;
        };

    } // namespace Events
} // namespace RiftForged