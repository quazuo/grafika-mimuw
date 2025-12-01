#ifndef VERTEX_HPP
#define VERTEX_HPP

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include "glm/gtx/hash.hpp"

struct BasicVertex {
    glm::vec3 position;
};

struct BasicTexturedVertex {
    glm::vec3 position;
    glm::vec2 tex_coords;
};

struct MeshVertex {
    glm::vec3 position;
    glm::vec2 tex_coords;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 bitangent;

    bool operator==(const MeshVertex &other) const {
        return position == other.position
            && tex_coords == other.tex_coords
            && normal == other.normal;
    }
};

template <>
struct std::hash<MeshVertex> {
    std::size_t operator()(const MeshVertex& vertex) const noexcept {
        return (hash<glm::vec3>()(vertex.position) >> 1)
             ^ (hash<glm::vec2>()(vertex.tex_coords) << 1)
             ^ (hash<glm::vec2>()(vertex.normal) >> 1);
    }
};

const std::vector<BasicVertex> cubeVertices{
    {{ 1.0f, -1.0f, -1.0f}},
    {{-1.0f, -1.0f, -1.0f}},
    {{ 1.0f,  1.0f, -1.0f}},
    {{-1.0f,  1.0f, -1.0f}},
    {{ 1.0f,  1.0f, -1.0f}},
    {{-1.0f, -1.0f, -1.0f}},

    {{-1.0f, -1.0f,  1.0f}},
    {{ 1.0f, -1.0f,  1.0f}},
    {{ 1.0f,  1.0f,  1.0f}},
    {{ 1.0f,  1.0f,  1.0f}},
    {{-1.0f,  1.0f,  1.0f}},
    {{-1.0f, -1.0f,  1.0f}},

    {{-1.0f,  1.0f,  1.0f}},
    {{-1.0f,  1.0f, -1.0f}},
    {{-1.0f, -1.0f, -1.0f}},
    {{-1.0f, -1.0f, -1.0f}},
    {{-1.0f, -1.0f,  1.0f}},
    {{-1.0f,  1.0f,  1.0f}},

    {{ 1.0f,  1.0f, -1.0f}},
    {{ 1.0f,  1.0f,  1.0f}},
    {{ 1.0f, -1.0f, -1.0f}},
    {{ 1.0f, -1.0f,  1.0f}},
    {{ 1.0f, -1.0f, -1.0f}},
    {{ 1.0f,  1.0f,  1.0f}},

    {{-1.0f, -1.0f, -1.0f}},
    {{ 1.0f, -1.0f, -1.0f}},
    {{ 1.0f, -1.0f,  1.0f}},
    {{ 1.0f, -1.0f,  1.0f}},
    {{-1.0f, -1.0f,  1.0f}},
    {{-1.0f, -1.0f, -1.0f}},

    {{ 1.0f,  1.0f, -1.0f}},
    {{-1.0f,  1.0f, -1.0f}},
    {{ 1.0f,  1.0f,  1.0f}},
    {{-1.0f,  1.0f,  1.0f}},
    {{ 1.0f,  1.0f,  1.0f}},
    {{-1.0f,  1.0f, -1.0f}},
};

const std::vector<BasicTexturedVertex> screenSpaceQuadVertices{
    {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f}},
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},

    {{-1.0f,  1.0f, 0.0f}, {0.0f, 1.0f}},
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f}},
};

#endif //VERTEX_HPP
