#version 410

in vec2 tex_coords;

out vec4 out_color;

uniform sampler2D colorTexture;

float linearize_depth(float depth) {
    float near = 0.1f;
    float far = 500.0f;

    float ndc = depth * 2.0 - 1.0;
    float linearDepth = (2.0 * near * far) / (far + near - ndc * (far - near));

    return linearDepth;
}

void main() {
    out_color = texture(colorTexture, tex_coords);

    // visualize the depth buffer
    // out_color = vec4(linearize_depth(gl_FragCoord.z)) / 5; // divide by 5 to make it look a bit clearer; this is purely ad-hoc
}
