# RiftForged/Engine/PhysicsEngine/CMakeLists.txt

# Explicitly list all source files for the PhysicsEngine library,
# pointing to their new locations in the organized subdirectories.
set(PHYSICS_ENGINE_SOURCES
    "src/PhysicsEngine/PhysicsEngine.cpp"
    "src/PhysicsActorFactory/PhysicsActorFactory.cpp"
    "src/PhysicsSceneQuery/PhysicsSceneQuery.cpp"
    "src/PhysicsFiltering/PhysicsFiltering.cpp"
    "src/PhysicsActorManagement/PhysicsActorManagement.cpp"
)

# Create the "PhysicsEngine" library from the sources listed above.
add_library(PhysicsEngine ${PHYSICS_ENGINE_SOURCES})

# Define a modern alias for this target for better namespacing.
add_library(RiftForged::Physics ALIAS PhysicsEngine)

# --- Compiler Options for PhysicsEngine ---
# Apply the /utf-8 flag for MSVC, similar to Core, as it uses spdlog.
if(MSVC)
    target_compile_options(PhysicsEngine PRIVATE /W4 /WX /utf-8) # <--- ADD THIS BLOCK
else()
    target_compile_options(PhysicsEngine PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Make the public headers in the "include" folder available to other modules.
target_include_directories(PhysicsEngine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# --- DEPENDENCIES ---
# The PhysicsEngine needs Core (for logging/math) and PhysX.
# We use PUBLIC because our public headers (PhysicsTypes.h) include files
# from both Core and PhysX, so any library that uses PhysicsEngine also
# needs to know about their headers.
target_link_libraries(PhysicsEngine PUBLIC
    Core
    # Changed from PhysX::PhysX to PhysX::SDK as per our aggregated target
    # This ensures all necessary PhysX components are linked.
    PhysX::SDK # <--- CHANGED THIS LINE
)