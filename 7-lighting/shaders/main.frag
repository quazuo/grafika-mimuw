#version 410

in vec2 tex_coords;
in vec3 normal;

out vec4 out_color;

uniform sampler2D color_texture;
uniform vec3 light_direction;

void main() {
    vec3 base_color = texture(color_texture, tex_coords).rgb;

    float diffuse_factor = max(dot(-normal, light_direction), 0.1f);
    vec3 diffuse = diffuse_factor * base_color;

    out_color = vec4(diffuse, 1.0f);
}
