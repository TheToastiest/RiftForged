# =============================================================================
# CMAKE-MINIMUM-REQUIRED & PROJECT
# =============================================================================
cmake_minimum_required(VERSION 3.22)
project(RiftForged VERSION 1.0 LANGUAGES CXX)

# =============================================================================
# CENTRAL DEPENDENCY PATH
# =============================================================================
set(THIRD_PARTY_ROOT "C:/project_includes" CACHE PATH "Root directory for pre-built third-party libraries")
message(STATUS "Using third-party libraries from: ${THIRD_PARTY_ROOT}")

# =============================================================================
# PROJECT-WIDE SETTINGS
# =============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# =============================================================================
# DEFINE PRE-BUILT THIRD-PARTY LIBRARIES
# =============================================================================
# --- spdlog ---
add_library(spdlog::spdlog STATIC IMPORTED GLOBAL)
set_target_properties(spdlog::spdlog PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${THIRD_PARTY_ROOT}/spdlog-1.x/build/install/include"
    IMPORTED_LOCATION             "${THIRD_PARTY_ROOT}/spdlog-1.x/build/install/lib/spdlog.lib"
)

# --- glm ---
add_library(glm::glm STATIC IMPORTED GLOBAL)
set_target_properties(glm::glm PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${THIRD_PARTY_ROOT}/glm-master/build/install/include"
    IMPORTED_LOCATION             "${THIRD_PARTY_ROOT}/glm-master/build/install/lib/glm.lib"
)

# --- PhysX SDK Libraries (Complete Definition) ---
add_library(PhysX::Foundation STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::Foundation PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${THIRD_PARTY_ROOT}/physx/include"
    IMPORTED_LOCATION             "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXFoundation_64.lib"
)

add_library(PhysX::Common STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::Common PROPERTIES IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXCommon_64.lib")
target_link_libraries(PhysX::Common INTERFACE PhysX::Foundation)

add_library(PhysX::PhysX STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::PhysX PROPERTIES IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysX_64.lib")
target_link_libraries(PhysX::PhysX INTERFACE PhysX::Common)

add_library(PhysX::Cooking STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::Cooking PROPERTIES IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXCooking_64.lib")
target_link_libraries(PhysX::Cooking INTERFACE PhysX::Common)

add_library(PhysX::Extensions STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::Extensions PROPERTIES IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXExtensions_static_64.lib")
target_link_libraries(PhysX::Extensions INTERFACE PhysX::PhysX)

add_library(PhysX::CharacterKinematic STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::CharacterKinematic PROPERTIES IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXCharacterKinematic_static_64.lib")
target_link_libraries(PhysX::CharacterKinematic INTERFACE PhysX::PhysX)

add_library(PhysX::PvdSDK STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::PvdSDK PROPERTIES IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXPvdSDK_static_64.lib")
target_link_libraries(PhysX::PvdSDK INTERFACE PhysX::Foundation)

add_library(PhysX::PVDRuntime STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::PVDRuntime PROPERTIES IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PVDRuntime_64.lib")
target_link_libraries(PhysX::PVDRuntime INTERFACE PhysX::PvdSDK)

add_library(PhysX::LowLevel STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::LowLevel PROPERTIES IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/LowLevel_static_64.lib")
target_link_libraries(PhysX::LowLevel INTERFACE PhysX::Foundation)

add_library(PhysX::LowLevelAABB STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::LowLevelAABB PROPERTIES IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/LowLevelAABB_static_64.lib")
target_link_libraries(PhysX::LowLevelAABB INTERFACE PhysX::LowLevel)

add_library(PhysX::LowLevelDynamics STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::LowLevelDynamics PROPERTIES IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/LowLevelDynamics_static_64.lib")
target_link_libraries(PhysX::LowLevelDynamics INTERFACE PhysX::LowLevel)

add_library(PhysX::CudaContextManager STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::CudaContextManager PROPERTIES IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXCudaContextManager_static_64.lib")
target_link_libraries(PhysX::CudaContextManager INTERFACE PhysX::Foundation)

add_library(PhysX::GPU STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::GPU PROPERTIES IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXGpu_64.lib")
target_link_libraries(PhysX::GPU INTERFACE PhysX::PhysX)

add_library(PhysX::SceneQuery STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::SceneQuery PROPERTIES IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/SceneQuery_static_64.lib")
target_link_libraries(PhysX::SceneQuery INTERFACE PhysX::PhysX)

add_library(PhysX::Task STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::Task PROPERTIES IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXTask_static_64.lib")
target_link_libraries(PhysX::Task INTERFACE PhysX::Foundation)

add_library(PhysX::Vehicle STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::Vehicle PROPERTIES IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXVehicle2_static_64.lib")
target_link_libraries(PhysX::Vehicle INTERFACE PhysX::PhysX)

# Consolidated PhysX SDK Interface Target
add_library(PhysX::SDK INTERFACE IMPORTED)
target_link_libraries(PhysX::SDK INTERFACE
    PhysX::PhysX
    PhysX::Cooking
    PhysX::Extensions
    PhysX::CharacterKinematic
    PhysX::PvdSDK
    PhysX::PVDRuntime
    PhysX::LowLevel
    PhysX::LowLevelAABB
    PhysX::LowLevelDynamics
    PhysX::CudaContextManager
    PhysX::GPU
    PhysX::SceneQuery
    PhysX::Task
    PhysX::Vehicle
)

# =============================================================================
# ADD OUR OWN ENGINE MODULES
# =============================================================================
add_subdirectory(Engine/Core)
add_subdirectory(Engine/NetworkEngine)
add_subdirectory(Engine/PhysicsEngine)
add_subdirectory(Engine/AIEngine)
add_subdirectory(Engine/GameEngine) # Builds both GameLogic and GameEngine libs
add_subdirectory(Server/ServerEngine)
#add_subdirectory(AIEngine/)
#add_subdirectory(AIEngine/)
#add_subdirectory(AIEngine/)
#add_subdirectory(AIEngine/)

# =============================================================================
# DEFINE THE FINAL EXECUTABLE
# =============================================================================
add_executable(RiftForgedServer App/main.cpp)

# =============================================================================
# LINK LIBRARIES
# =============================================================================
target_link_libraries(RiftForgedServer PRIVATE
    ServerEngine
)

# =============================================================================
# BUILD SUMMARY
# =============================================================================
message(STATUS "--------------------------------------------------")
message(STATUS "Project:           ${PROJECT_NAME}")
message(STATUS "Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "Output binaries:   ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "--------------------------------------------------")