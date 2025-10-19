cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME 1_window)

project(${PROJECT_NAME} LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 20)

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
    
        target_compile_options(${PROJECT_NAME} PRIVATE -std=c++17 -stdlib=libc++)
        target_link_options(${PROJECT_NAME} PRIVATE -stdlib=libc++)

target_link_libraries(${PROJECT_NAME} ${ALL_LIBS})
