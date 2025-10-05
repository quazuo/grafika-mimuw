#version 450

in vec3 color; // this *has* to match the "out vec3 color" in the vertex shader!

out vec4 out_color; // this is the final color which will land on the screen

void main() {
    out_color = vec4(color, 1.0f);
}
