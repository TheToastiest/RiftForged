# =============================================================================
# CMAKE-MINIMUM-REQUIRED & PROJECT
# =============================================================================
cmake_minimum_required(VERSION 3.22)
project(RiftForged VERSION 1.0 LANGUAGES CXX)

# =============================================================================
# CENTRAL DEPENDENCY PATH
# =============================================================================
# Set the single, central path to the folder containing all pre-built libraries.
set(THIRD_PARTY_ROOT "C:/project_includes" CACHE PATH "Root directory for pre-built third-party libraries")
message(STATUS "Using third-party libraries from: ${THIRD_PARTY_ROOT}")

# =============================================================================
# PROJECT-WIDE SETTINGS
# =============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8) # <--- ADD THIS LINE
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
# We create "IMPORTED" targets to tell CMake where to find your pre-built libs.
# Please verify the sub-paths like "/spdlog-1.x/..." match your folders exactly.

# --- spdlog (with .lib file) ---
add_library(spdlog::spdlog STATIC IMPORTED GLOBAL)
set_target_properties(spdlog::spdlog PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${THIRD_PARTY_ROOT}/spdlog-1.x/build/install/include"
    IMPORTED_LOCATION             "${THIRD_PARTY_ROOT}/spdlog-1.x/build/install/lib/spdlog.lib"
)

# --- glm (with .lib file) ---
add_library(glm::glm STATIC IMPORTED GLOBAL)
set_target_properties(glm::glm PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${THIRD_PARTY_ROOT}/glm-master/build/install/include"
    IMPORTED_LOCATION             "${THIRD_PARTY_ROOT}/glm-master/build/install/lib/glm.lib"
)

# --- (The rest are templates for you to use later) ---

# add_library(redis++::redis++ STATIC IMPORTED GLOBAL)
# set_target_properties(redis++::redis++ PROPERTIES
#     INTERFACE_INCLUDE_DIRECTORIES "${THIRD_PARTY_ROOT}/redis-plus-plus-master/build/install"
#     IMPORTED_LOCATION             "${THIRD_PARTY_ROOT}/redis-plus-plus-master/build/install/lib/redis++.lib"
# )


# --- PhysX SDK Libraries ---
# PhysX is a collection of libraries. We define the ones we need and then
# bundle them into a single easy-to-use target: PhysX::SDK

# Common PhysX Core Libraries
add_library(PhysX::Foundation STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::Foundation PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${THIRD_PARTY_ROOT}/physx/include"
    IMPORTED_LOCATION             "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXFoundation_64.lib"
)

add_library(PhysX::Common STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::Common PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXCommon_64.lib"
)
target_link_libraries(PhysX::Common INTERFACE PhysX::Foundation) # Common depends on Foundation

add_library(PhysX::PhysX STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::PhysX PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysX_64.lib"
)
target_link_libraries(PhysX::PhysX INTERFACE PhysX::Common) # Main PhysX lib depends on Common

add_library(PhysX::Cooking STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::Cooking PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXCooking_64.lib"
)
target_link_libraries(PhysX::Cooking INTERFACE PhysX::Common) # Cooking depends on Common

add_library(PhysX::Extensions STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::Extensions PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXExtensions_static_64.lib"
)
target_link_libraries(PhysX::Extensions INTERFACE PhysX::PhysX) # Extensions depends on PhysX

# Character Controller
add_library(PhysX::CharacterKinematic STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::CharacterKinematic PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXCharacterKinematic_static_64.lib"
)
target_link_libraries(PhysX::CharacterKinematic INTERFACE PhysX::PhysX) # CharacterKinematic likely depends on main PhysX library


# PVD (PhysX Visual Debugger)
add_library(PhysX::PvdSDK STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::PvdSDK PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXPvdSDK_static_64.lib"
)
target_link_libraries(PhysX::PvdSDK INTERFACE PhysX::Foundation) # PVD SDK often depends on Foundation

add_library(PhysX::PVDRuntime STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::PVDRuntime PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PVDRuntime_64.lib"
)
target_link_libraries(PhysX::PVDRuntime INTERFACE PhysX::PvdSDK) # PVD Runtime depends on PVD SDK


# Low-Level / Internal PhysX Components (often implicitly pulled by others, but good to include for completeness)
add_library(PhysX::LowLevel STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::LowLevel PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/LowLevel_static_64.lib"
)
target_link_libraries(PhysX::LowLevel INTERFACE PhysX::Foundation) # LowLevel depends on Foundation

add_library(PhysX::LowLevelAABB STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::LowLevelAABB PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/LowLevelAABB_static_64.lib"
)
target_link_libraries(PhysX::LowLevelAABB INTERFACE PhysX::LowLevel) # LowLevelAABB depends on LowLevel

add_library(PhysX::LowLevelDynamics STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::LowLevelDynamics PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/LowLevelDynamics_static_64.lib"
)
target_link_libraries(PhysX::LowLevelDynamics INTERFACE PhysX::LowLevel) # LowLevelDynamics depends on LowLevel


