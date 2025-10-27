#version 410

in vec2 tex_coords;

out vec4 out_color;

uniform sampler2D colorTexture;

void main() {
    out_color = texture(colorTexture, tex_coords);
}
