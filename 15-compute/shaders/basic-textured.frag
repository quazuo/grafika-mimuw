#version 410

in vec2 tex_coords;

out vec4 out_color;

uniform sampler2D sampled_texture;

void main() {
    out_color = vec4(texture(sampled_texture, tex_coords).r == 0 ? 0 : 1);
}
