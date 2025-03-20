cmake_minimum_required(VERSION 3.12)
project(C_Structures LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(Eigen3 REQUIRED)
find_package(glm REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(GTest REQUIRED)

# Add executable
add_executable(C_Structures
  src/main.cpp
  src/model/Node.cpp
  src/model/Beam.cpp
  src/data/CSVHandler.cpp
  src/physics/Simulator.cpp
  src/ui/UIHandler.cpp  # Added UIHandler
)

# Include directories
target_include_directories(C_Structures PRIVATE
  ${SDL2_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIRS}
  ${GLEW_INCLUDE_DIRS}
  ${GLM_INCLUDE_DIRS}
  include/
)

# Link libraries
target_link_libraries(C_Structures PRIVATE
  SDL2::SDL2
  SDL2::SDL2main
  OpenGL::GL
  GLEW::GLEW
)

# Enable Testing
enable_testing()

# Add test executable
add_executable(tests
    tests/test_main.cpp
    tests/PhysicsTests.cpp
)

# Link GTest and Eigen3 to tests
target_link_libraries(tests PRIVATE
    GTest::GTest
    GTest::Main
    Eigen3::Eigen
)

# Register tests with CTest
add_test(NAME Tests COMMAND tests)

# Add new shaders to install
install(FILES shaders/force_vertex.glsl shaders/force_fragment.glsl
    DESTINATION shaders)

# Register tests with CTest
add_test(NAME Tests COMMAND tests)
