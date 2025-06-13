// File: Utilities/AssetLoader.h (Updated for Raw Binary Files)
// Copyright (c) 2025-202* RiftForged Game Development Team

#pragma once

#include <string>
#include <vector>
#include <fstream> // For file I/O
#include <RiftForged/Utilities/Logger/Logger.h>
#include <RiftForged/Core/TerrainData/TerrainData.h>

namespace RiftForged {
    namespace Utilities {

        /**
         * @brief Loads an 8-bit raw binary heightmap file from disk.
         * @param filePath The path to the .raw file.
         * @param numRows The number of rows (height) of the terrain.
         * @param numCols The number of columns (width) of the terrain.
         * @param outData The HeightmapData struct to populate.
         * @return True on success, false on failure.
         */
        inline bool LoadHeightmapFromRaw8(
            const std::string& filePath,
            uint32_t numRows,
            uint32_t numCols,
            Core::HeightmapData& outData)
        {
            std::ifstream file(filePath, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                RF_CORE_ERROR("AssetLoader: Failed to open raw heightmap file '{}'", filePath);
                return false;
            }

            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            const size_t expectedSize = static_cast<size_t>(numRows) * numCols;
            if (static_cast<size_t>(size) != expectedSize) { // <--- MODIFIED LINE 39
                RF_CORE_ERROR("AssetLoader: Raw heightmap file '{}' has unexpected size. Expected: {}, Got: {}", filePath, expectedSize, size);
                return false;
            }

            // Read the raw 8-bit data into a temporary buffer
            std::vector<uint8_t> raw_samples(expectedSize);
            if (!file.read(reinterpret_cast<char*>(raw_samples.data()), size)) {
                RF_CORE_ERROR("AssetLoader: Failed to read data from raw heightmap file '{}'", filePath);
                return false;
            }

            // Convert the 8-bit data (0-255) to the 16-bit integers that PhysX needs
            outData.numRows = numRows;
            outData.numCols = numCols;
            outData.samples.clear();
            outData.samples.reserve(expectedSize);
            for (uint8_t sample : raw_samples) {
                outData.samples.push_back(static_cast<int16_t>(sample));
            }

            RF_CORE_INFO("AssetLoader: Successfully loaded raw heightmap '{}'. Dimensions: {}x{}.", filePath, numCols, numRows);
            return true;
        }

