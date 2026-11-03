cmake_minimum_required (VERSION 3.5)

project (grafika-komputerowa LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES arm64)

if(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_BINARY_DIR}>)
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

find_package(OpenGL REQUIRED)

include_directories(
        dependencies/glm/
        .
)

set(ALL_LIBS
        ${OPENGL_LIBRARY}
        glfw
        #GLEW_1130
        glew
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
add_subdirectory(utilities)

function(add_subproject DIR_NAME)
        string(REPLACE "-" "_" TARGET_NAME ${DIR_NAME})
        file(GLOB SOURCES "${DIR_NAME}/src/*")

        add_executable(${TARGET_NAME} ${SOURCES})
        set_target_properties(${TARGET_NAME} PROPERTIES
                CXX_STANDARD 20
                CXX_STANDARD_REQUIRED ON
        )

        target_link_libraries(${TARGET_NAME} PUBLIC
                ${ALL_LIBS}
                utilities
                ${ARGN}
        )
endfunction()

add_subproject(01-window)
add_subproject(02-triangle)
add_subproject(03-icosahedron)
add_subproject(04-icosahedron-moving)
add_subproject(05-textured stb)
add_subproject(06-loaded stb tinyobjloader)
add_subproject(07-lighting stb tinyobjloader)
add_subproject(08-normals stb tinyobjloader)
add_subproject(09-cubemap stb tinyobjloader)
add_subproject(10-blending stb tinyobjloader)
add_subproject(11-framebuffer stb tinyobjloader)
add_subproject(12-stencil stb tinyobjloader)
add_subproject(13-imgui stb tinyobjloader imgui)
add_subproject(14-hdr stb tinyobjloader imgui)
add_subproject(15-compute stb tinyobjloader imgui)
add_subproject(16-blur stb tinyobjloader imgui)
add_subproject(17-bloom stb tinyobjloader imgui)
add_subproject(18-ssao stb tinyobjloader imgui)
add_subproject(19-raymarching stb tinyobjloader imgui)

