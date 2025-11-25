#version 410

in vec2 tex_coords;

out vec4 out_color;

uniform sampler2D sampled_texture;
uniform float exposure;

void main() {
    const float gamma = 2.2;
    vec3 hdr_color = texture(sampled_texture, tex_coords).rgb;

    // reinhard tone mapping
    vec3 tone_mapped = hdr_color / (hdr_color + vec3(1.0));

    // exposure tone mapping
    tone_mapped = vec3(1.0) - exp(-hdr_color * exposure);

    // gamma correction
    vec3 gamma_corrected = pow(tone_mapped, vec3(1.0 / gamma));

    out_color = vec4(gamma_corrected, 1.0);
}
