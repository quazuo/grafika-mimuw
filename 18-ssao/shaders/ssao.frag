#version 410

#define KERNEL_SIZE 64

in vec2 tex_coords;

out vec4 out_ssao;

uniform vec3 samples[KERNEL_SIZE];
uniform sampler2D sample_noise;
uniform float radius;

uniform mat4 projection;

uniform sampler2D g_position;
uniform sampler2D g_normal;

uniform ivec2 window_size;

void main() {
    vec3 frag_pos = texture(g_position, tex_coords).xyz;
    vec3 normal = normalize(texture(g_normal, tex_coords).xyz);

    if (normal == vec3(0)) discard;

    vec2 noise_scale = vec2(window_size) / 4.0;
    vec3 random_vec = texture(sample_noise, tex_coords * noise_scale).xyz;
    random_vec = normalize(random_vec);

    vec3 tangent = normalize(random_vec - normal * dot(random_vec, normal)); // gramm-schmidt
    vec3 bitangent = cross(normal, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < KERNEL_SIZE; i++) {
        vec3 sample_vec = tbn * samples[i];
        vec3 sample_view_pos = frag_pos + sample_vec * radius;

        vec4 sample_clip_pos = projection * vec4(sample_view_pos, 1.0);
        sample_clip_pos.xyz /= sample_clip_pos.w;
        sample_clip_pos.xyz = sample_clip_pos.xyz * 0.5 + 0.5;

        float sample_depth = texture(g_position, sample_clip_pos.xy).z;

        float range_check = smoothstep(0.0, 1.0, radius / abs(frag_pos.z - sample_depth));

        const float bias = 0.025;
        occlusion += (sample_depth >= sample_view_pos.z + bias) ? range_check : 0.0;
    }

    occlusion /= KERNEL_SIZE;

    out_ssao = vec4(vec3(1.0 - occlusion), 1.0);
}
