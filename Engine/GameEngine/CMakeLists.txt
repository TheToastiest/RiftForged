# File: RiftForged/Engine/GameEngine/CMakeLists.txt

# This file orchestrates the build for the modules inside this directory.

# 1. First, add the GameLogic subdirectory. CMake will find its own
#    CMakeLists.txt inside and build the "GameLogic" library target.
add_subdirectory(GameLogic)

# 2. Second, add the GameEngine subdirectory. Its CMakeLists.txt will
#    build the "GameEngine" library, which can now link against GameLogic.
add_subdirectory(GameEngine)