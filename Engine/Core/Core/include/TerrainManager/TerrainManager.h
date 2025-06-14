// File: // Core/TerrainManager.h
// Copyright (c) 2025-202* RiftForged Game Development Team

#pragma once

#include <string>
#include <unordered_map>
#include <RiftForged/Core/TerrainData/TerrainData.h> 

namespace RiftForged {
    namespace Core {

        class TerrainManager {
        public:
            TerrainManager();
            TerrainManager(const TerrainManager&) = delete;
            TerrainManager& operator=(const TerrainManager&) = delete;

            // This function stays the same. It's used at startup.
            void RegisterTerrainType(const std::string& assetName, const TerrainAssetInfo& info);
            const TerrainAssetInfo* GetAssetInfo(const std::string& assetName) const;

            // This is the NEW public function. It replaces the Generate...Mesh functions.
            // It's the only function the ZoneOrchestrator will need to call.
            //const TerrainMeshData* GetOrLoadTerrainMesh(const std::string& assetName); // <--- CHANGED/RENAMED

            TerrainMeshData GenerateSingleTerrainMesh(
                const std::string& assetName,
                const glm::vec3& position
            );

            TerrainMeshData GenerateTerrainChunkMesh(
                const std::string& assetName,
                const glm::vec3& zone_world_position,
                int chunk_x, int chunk_y,
                int chunk_resolution
            );

        private:
            bool loadAndStoreTerrain(const std::string& assetName, const TerrainAssetInfo& info);
            // The registry stores the METADATA for all known terrain types.
            std::unordered_map<std::string, TerrainAssetInfo> mAssetRegistry;

            // This map now acts as a CACHE for the fully PROCESSED heightfield data.
            std::unordered_map<std::string, ProcessedHeightfieldData> mProcessedDataCache;

            std::unordered_map<std::string, HeightmapData> mTerrainAssets;

            };

    } // namespace Core
} // namespace RiftForged