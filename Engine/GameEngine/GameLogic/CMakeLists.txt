# File: RiftForged/Engine/GameEngine/GameLogic/CMakeLists.txt

add_library(GameLogic
    # List all your GameLogic source files here
    "src/CombatSystem/CombatSystem.cpp"
    #"src/RiftStepLogic/RiftStepLogic.cpp"
    #"src/CombatEvents/CombatEvents.cpp"
    #"src/"
)

add_library(RiftForged::GameLogic ALIAS GameLogic)

target_include_directories(GameLogic PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# GameLogic's only dependency is Core
target_link_libraries(GameLogic PUBLIC Core)