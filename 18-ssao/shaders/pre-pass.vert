#version 410

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_tex_coords;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec3 in_tangent;
layout (location = 4) in vec3 in_bitangent;

out vec3 position;
out vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(in_position, 1.0);
    position = vec3(view * model * vec4(in_position, 1.0));
    normal = in_normal;
}
