#version 410

out vec4 out_color;
out vec4 out_bloom_color;

uniform vec4 color;
uniform float bloom_threshold;

void main() {
    out_color = color;

    // new! color above a threshold becomes
    float brightness = dot(out_color.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness >= bloom_threshold) {
        out_bloom_color = out_color;
    } else {
        out_bloom_color = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
