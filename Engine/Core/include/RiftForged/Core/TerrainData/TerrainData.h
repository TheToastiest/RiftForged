// File: Core/TerrainData.h
// Copyright (c) 2025-202* RiftForged Game Development Team

#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <glm/glm.hpp>

namespace RiftForged {
    namespace Core {

        struct HeightmapData {
            // Your data members remain the same
            std::vector<int16_t> samples;
            uint32_t numRows = 0;
            uint32_t numCols = 0;
            float heightScale = 1.0f;
            float rowScale = 1.0f;
            float colScale = 1.0f;

            // --- THE FIX: The Rule of Five ---
            // By explicitly defaulting all five special member functions, we guarantee
            // to the compiler that this struct is default-constructible,
            // copyable, and movable, making it safe for use in containers
            // like std::unordered_map.

            HeightmapData() = default;
            ~HeightmapData() = default;
            HeightmapData(const HeightmapData&) = default;
            HeightmapData& operator=(const HeightmapData&) = default;
            HeightmapData(HeightmapData&&) = default;
            HeightmapData& operator=(HeightmapData&&) = default;
        };

        struct TerrainAssetInfo {
            std::string filePath;
            uint32_t numRows;
            uint32_t numCols;
            float heightScale;
            float horizontalScale;
        };

        struct ProcessedHeightfieldData {
            uint32_t rows = 0;
            uint32_t cols = 0;
            float heightScale = 1.0f;
            float horizontalScale = 1.0f;
            std::vector<int16_t> samples;
        };

        struct TerrainHeightData {
            std::vector<int16_t> samples; // The height values as 16-bit integers
            uint32_t rows = 0;
            uint32_t cols = 0;
        };

        // This struct remains unchanged
        struct TerrainMeshData {
            std::vector<glm::vec3> vertices;
            std::vector<uint32_t> indices;
        };
    } // namespace Core
} // namespace RiftForgedbin