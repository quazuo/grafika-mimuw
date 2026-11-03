if(MSVC AND NOT "${MSVC_VERSION}" LESS 1400)
    add_definitions( "/MP" )
endif()

add_definitions(
        -DTW_STATIC
        -DTW_NO_LIB_PRAGMA
        -DTW_NO_DIRECT3D
        -DGLEW_STATIC
        -D_CRT_SECURE_NO_WARNINGS
)

### GLFW ###

add_subdirectory(glfw)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(OPENGL_LIBRARY
            ${OPENGL_LIBRARY}
            -lGL -lGLU -lXrandr -lXext -lX11 -lrt
            ${CMAKE_DL_LIBS}
            ${GLFW_LIBRARIES}
    )
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

### GLEW ###

set(GLEW_SOURCE
        glew/src/glew.c
        glew/src/glewinfo.c
        glew/src/visualinfo.c
)

set(GLEW_HEADERS)

add_library(glew STATIC
        ${GLEW_SOURCE}
        ${GLEW_INCLUDE}
)

target_include_directories(glew PUBLIC
	glew/include/
)

target_link_libraries(glew
        ${OPENGL_LIBRARY}
        ${EXTRA_LIBS}
)

### IMGUI ###

add_library(imgui STATIC
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_glfw.cpp
        imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
        imgui
        imgui/backends
)

### STB ###

add_library(stb STATIC
        stb/stb_image.cpp
)

target_include_directories(stb PUBLIC
        stb
)

### TinyObjLoader ###

add_library(tinyobjloader STATIC
        tinyobjloader/tiny_obj_loader.cpp
)

target_include_directories(tinyobjloader PUBLIC
        tinyobjloader
)
