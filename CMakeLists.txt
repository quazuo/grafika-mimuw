cmake_minimum_required (VERSION 3.5)

project (grafika-komputerowa LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES arm64)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

if(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_BINARY_DIR}>)
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

find_package(OpenGL REQUIRED)

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
add_subdirectory(4-icosahedron-moving)
