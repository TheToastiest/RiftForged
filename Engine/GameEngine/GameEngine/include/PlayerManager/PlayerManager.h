// File: Gameplay/PlayerManager.h (Refactored)
// RiftForged Game Development Team
// Copyright (c) 2023-2025 RiftForged Game Development Team

#pragma once

#include <cstdint>
#include <map>
#include <vector>
#include <mutex>
#include <memory> // For std::unique_ptr
#include <atomic>

#include <RiftForged/GameEngine/ActivePlayer/ActivePlayer.h>
#include <RiftForged/Utilities/Logger/Logger.h> 

// The direct include for FlatBuffers common types for Vec3/Quaternion is no longer needed here,
// as ActivePlayer.h should now include MathUtil.h for the GLM-based types.
// #include "../FlatBuffers/Versioning/V0.0.5/riftforged_common_types_generated.h" 

namespace RiftForged {
    namespace GameLogic {

        class PlayerManager {
        public:
            PlayerManager();
            ~PlayerManager();

            // Prevent copying
            PlayerManager(const PlayerManager&) = delete;
            PlayerManager& operator=(const PlayerManager&) = delete;

            // Creates a new player instance. Called by GameServerEngine.
            // The parameters for position and orientation are now GLM-based types.
            ActivePlayer* CreatePlayer(
                uint64_t playerId,
                const RiftForged::Utilities::Math::Vec3& startPos,
                const RiftForged::Utilities::Math::Quaternion& startOrientation,
                float cap_radius = 0.5f, float cap_half_height = 0.9f
            );

            // Removes a player by their unique PlayerID.
            // Returns true if player was found and removed, false otherwise.
            bool RemovePlayer(uint64_t playerId);

            // Finds a player by their unique PlayerID.
            ActivePlayer* FindPlayerById(uint64_t playerId) const;

            // Gets a list of pointers to all active player objects for iteration.
            // The const overload is called on const instances of PlayerManager.
            std::vector<ActivePlayer*> GetAllActivePlayerPointersForUpdate();
            std::vector<const ActivePlayer*> GetAllActivePlayerPointersForUpdate() const;

            // Utility for GameServerEngine to assign new IDs
            uint64_t GetNextAvailablePlayerID();
            uint64_t GetNextAvailableProjectileID();

        private:
            std::map<uint64_t, std::unique_ptr<ActivePlayer>> m_playersById;

            std::atomic<uint64_t> m_nextPlayerId;
            std::atomic<uint64_t> m_nextProjectileId;
            mutable std::mutex m_playerMapMutex; // Protects m_playersById map
        };

    } // namespace GameLogic
} // namespace RiftForged