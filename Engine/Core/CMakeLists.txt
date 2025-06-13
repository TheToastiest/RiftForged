# RiftForged/Engine/Core/CMakeLists.txt

# Explicitly list all source files for the Core library.
# This is the most robust way to manage your source list.
set(CORE_SOURCES
    "src/Logger/Logger.cpp"
    "src/Threadpool/Threadpool.cpp"
    "src/TerrainManager/TerrainManager.cpp" #
)

# Create the "Core" library from all .cpp files in the src folder
add_library(Core ${CORE_SOURCES})

# Define a modern alias for this target
add_library(RiftForged::Core ALIAS Core)

# --- PUBLIC INCLUDE DIRECTORY ---
# Make our public headers in the "include" folder available to any other
# library that links against Core.
target_include_directories(Core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# --- DEPENDENCIES ---
# The Core library needs spdlog and glm to compile.
# Since our root CMakeLists.txt has already defined "spdlog::spdlog" and
# "glm::glm", we can link against them here.
target_link_libraries(Core PUBLIC
    spdlog::spdlog
    glm::glm
)

# --- COMPILER WARNINGS ---
# Set strict warning levels for this module specifically.
# ADDED /utf-8 for MSVC to handle Unicode source files correctly.
if(MSVC)
    target_compile_options(Core PRIVATE /W4 /WX /utf-8)
else()
    target_compile_options(Core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# (Any other commands specific to your Core module)