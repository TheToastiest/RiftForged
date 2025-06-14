# File: RiftForged/Engine/Core/CMakeLists.txt

# Explicitly list all source files for the Core library.
set(CORE_SOURCES
    "src/Logger/Logger.cpp"
    "src/Threadpool/Threadpool.cpp"
    "src/TerrainManager/TerrainManager.cpp"
    # ... any other .cpp files for your Core utilities ...
)

# Create the "Core" library from the source files.
add_library(Core ${CORE_SOURCES})

# Create a modern namespaced alias for easier linking (e.g., RiftForged::Core).
add_library(RiftForged::Core ALIAS Core)

# --- PUBLIC INCLUDE DIRECTORY ---
# Make the public headers in the "include" folder available to any other
# library that links against Core.
target_include_directories(Core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# --- DEPENDENCIES ---
# The Core library needs spdlog and glm to compile. By linking them here
# as PUBLIC, any other library that links to Core will automatically
# get access to spdlog and glm's headers.
target_link_libraries(Core PUBLIC
    spdlog::spdlog
    glm::glm
)