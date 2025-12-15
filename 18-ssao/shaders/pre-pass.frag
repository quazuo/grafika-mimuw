#version 410

in vec3 position;
in vec3 normal;

out vec4 out_position;
out vec4 out_normal;

void main() {
    out_position = vec4(position, 1.0);
    out_normal = vec4(normalize(normal), 1.0); // interpolation can cause `normal` to not be normalized; re-normalize it
}
