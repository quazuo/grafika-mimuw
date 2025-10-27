#version 410

in vec3 color;
in vec3 normal;

out vec4 out_color;

uniform vec3 light_direction;

void main() {
    float diffuse_factor = max(dot(-normal, light_direction), 0.1f);
    vec3 diffuse = diffuse_factor * color;

    out_color = vec4(diffuse, 1.0f);
}
