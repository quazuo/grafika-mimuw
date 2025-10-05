cmake_minimum_required (VERSION 3.5)

project (grafika-komputerowa LANGUAGES CXX C)

set(CMAKE_TOOLCHAIN_FILE "C:/Users/PCx/Desktop/vcpkg/scripts/buildsystems/vcpkg.cmake")

include_directories(
        dependencies/glew/include/
        dependencies/glfw/include/
        dependencies/glm/
        utilities/
        .
)

set(ALL_LIBS
        ${OPENGL_LIBRARY}
        glfw
        GLEW_1130
        ${libnoise}
)

add_definitions(
        -DTW_STATIC
        -DTW_NO_LIB_PRAGMA
        -DTW_NO_DIRECT3D
        -DGLEW_STATIC
        -D_CRT_SECURE_NO_WARNINGS
)

add_subdirectory(dependencies)

add_subdirectory(1-window)
add_subdirectory(2-triangle)
add_subdirectory(3-icosahedron)
