#version 410

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_color;
layout (location = 2) in vec3 in_normal;

out vec3 color;
out vec3 normal; // normals are interpolated! this might not be intended (for flat shading)

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 light_direction;

void main() {
    gl_Position = projection * view * model * vec4(in_position, 1.0);
    color = in_color;
    normal = in_normal;
}
