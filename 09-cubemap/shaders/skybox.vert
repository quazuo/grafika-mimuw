#version 410

layout (location = 0) in vec3 in_position;

out vec3 tex_coords;

uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 pos = projection * view * vec4(in_position, 1.0);
    gl_Position = pos.xyww;
    tex_coords = in_position;
}
