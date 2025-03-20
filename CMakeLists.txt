cmake_minimum_required(VERSION 3.12)
project(C_Structures LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Include Eigen and GLM (header-only)
find_package(Eigen3 REQUIRED)
find_package(glm REQUIRED)

# SDL2 and OpenGL
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

# GLEW (for OpenGL extensions)
find_package(GLEW REQUIRED)

# Add executable
add_executable(C_Structures
  src/main.cpp
  src/model/Node.cpp
  src/model/Beam.cpp
  src/data/CSVHandler.cpp
  # Add future files here (e.g., src/model/Node.cpp)
)

# Include directories
target_include_directories(C_Structures PRIVATE
  ${SDL2_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIR}
  ${GLEW_INCLUDE_DIRS}
  include/
)

# Link libraries
find_package(GTest REQUIRED)  # Add this line to find GTest

target_link_libraries(C_Structures PRIVATE
  ${SDL2_LIBRARIES}
  OpenGL::GL
  GLEW::GLEW
  GTest::GTest  # Link GTest to the C_Structures target
)

# Testing (future use)
enable_testing()
add_subdirectory(tests)
