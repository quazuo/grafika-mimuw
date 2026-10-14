add_library(utilities STATIC
	camera.cpp
	debug.cpp
	gl-shader.cpp
)

target_include_directories(utilities PUBLIC
	.
)
