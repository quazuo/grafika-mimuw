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
    mat4 model_view = view * model;
    vec4 view_pos = model_view * vec4(in_position, 1.0);

    gl_Position = projection * view_pos;
    position = view_pos.xyz;
    normal = mat3(transpose(inverse(model_view))) * in_normal;
}
