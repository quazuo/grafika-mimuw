#version 410

in vec2 tex_coords;

out vec4 out_color;

uniform sampler2D color_texture;

void main() {
    out_color = texture(color_texture, tex_coords);
}