# GPU / CUDA related PhysX Libraries (for hardware acceleration)
add_library(PhysX::CudaContextManager STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::CudaContextManager PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXCudaContextManager_static_64.lib"
)
target_link_libraries(PhysX::CudaContextManager INTERFACE PhysX::Foundation) # Depends on Foundation (and CUDA SDK if used)

add_library(PhysX::GPU STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::GPU PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXGpu_64.lib"
)
target_link_libraries(PhysX::GPU INTERFACE PhysX::PhysX) # Depends on main PhysX library

add_library(PhysX::ArticulationGpu STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::ArticulationGpu PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXArticulationGpu_static_64.lib"
)
target_link_libraries(PhysX::ArticulationGpu INTERFACE PhysX::GPU) # Depends on PhysX GPU

add_library(PhysX::BroadphaseGpu STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::BroadphaseGpu PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXBroadphaseGpu_static_64.lib"
)
target_link_libraries(PhysX::BroadphaseGpu INTERFACE PhysX::GPU) # Depends on PhysX GPU

add_library(PhysX::SimulationController STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::SimulationController PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/SimulationController_static_64.lib"
)
target_link_libraries(PhysX::SimulationController INTERFACE PhysX::PhysX) # Depends on main PhysX

add_library(PhysX::SimulationControllerGpu STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::SimulationControllerGpu PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXSimulationControllerGpu_static_64.lib"
)
target_link_libraries(PhysX::SimulationControllerGpu INTERFACE PhysX::GPU) # Depends on PhysX GPU

add_library(PhysX::SolverGpu STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::SolverGpu PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXSolverGpu_static_64.lib"
)
target_link_libraries(PhysX::SolverGpu INTERFACE PhysX::GPU) # Depends on PhysX GPU

add_library(PhysX::NarrowphaseGpu STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::NarrowphaseGpu PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXNarrowphaseGpu_static_64.lib"
)
target_link_libraries(PhysX::NarrowphaseGpu INTERFACE PhysX::GPU) # Depends on PhysX GPU


# Other Specific Libraries
add_library(PhysX::SceneQuery STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::SceneQuery PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/SceneQuery_static_64.lib"
)
target_link_libraries(PhysX::SceneQuery INTERFACE PhysX::PhysX) # Depends on main PhysX

add_library(PhysX::Task STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::Task PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXTask_static_64.lib"
)
target_link_libraries(PhysX::Task INTERFACE PhysX::Foundation) # Task often depends on Foundation

add_library(PhysX::Vehicle STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::Vehicle PROPERTIES
    IMPORTED_LOCATION "${THIRD_PARTY_ROOT}/physx/bin/win.x86_64.vc143.md/release/PhysXVehicle2_static_64.lib" # Note: Your file is PhysXVehicle2_static_64.lib
)
target_link_libraries(PhysX::Vehicle INTERFACE PhysX::PhysX) # Vehicle depends on main PhysX


# Finally, create a single interface target that bundles EVERYTHING together
# Any target linking to PhysX::SDK will get all of these.
add_library(PhysX::SDK INTERFACE IMPORTED)
target_link_libraries(PhysX::SDK INTERFACE
    # Core PhysX components
    PhysX::PhysX
    PhysX::Cooking
    PhysX::Extensions

    # Character Controller
    PhysX::CharacterKinematic

    # PVD components
    PhysX::PvdSDK
    PhysX::PVDRuntime

    # Low-Level / Internal components
    PhysX::LowLevel
    PhysX::LowLevelAABB
    PhysX::LowLevelDynamics

    # GPU / CUDA components
    PhysX::CudaContextManager
    PhysX::GPU
    PhysX::ArticulationGpu
    PhysX::BroadphaseGpu
    PhysX::SimulationController
    PhysX::SimulationControllerGpu
    PhysX::SolverGpu
    PhysX::NarrowphaseGpu

    # Other Specific components
    PhysX::SceneQuery
    PhysX::Task
    PhysX::Vehicle
)

# =============================================================================
# ADD OUR OWN ENGINE MODULES
# =============================================================================
add_subdirectory(Engine/Core)
add_subdirectory(Engine/PhysicsEngine)
# add_subdirectory(Engine/ServerEngine)
# add_subdirectory(Engine/ShardEngine)
# add_subdirectory(Engine/GameEngine) # Add later

# =============================================================================
# DEFINE THE FINAL EXECUTABLE
# =============================================================================
add_executable(RiftForgedServer App/main.cpp)

# =============================================================================
# LINK LIBRARIES
# =============================================================================
target_link_libraries(RiftForgedServer PRIVATE
    Core
    PhysicsEngine 
    #ServerEngine
    #ShardEngine
    #GameEngine
)

# =============================================================================
# BUILD SUMMARY
# =============================================================================
message(STATUS "--------------------------------------------------")
message(STATUS "Project:         ${PROJECT_NAME}")
message(STATUS "Build type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "Output binaries: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "--------------------------------------------------")