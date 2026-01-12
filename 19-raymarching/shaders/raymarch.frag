#version 410

in vec2 tex_coords;

out vec4 out_color;

uniform vec3 camera_pos;
uniform mat4 inverse_vp;
uniform float aspect_ratio;

struct Sphere {
    vec3 center;
    float radius;
    vec3 color;
};

struct Cube {
    vec3 center;
    vec3 half_size;
    vec3 color;
};

struct Torus {
    vec3 center;
    float minor_radius;
    float major_radius;
    vec3 color;
};

uniform Sphere sphere;
uniform Cube cube;
uniform Torus torus;

vec3 get_ray_direction() {
    // contibutions of the up and right vectors
    float fov = radians(aspect_ratio);

    vec2 ndc = tex_coords * 2.0 - vec2(1.0); // [0,1] -> [-1,-1]
    vec4 clip = vec4(ndc, -1.0, 1.0);
    vec4 world = inverse_vp * clip;
    vec3 ray_pos = vec3(world) / world.w;
    vec3 ray_direction = normalize(ray_pos - camera_pos);

    return ray_direction;
}

float distance_from_sphere(vec3 point, Sphere sphere) {
    return length(point - sphere.center) - sphere.radius;
}

float distance_from_cube(vec3 point, Cube cube) {
    vec3 point_centered = point - cube.center;
    vec3 d = abs(point_centered) - cube.half_size;
    return length(max(d, vec3(0))) + min(max(d.x, max(d.y, d.z)), 0);
}

float distance_from_torus(vec3 point, Torus torus) {
    vec3 point_centered = point - torus.center;
    vec2 q = vec2(length(point_centered.xz) - torus.major_radius, point_centered.y);
    return length(q) - torus.minor_radius;
}

float sdf_min(float a, float b) {
    return min(a, b);
}

float smooth_min(float a, float b) {
    const float k = 1.0;
    float h = max(k - abs(a - b), 0) / k;
    return min(a, b) - h * h * h * k / 6.0;
}

float distance_from_any(vec3 point) {
    float d_sphere = distance_from_sphere(point, sphere);
    float d_cube = distance_from_cube(point, cube);
    float d_torus = distance_from_torus(point, torus);

    return smooth_min(d_sphere, smooth_min(d_cube, d_torus));
}

vec3 get_color(vec3 point) {
    float dist = distance_from_any(point);

    float d_sphere = distance_from_sphere(point, sphere);
    float d_cube = distance_from_cube(point, cube);
    float d_torus = distance_from_torus(point, torus);

    const float k = 2.0;
    vec3 w = exp(-k * vec3(d_sphere, d_cube, d_torus)); // w == weights
    w /= (w.x + w.y + w.z); // now components sum to 1.0

    return w.x * sphere.color + w.y * cube.color + w.z * torus.color;
}

vec3 get_normal(vec3 point) {
    const float eps = 0.0005;

    return normalize(vec3(
        distance_from_any(point + vec3(eps, 0, 0)) - distance_from_any(point - vec3(eps, 0, 0)),
        distance_from_any(point + vec3(0, eps, 0)) - distance_from_any(point - vec3(0, eps, 0)),
        distance_from_any(point + vec3(0, 0, eps)) - distance_from_any(point - vec3(0, 0, eps))
    ));
}

void main() {
    const uint MAX_STEPS = 256;
    const float MIN_DISTANCE = 0.001;
    const float MAX_DISTANCE = 1000.0;

    const vec3 LIGHT_DIRECTION = normalize(vec3(1.0, -2.0, -3.0));

    vec3 ray_direction = get_ray_direction();

    float current_distance = 0.0;

    for (uint i = 0; i < MAX_STEPS; ++i) {
        vec3 current_position = camera_pos + ray_direction * current_distance;

        float distance_to_closest = distance_from_any(current_position);

        if (distance_to_closest < MIN_DISTANCE) {
            vec3 color = get_color(current_position);
            vec3 normal = get_normal(current_position);

            vec3 shaded_color = color * max(dot(-LIGHT_DIRECTION, normal), 0.1);

            out_color = vec4(shaded_color, 1);
            return;
        }

        if (current_distance > MAX_DISTANCE) {
            break;
        }

        // accumulate the distance traveled thus far
        current_distance += distance_to_closest;
    }

    const vec3 CLEAR_COLOR = vec3(0.0);
    out_color = vec4(CLEAR_COLOR, 1.0);
}
