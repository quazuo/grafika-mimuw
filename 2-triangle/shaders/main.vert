#version 450

// names here are irrelevant, but we'll prefix them with "in" for clarity
layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_color;

out vec3 color; // this *has* to match the "in vec3 color" in the fragment shader!

void main() {
    gl_Position = vec4(in_position, 1.0); // this is necessary in every vertex shader
    color = in_color; // pass the color to the fragment shader
}
