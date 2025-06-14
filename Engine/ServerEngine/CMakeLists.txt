# Defines the build instructions for the ServerEngine library.

# Define the list of source files for this module.
set(SERVERENGINE_SOURCES
    "src/ServerEngine/ServerEngine.cpp"
    "src/ShardEngine/ShardEngine.cpp"
    "src/CacheService/CacheService.cpp"
)

# Create the "ServerEngine" library from the source files.
add_library(ServerEngine ${SERVERENGINE_SOURCES})

# Create a modern namespaced alias for easier linking.
add_library(RiftForged::ServerEngine ALIAS ServerEngine)

# --- PUBLIC INCLUDE DIRECTORY ---
# Make the public headers in this module's "include" folder available.
target_include_directories(ServerEngine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# --- DEPENDENCIES ---
# The ServerEngine is the top-level module and links against all the other
# major engine components it needs to orchestrate.
target_link_libraries(ServerEngine PUBLIC
    GameEngine
    NetworkEngine # <-- This will transitively link Core, GameLogic, etc.
)