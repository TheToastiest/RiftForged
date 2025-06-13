// File: ServerEngine/CacheService.cpp

#include "CacheService.h"
#include "../Utilities/Logger.h" // For logging

// We need the full definition of redis-plus-plus here.
// This path depends on where you placed the submodule.
#include "sw/redis++/redis++.h"

namespace RiftForged {
    namespace Server {

        CacheService::CacheService(const std::string& connection_string) {
            try {
                // Create the Redis object, which handles the connection.
                m_redis = std::make_unique<sw::redis::Redis>(connection_string);

                // Ping the server to ensure the connection is live.
                if (m_redis->ping() == "PONG") {
                    RF_CORE_INFO("CacheService: Successfully connected to DragonflyDB at {}.", connection_string);
                }
                else {
                    RF_CORE_ERROR("CacheService: Connected, but PING command failed.");
                    m_redis.reset(); // Invalidate the connection if ping fails
                }
            }
            catch (const sw::redis::Error& e) {
                RF_CORE_CRITICAL("CacheService: Failed to connect to DragonflyDB. Error: {}", e.what());
                // m_redis remains nullptr
            }
        }

        CacheService::~CacheService() {
            // The unique_ptr will automatically handle cleanup.
        }

        bool CacheService::IsConnected() const {
            return m_redis != nullptr;
        }

        void CacheService::PublishPlayerState(uint64_t player_id, const std::vector<uint8_t>& buffer) {
            if (!IsConnected()) {
                return; // Don't try to publish if not connected.
            }

            // The key will be something like "player:1:state"
            std::string key = "player:" + std::to_string(player_id) + ":state";

            // redis-plus-plus uses StringView, which can be constructed from a pointer and a size.
            // This avoids making an extra copy of the data into a std::string.
            sw::redis::StringView value(reinterpret_cast<const char*>(buffer.data()), buffer.size());

            try {
                // Use the Redis 'SET' command to store the data.
                m_redis->set(key, value);
            }
            catch (const sw::redis::Error& e) {
                RF_CORE_ERROR("CacheService: Failed to SET key '{}'. Error: {}", key, e.what());
                // TODO: Handle potential disconnection here.
            }
        }

        std::optional<std::vector<uint8_t>> CacheService::GetPlayerState(uint64_t player_id) {
            if (!IsConnected()) {
                return std::nullopt;
            }

            std::string key = "player:" + std::to_string(player_id) + ":state";

            try {
                // CORRECTED: Use 'auto' to let the compiler deduce the correct type,
                // which is sw::redis::OptionalString.
                auto value_str = m_redis->get(key);

                if (value_str) {
                    // The rest of the logic works perfectly, because sw::redis::OptionalString
                    // can be checked like a bool and dereferenced with *.
                    const std::string& str = *value_str;
                    return std::vector<uint8_t>(str.begin(), str.end());
                }
                else {
                    // The key does not exist in the database.
                    return std::nullopt;
                }

            }
            catch (const sw::redis::Error& e) {
                RF_CORE_ERROR("CacheService: Failed to GET key '{}'. Error: {}", key, e.what());
                return std::nullopt;
            }
        }

    } // namespace Server
} // namespace RiftForged