        inline bool LoadHeightmapFromRaw32Float(
            const std::string& filePath,
            uint32_t numRows_expected, // Parameter names adjusted for clarity
            uint32_t numCols_expected, // Parameter names adjusted for clarity
            Core::HeightmapData& outData)
        {
            std::ifstream file(filePath, std::ios::binary); // Open at beginning, no std::ios::ate
            if (!file.is_open()) {
                RF_CORE_ERROR("AssetLoader: Failed to open raw float heightmap file '{}'", filePath);
                return false;
            }

            // --- 1. Read Header (int width, int height) ---
            int fileWidth, fileHeight;
            if (!file.read(reinterpret_cast<char*>(&fileWidth), sizeof(int))) {
                RF_CORE_ERROR("AssetLoader: Failed to read width from header of '{}'. File too small or corrupt.", filePath);
                return false;
            }
            if (!file.read(reinterpret_cast<char*>(&fileHeight), sizeof(int))) {
                RF_CORE_ERROR("AssetLoader: Failed to read height from header of '{}'. File too small or corrupt.", filePath);
                return false;
            }

            // Validate header dimensions against expected dimensions from AssetInfo
            if (static_cast<uint32_t>(fileWidth) != numCols_expected || static_cast<uint32_t>(fileHeight) != numRows_expected) {
                RF_CORE_WARN("AssetLoader: Dimensions from file header ({},{}) mismatch expected ({},{}) for '{}'. Using file header dimensions.",
                    fileWidth, fileHeight, numCols_expected, numRows_expected, filePath);
                // Update internal dimensions to match file, so calculations are correct
                outData.numCols = static_cast<uint32_t>(fileWidth);
                outData.numRows = static_cast<uint32_t>(fileHeight);
            }
            else {
                outData.numCols = numCols_expected;
                outData.numRows = numRows_expected;
            }

            // Calculate expected size of only the pixel data (after header)
            const size_t expectedPixelDataSize = static_cast<size_t>(outData.numRows) * outData.numCols * sizeof(float);

            // --- 2. Robust File Size Validation ---
            std::streampos currentReadPos = file.tellg(); // Get current position (after header)
            file.seekg(0, std::ios::end); // Go to end
            std::streampos totalFileSize = file.tellg(); // Get total size
            file.seekg(currentReadPos); // Reset file pointer to after header for reading data

            std::streamsize actualPixelDataSizeOnDisk = totalFileSize - currentReadPos;

            if (actualPixelDataSizeOnDisk < static_cast<std::streamsize>(expectedPixelDataSize)) {
                RF_CORE_ERROR("AssetLoader: Raw float heightmap file '{}' is truncated. Expected data size: {}, Actual data on disk: {}. Header size: {} bytes.",
                    filePath, expectedPixelDataSize, actualPixelDataSizeOnDisk, static_cast<size_t>(currentReadPos));
                return false;
            }
            if (actualPixelDataSizeOnDisk > static_cast<std::streamsize>(expectedPixelDataSize)) {
                RF_CORE_WARN("AssetLoader: Raw float heightmap file '{}' has excess data. Expected: {}, Actual: {}. Reading expected amount.",
                    filePath, expectedPixelDataSize, actualPixelDataSizeOnDisk);
            }

            // --- 3. Read the Raw Float Data ---
            std::vector<float> raw_float_samples(static_cast<size_t>(outData.numRows) * outData.numCols);
            if (!file.read(reinterpret_cast<char*>(raw_float_samples.data()), raw_float_samples.size() * sizeof(float))) {
                RF_CORE_ERROR("AssetLoader: Failed to read pixel data from raw float heightmap file '{}'", filePath);
                return false;
            }

            // --- 4. Scale Float Samples to POSITIVE int16_t Range [0, 32767] for PhysX ---
            outData.samples.clear();
            outData.samples.reserve(raw_float_samples.size());

            if (raw_float_samples.empty()) {
                RF_CORE_WARN("AssetLoader: Loaded float heightmap has no samples. Resulting terrain will be flat.");
                return true; // Still a success, but flat
            }

            // Find min/max values in the float data to define its range
            float min_val_float = raw_float_samples[0];
            float max_val_float = raw_float_samples[0];
            for (float sample : raw_float_samples) {
                if (sample < min_val_float) min_val_float = sample;
                if (sample > max_val_float) max_val_float = sample;
            }
            RF_CORE_INFO("AssetLoader: Raw float heightmap samples range from {:.2f} to {:.2f}.", min_val_float, max_val_float);

            float float_range = max_val_float - min_val_float;

            // Define the target int16_t range variables at function scope
            const float TARGET_INT16_POSITIVE_MAX = static_cast<float>(std::numeric_limits<int16_t>::max()); // 32767.0f
            const float TARGET_INT16_MIN_VAL = 0.0f; // <--- Corrected: Map to 0
            const float TARGET_INT16_RANGE = TARGET_INT16_POSITIVE_MAX - TARGET_INT16_MIN_VAL; // Will be 32767.0f

            for (float sample : raw_float_samples) {
                int16_t scaled_sample;
                if (float_range == 0.0f) {
                    // If the float heightmap is perfectly flat, map all samples to 0.
                    scaled_sample = static_cast<int16_t>(TARGET_INT16_MIN_VAL); // Maps to 0
                }
                else {
                    // Normalize the float sample to 0.0-1.0 based on its actual range.
                    float normalized_val = (sample - min_val_float) / float_range;

                    // Scale this normalized value to the target positive int16_t range.
                    scaled_sample = static_cast<int16_t>(normalized_val * TARGET_INT16_RANGE + TARGET_INT16_MIN_VAL);
                }
                outData.samples.push_back(scaled_sample);
            }
            RF_CORE_INFO("AssetLoader: Float samples scaled and converted to int16_t range [{:.0f}, {:.0f}].", TARGET_INT16_MIN_VAL, TARGET_INT16_POSITIVE_MAX);


            RF_CORE_INFO("AssetLoader: Successfully loaded raw float heightmap '{}'. Final dimensions: {}x{}.", filePath, outData.numCols, outData.numRows);
            return true;
        }

    } // namespace Utilities
} // namespace RiftForged