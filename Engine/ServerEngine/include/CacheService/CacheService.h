// File: ServerEngine/CacheService.h

#pragma once

#include <optional>
#include <string>
#include <memory>
#include <vector>
#include <cstdint>

// Forward declare the redis-plus-plus Redis object
namespace sw { namespace redis { class Redis; } }

namespace RiftForged {
    namespace Server {

        class CacheService {
        public:
            // Connects to the DragonflyDB instance upon construction.
            // The connection string is typically "tcp://127.0.0.1:6379" (or your port).
            explicit CacheService(const std::string& connection_string);
            ~CacheService();

            // Check if the service is connected to the database.
            bool IsConnected() const;

            // Publishes a player's state. It takes the raw, serialized FlatBuffer data.
            void PublishPlayerState(uint64_t player_id, const std::vector<uint8_t>& buffer);

            std::optional<std::vector<uint8_t>> GetPlayerState(uint64_t player_id);

        private:
            // PIMPL (Pointer to Implementation) idiom can be useful here, but for simplicity
            // we'll use a unique_ptr to the Redis object directly.
            std::unique_ptr<sw::redis::Redis> m_redis;
        };

    } // namespace Server
} // namespace RiftForged