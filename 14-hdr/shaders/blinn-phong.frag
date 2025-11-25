#version 410

in vec3 position;
in vec2 tex_coords;
in mat3 TBN;

out vec4 out_color;

uniform sampler2D color_texture;
uniform sampler2D normal_texture;
uniform sampler2D reflectivity_texture;
uniform samplerCube skybox_texture;
uniform vec3 camera_position;

struct DirectionalLight {
    vec3 direction;
    vec3 color;
};

struct PointLight {
    vec3 position;
    vec3 color;

    // attenuation knobs
    float att_constant;
    float att_linear;
    float att_quadratic;
};

#define POINT_LIGHT_COUNT 2

uniform DirectionalLight directional_light;
uniform PointLight point_lights[POINT_LIGHT_COUNT];

vec2 calc_directional_light() {
    vec3 normal = texture(normal_texture, tex_coords).rgb;
    normal = normal * 2.0 - 1.0; // [0,1] -> [-1,1]
    normal = TBN * normal; // tangent space -> world space

    vec3 light_direction = normalize(-directional_light.direction);
    vec3 view_direction = normalize(camera_position - position);
    vec3 halfway_direction = normalize(light_direction + view_direction);

    float diffuse_factor = max(dot(normal, light_direction), 0.0f);
    float specular_factor = pow(max(dot(normal, halfway_direction), 0.0f), 64.0f);

    return vec2(diffuse_factor, specular_factor);
}

vec2 calc_point_light(uint index) {
    vec3 normal = texture(normal_texture, tex_coords).rgb;
    normal = normal * 2.0 - 1.0; // [0,1] -> [-1,1]
    normal = TBN * normal; // tangent space -> world space

    vec3 light_direction = normalize(point_lights[index].position - position);
    vec3 view_direction = normalize(camera_position - position);
    vec3 halfway_direction = normalize(light_direction + view_direction);

    float distance = length(point_lights[index].position - position);
    float attenuation = 1.0f / (point_lights[index].att_constant
    + point_lights[index].att_linear * distance
    + point_lights[index].att_quadratic * distance * distance);

    float diffuse_factor = max(dot(normal, light_direction), 0.0f);
    float specular_factor = pow(max(dot(normal, halfway_direction), 0.0f), 64.0f);

    return attenuation * vec2(diffuse_factor, specular_factor);
}

vec3 calc_reflected_color() {
    vec3 view_direction = normalize(camera_position - position);

    vec3 normal = texture(normal_texture, tex_coords).rgb;
    normal = normal * 2.0 - 1.0; // [0,1] -> [-1,1]
    normal = TBN * normal; // tangent space -> world space

    vec3 reflected_color = texture(skybox_texture, -reflect(view_direction, normal)).rgb;
    return reflected_color;
}

void main() {
    vec3 base_color = texture(color_texture, tex_coords).rgb;

    const float ambient_factor = 0.03f;

    vec2 directional_light_factors = calc_directional_light();

    vec3 ambient  = ambient_factor * base_color;
    vec3 diffuse  = directional_light_factors.x * directional_light.color;
    vec3 specular = directional_light_factors.y * directional_light.color;

    for (uint point_light_index = 0; point_light_index < POINT_LIGHT_COUNT; point_light_index++) {
        vec2 point_light_factors = calc_point_light(point_light_index);
        diffuse  += point_light_factors.x * point_lights[point_light_index].color;
        specular += point_light_factors.y * point_lights[point_light_index].color;
    }

    diffuse *= base_color;

    float reflectivity = texture(reflectivity_texture, tex_coords).r;
    vec3 reflected_color = calc_reflected_color();

    vec3 color = ambient + mix(diffuse, reflected_color, reflectivity) + specular;
    out_color = vec4(color, 1.0f);
}
