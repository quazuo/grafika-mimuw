#version 410

in vec3 tex_coords;

out vec4 out_color;

uniform samplerCube cubemap_texture;

void main() {
    out_color = texture(cubemap_texture, tex_coords);
}
