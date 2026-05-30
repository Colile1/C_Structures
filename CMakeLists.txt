cmake_minimum_required(VERSION 3.14)
project(C_Structures LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── System dependencies ────────────────────────────────────────────────────────
find_package(Eigen3 REQUIRED)
find_package(glm REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(GTest REQUIRED)

# ── Dear ImGui (fetched at configure time) ─────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${SDL2_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
)
target_link_libraries(imgui_lib PUBLIC SDL2::SDL2 OpenGL::GL GLEW::GLEW)

# ── Main executable ────────────────────────────────────────────────────────────
add_executable(C_Structures
    src/main.cpp
    src/build_meta.cpp
    src/model/Node.cpp
    src/model/Beam.cpp
    src/data/CSVHandler.cpp
    src/physics/Simulator.cpp
    src/physics/Determinacy.cpp
    src/ui/UIHandler.cpp
    src/ui/ReactionsPanel.cpp
    src/ui/ModelCheckPanel.cpp
    src/visualization/ForceRenderer.cpp
    src/visualization/RendererUtils.cpp
    src/graphics/Camera.cpp
)

target_include_directories(C_Structures PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    include/
    resources/
)

target_link_libraries(C_Structures PRIVATE
    SDL2::SDL2
    SDL2::SDL2main
    OpenGL::GL
    GLEW::GLEW
    Eigen3::Eigen
    imgui_lib
)

# ── Tests ──────────────────────────────────────────────────────────────────────
enable_testing()

add_executable(tests
    tests/test_main.cpp
    tests/ModelTests.cpp
    tests/PhysicsTests.cpp
    tests/CSVHandlerTests.cpp
    tests/IntegrationTests.cpp
    tests/DeterminacyTests.cpp
    src/model/Node.cpp
    src/model/Beam.cpp
    src/physics/Simulator.cpp
    src/physics/Determinacy.cpp
    src/data/CSVHandler.cpp
)

target_include_directories(tests PRIVATE
    ${EIGEN3_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    include/
)

add_test(NAME ModelTests       COMMAND tests --gtest_filter=NodeTest*:BeamTest*)
add_test(NAME PhysicsTests     COMMAND tests --gtest_filter=PhysicsEngine*:PhysicsVerification*:Reactions*)
add_test(NAME CSVTests         COMMAND tests --gtest_filter=CSVHandler*)
add_test(NAME IntegrationTests COMMAND tests --gtest_filter=Integration*)
add_test(NAME DeterminacyTests COMMAND tests --gtest_filter=Determinacy*)

target_link_libraries(tests PRIVATE
    GTest::GTest
    GTest::Main
    Eigen3::Eigen
)
