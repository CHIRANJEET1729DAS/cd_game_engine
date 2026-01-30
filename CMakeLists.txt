cmake_minimum_required(VERSION 3.10)
project(StarWarsGame)

# Add GLFW as a subdirectory
add_subdirectory(libraries/glfw)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Add the executable 'main' with sources main.cpp and src/glad.c
# Note: glad.c and glad.h need to be added to the project if not present
add_executable(main main.cpp shader.cpp glad.c)

# Include directories
target_include_directories(main PRIVATE include libraries/glfw/include .)

# Link the executable with GLFW, OpenGL, and Assimp
target_link_libraries(main glfw OpenGL::GL assimp pthread dl m)

# Set C++ standard
set_target_properties(main PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)

# Post-build command to copy shaders to the build directory every time
add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/shader.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/planet.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/planet.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/sky.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/sky.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/grid.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/grid.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/enigma.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/enigma.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/hud.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/hud.frag
    ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Copying shaders to build directory"
)
