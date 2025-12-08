cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME 17_bloom)

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
        "../dependencies/stb/stb_image.cpp"
        "../dependencies/tinyobjloader/tiny_obj_loader.cpp"
        "../dependencies/imgui/*.cpp"
        "../dependencies/imgui/backends/imgui_impl_glfw.cpp"
        "../dependencies/imgui/backends/imgui_impl_opengl3.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} ${ALL_LIBS})
