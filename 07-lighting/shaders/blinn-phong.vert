#version 410

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_tex_coords;
layout (location = 2) in vec3 in_normal;

out vec3 position;
out vec2 tex_coords;
out vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 light_direction;

void main() {
    gl_Position = projection * view * model * vec4(in_position, 1.0);
    position = vec3(model * vec4(in_position, 1.0));
    tex_coords = in_tex_coords;

    mat3 normal_matrix = mat3(transpose(inverse(model)));
    normal = normalize(normal_matrix * in_normal);
}
