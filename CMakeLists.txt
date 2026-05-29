cmake_minimum_required(VERSION 3.12)
project(C_Structures LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Eigen3 REQUIRED)
find_package(glm REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(GTest REQUIRED)

add_executable(C_Structures
    src/main.cpp
    src/model/Node.cpp
    src/model/Beam.cpp
    src/data/CSVHandler.cpp
    src/physics/Simulator.cpp
    src/ui/UIHandler.cpp
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
)

target_link_libraries(C_Structures PRIVATE
    SDL2::SDL2
    SDL2::SDL2main
    OpenGL::GL
    GLEW::GLEW
    Eigen3::Eigen
)

enable_testing()

add_executable(tests
    tests/test_main.cpp
    tests/ModelTests.cpp
    tests/PhysicsTests.cpp
    tests/CSVHandlerTests.cpp
    tests/IntegrationTests.cpp
    src/model/Node.cpp
    src/model/Beam.cpp
    src/physics/Simulator.cpp
    src/data/CSVHandler.cpp
)

target_include_directories(tests PRIVATE
    ${EIGEN3_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    include/
)

# Register individual test suites so ctest can filter by name.
add_test(NAME ModelTests     COMMAND tests --gtest_filter=NodeTest*:BeamTest*)
add_test(NAME PhysicsTests   COMMAND tests --gtest_filter=PhysicsEngine*)
add_test(NAME CSVTests       COMMAND tests --gtest_filter=CSVHandler*)
add_test(NAME IntegrationTests COMMAND tests --gtest_filter=Integration*)

target_link_libraries(tests PRIVATE
    GTest::GTest
    GTest::Main
    Eigen3::Eigen
)

install(FILES shaders/force_vertex.glsl shaders/force_fragment.glsl
    DESTINATION shaders)
