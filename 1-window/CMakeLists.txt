cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME 1-window)

project(${PROJECT_NAME} LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_TOOLCHAIN_FILE "C:/Users/PCx/Desktop/vcpkg/scripts/buildsystems/vcpkg.cmake")

find_package(OpenGL REQUIRED)

include_directories(
        dependencies/glew/include/
        dependencies/glfw/include/
        dependencies/glm/
        include/
        ../
)

set(ALL_LIBS
        ${OPENGL_LIBRARY}
        glew
        glfw
)

file(GLOB SOURCES
        "src/*"
        "../utilities/*"
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} ${ALL_LIBS})
