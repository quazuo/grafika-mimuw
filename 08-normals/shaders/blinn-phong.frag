#version 410

in vec3 position;
in vec2 tex_coords;
in mat3 TBN;

out vec4 out_color;

uniform sampler2D color_texture;
uniform sampler2D normal_texture;
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

uniform DirectionalLight directional_light;
uniform PointLight point_light;

vec3 calc_directional_light() {
    vec3 base_color = texture(color_texture, tex_coords).rgb;
    vec3 normal = texture(normal_texture, tex_coords).rgb;
    normal = normal * 2.0 - 1.0; // [0,1] -> [-1,1]
    normal = TBN * normal; // tangent space -> world space

    vec3 light_direction = normalize(-directional_light.direction);
    vec3 view_direction = normalize(camera_position - position);
    vec3 halfway_direction = normalize(light_direction + view_direction);

    float ambient_factor = 0.03f;
    float diffuse_factor = max(dot(normal, light_direction), 0.0f);
    float specular_factor = pow(max(dot(normal, halfway_direction), 0.0f), 64.0f);

    vec3 ambient = ambient_factor * base_color;
    vec3 diffuse = diffuse_factor * directional_light.color * base_color;
    vec3 specular = specular_factor * directional_light.color;

    return ambient + diffuse + specular;
}

vec3 calc_point_light() {
    vec3 base_color = texture(color_texture, tex_coords).rgb;
    vec3 normal = texture(normal_texture, tex_coords).rgb;
    normal = normal * 2.0 - 1.0; // [0,1] -> [-1,1]
    normal = TBN * normal; // tangent space -> world space

    vec3 light_direction = normalize(point_light.position - position);
    vec3 view_direction = normalize(camera_position - position);
    vec3 halfway_direction = normalize(light_direction + view_direction);

    float distance = length(point_light.position - position);
    float attenuation = 1.0f / (point_light.att_constant
                                + point_light.att_linear * distance
                                + point_light.att_quadratic * distance * distance);

    float ambient_factor = 0.03f;
    float diffuse_factor = max(dot(normal, light_direction), 0.0f);
    float specular_factor = pow(max(dot(normal, halfway_direction), 0.0f), 64.0f);

    vec3 ambient = ambient_factor * base_color;
    vec3 diffuse = diffuse_factor * point_light.color * base_color;
    vec3 specular = specular_factor * point_light.color;

    return attenuation * (ambient + diffuse + specular);
}

void main() {
    vec3 color = calc_directional_light() + calc_point_light();
    out_color = vec4(color, 1.0f);
}
