#version 410

in vec3 tex_coords;

out vec4 out_color;

uniform samplerCube skybox_texture;

void main() {
    out_color = texture(skybox_texture, tex_coords);
}
