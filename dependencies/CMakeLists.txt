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

include_directories(
        glfw/include/GLFW/
        glew/include/
)

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

target_link_libraries(glew
        ${OPENGL_LIBRARY}
        ${EXTRA_LIBS}
)
