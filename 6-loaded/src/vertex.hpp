#ifndef VERTEX_HPP
#define VERTEX_HPP

#include <string>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include "glm/gtx/hash.hpp"

struct Vertex {
    glm::vec3 position;
    glm::vec2 uv;

    bool operator==(const Vertex &other) const {
        return position == other.position
            && uv == other.uv;
    }
};

template <>
struct std::hash<Vertex> {
    std::size_t operator()(const Vertex& vertex) const noexcept {
        return (hash<glm::vec3>()(vertex.position) >> 1) ^
               (hash<glm::vec2>()(vertex.uv) << 1);
    }
};

#endif //VERTEX_HPP
