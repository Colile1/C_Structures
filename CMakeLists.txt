cmake_minimum_required(VERSION 3.12)
project(C_Structures LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Find dependencies
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(glm REQUIRED)
find_package(GLEW REQUIRED)

# Executable
add_executable(C_Structures 
    src/main.cpp
)

# Link libraries
target_include_directories(C_Structures PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
)

target_link_libraries(C_Structures PRIVATE
    ${SDL2_LIBRARIES}
    OpenGL::GL
    GLEW::GLEW
)
