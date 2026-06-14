# wasm/CMakeLists.txt — standalone Emscripten build of the C_Structures solver core.
#
# Self-contained on purpose: configure it with `emcmake` so it never pulls in the
# SDL/OpenGL/ImGui/GTest dependencies of the top-level project. Eigen and glm are
# header-only and fetched here so the wasm build needs no host dev packages.
#
#   emcmake cmake -S wasm -B build-wasm -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-wasm
#   node wasm/harness.mjs build-wasm/solver_core.js
cmake_minimum_required(VERSION 3.14)
project(C_Structures_wasm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR
        "wasm/ must be configured with the Emscripten toolchain: "
        "use `emcmake cmake -S wasm -B build-wasm`.")
endif()

# ── Header-only maths libraries (fetched so no host packages are required) ────
include(FetchContent)
FetchContent_Declare(
    Eigen3
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG        3.4.0
    GIT_SHALLOW    TRUE
)
set(EIGEN_BUILD_DOC      OFF CACHE BOOL "" FORCE)
set(EIGEN_BUILD_TESTING  OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING        OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(Eigen3)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glm)

# ── Solver core: the pure, deterministic subset (no SDL/OpenGL/CSV/JSON) ───────
set(CORE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
add_executable(solver_core
    ${CMAKE_CURRENT_SOURCE_DIR}/SolverBindings.cpp
    ${CORE_ROOT}/src/model/Node.cpp
    ${CORE_ROOT}/src/model/Beam.cpp
    ${CORE_ROOT}/src/physics/Simulator.cpp
    ${CORE_ROOT}/src/physics/FrameElement.cpp
    ${CORE_ROOT}/src/physics/FrameSimulator.cpp
    ${CORE_ROOT}/src/physics/DistributedLoad.cpp
    ${CORE_ROOT}/src/physics/MemberForces.cpp
)

target_include_directories(solver_core PRIVATE ${CORE_ROOT}/include)
target_link_libraries(solver_core PRIVATE Eigen3::Eigen glm::glm)

# ── Emscripten emit options ───────────────────────────────────────────────────
# MODULARIZE gives a factory (createSolverModule) that loads identically from a
# browser (<script> then createSolverModule()) and from Node (require()).
target_link_options(solver_core PRIVATE
    "-lembind"
    "-sMODULARIZE=1"
    "-sEXPORT_NAME=createSolverModule"
    "-sENVIRONMENT=web,node"
    "-sALLOW_MEMORY_GROWTH=1"
)
set_target_properties(solver_core PROPERTIES OUTPUT_NAME "solver_core" SUFFIX ".js")
