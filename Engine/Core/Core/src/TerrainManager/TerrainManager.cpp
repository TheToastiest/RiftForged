// File: // Core/TerrainManager.cpp
// Copyright (c) 2025-202* RiftForged Game Development Team

#include <RiftForged/Core/TerrainManager/TerrainManager.h>    
#include <RiftForged/Utilities/AssetLoader/AssetLoader.h> 
#include <RiftForged/Utilities/Logger/Logger.h>


namespace RiftForged {
    namespace Core {

        TerrainManager::TerrainManager() {
            RF_CORE_INFO("TerrainManager: System constructed and ready.");
        }

        void TerrainManager::RegisterTerrainType(const std::string& assetName, const TerrainAssetInfo& info) {
            if (mAssetRegistry.count(assetName)) {
                RF_CORE_WARN("TerrainManager: Terrain asset '{}' is already registered. Overwriting.", assetName);
            }
            mAssetRegistry[assetName] = info;
            RF_CORE_INFO("TerrainManager: Registered terrain type '{}' with file '{}'.", assetName, info.filePath);
        }

        const TerrainAssetInfo* TerrainManager::GetAssetInfo(const std::string& assetName) const {
            auto it = mAssetRegistry.find(assetName);
            if (it != mAssetRegistry.end()) {
                return &it->second;
            }
            return nullptr;
        }

        bool TerrainManager::loadAndStoreTerrain(const std::string& assetName, const TerrainAssetInfo& info) {
            HeightmapData loadedData;
            bool success = false;

            // Determine loading method based on file type or extension (as per your AssetLoader calls)
            if (info.filePath.find(".raw") != std::string::npos) {
                success = RiftForged::Utilities::LoadHeightmapFromRaw8(info.filePath, info.numRows, info.numCols, loadedData);
            }
            else { // Assume it's a float binary file like your tool outputs (FileNameZX no extension)
                // Or add more sophisticated checks for .r32 vs. no extension
                success = RiftForged::Utilities::LoadHeightmapFromRaw32Float(info.filePath, info.numRows, info.numCols, loadedData);
            }

            if (success) {
                mTerrainAssets[assetName] = std::move(loadedData); // Cache the loaded HeightmapData
                RF_CORE_INFO("TerrainManager: Successfully loaded and stored raw data for '{}'.", assetName);
                return true;
            }
            else {
                RF_CORE_ERROR("TerrainManager: Failed to load and store raw data for '{}'.", assetName);
                return false;
            }
        }


        TerrainMeshData TerrainManager::GenerateSingleTerrainMesh(
            const std::string& assetName,
            const glm::vec3& position
        ) {
            auto it = mTerrainAssets.find(assetName); // Initial cache lookup

            // If the asset is not in our cache, we need to load it.
            if (it == mTerrainAssets.end()) {
                // This is the on-demand loading block.
                RF_CORE_INFO("TerrainManager: Cache miss for '{}'. Triggering on-demand load.", assetName);

                // First, find the registration info.
                auto registryIt = mAssetRegistry.find(assetName);
                if (registryIt == mAssetRegistry.end()) {
                    RF_CORE_ERROR("TerrainManager: Cannot generate mesh. Asset '{}' was never registered.", assetName);
                    return {}; // Return empty mesh data
                }

                // Now, load the data from disk using the registration info.
                // This function will populate mTerrainAssets if successful.
                if (!loadAndStoreTerrain(assetName, registryIt->second)) {
                    // Loading from disk failed. The error is logged inside the function.
                    return {}; // Return empty mesh data
                }

                // --- THE FIX IS HERE ---
                // After a successful load, we MUST find the asset in the cache again
                // to get a valid iterator to the new data. The previous 'it' is now invalid.
                it = mTerrainAssets.find(assetName); // <--- RE-LOOKUP THE ITERATOR
                if (it == mTerrainAssets.end()) {
                    // This should never happen if loading succeeded and stored correctly, but it's a critical safety check.
                    RF_CORE_CRITICAL("TerrainManager: Data for '{}' was loaded but could not be found in cache after re-lookup!", assetName);
                    return {}; // Return empty mesh data
                }
            }

            // By this point, 'it' is GUARANTEED to be a valid iterator pointing to our terrain data.
            RF_CORE_INFO("TerrainManager: Generating mesh for asset '{}'...", assetName);
            const HeightmapData& heightmap = it->second; // This line is now safe.
            TerrainMeshData meshData;

            // The vertex and index generation logic is unchanged.
            const size_t totalVertices = static_cast<size_t>(heightmap.numRows) * heightmap.numCols;
            meshData.vertices.reserve(totalVertices);
            for (uint32_t r = 0; r < heightmap.numRows; ++r) {
                for (uint32_t c = 0; c < heightmap.numCols; ++c) {
                    int16_t sample = heightmap.samples[static_cast<size_t>(r) * heightmap.numCols + c];
                    meshData.vertices.emplace_back(
                        position.x + (c * heightmap.colScale),    // X (horizontal)
                        position.y + (r * heightmap.rowScale),    // Y (horizontal, "depth" in your log terms)
                        position.z + (sample * heightmap.heightScale)    // Z (vertical height)
                    );
                }
            }

            const size_t totalIndices = static_cast<size_t>(heightmap.numRows - 1) * (heightmap.numCols - 1) * 6;
            meshData.indices.reserve(totalIndices);
            for (uint32_t r = 0; r < heightmap.numRows - 1; ++r) {
                for (uint32_t c = 0; c < heightmap.numCols - 1; ++c) {
                    uint32_t topLeft = r * heightmap.numCols + c;
                    uint32_t topRight = topLeft + 1;
                    uint32_t bottomLeft = (r + 1) * heightmap.numCols + c;
                    uint32_t bottomRight = bottomLeft + 1;

                    // Triangle 1 (e.g., top-left triangle of quad)
                    meshData.indices.push_back(topLeft);
                    meshData.indices.push_back(bottomLeft);
                    meshData.indices.push_back(topRight);

                    // Triangle 2 (e.g., bottom-right triangle of quad)
                    meshData.indices.push_back(topRight);
                    meshData.indices.push_back(bottomLeft);
                    meshData.indices.push_back(bottomRight);
                }
            }

            RF_CORE_INFO("TerrainManager: Successfully generated mesh for '{}'. Vertices: {}, Indices: {}.", assetName, meshData.vertices.size(), meshData.indices.size());
            return meshData;
        }


    } // namespace Core
} // namespace RiftForged