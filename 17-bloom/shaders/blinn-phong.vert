#version 410

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_tex_coords;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec3 in_tangent;
layout (location = 4) in vec3 in_bitangent;

out vec3 position;
out vec2 tex_coords;
out mat3 TBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 light_direction;

void main() {
    gl_Position = projection * view * model * vec4(in_position, 1.0);
    position = vec3(model * vec4(in_position, 1.0));
    tex_coords = in_tex_coords;

    mat3 normal_matrix = mat3(transpose(inverse(model)));
    vec3 T = normalize(normal_matrix * in_tangent);
    vec3 B = normalize(normal_matrix * in_bitangent);
    vec3 N = normalize(normal_matrix * in_normal);
    TBN = mat3(T, B, N);
}
