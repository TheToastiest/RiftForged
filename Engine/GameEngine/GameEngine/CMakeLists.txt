# File: RiftForged/Engine/GameEngine/GameEngine/CMakeLists.txt

add_library(GameEngine
    "src/GameEngine.cpp"
    "src/ActivePlayer.cpp"
    "src/PlayerManager.cpp"
    # ...etc
)

add_library(RiftForged::GameEngine ALIAS GameEngine)

target_include_directories(GameEngine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# GameEngine depends on GameLogic, PhysicsEngine, and Core
target_link_libraries(GameEngine PUBLIC
    GameLogic
    PhysicsEngine
    Core
)