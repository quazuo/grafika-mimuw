#include "renderer.hpp"

#include <stdexcept>
#include <sstream>
#include <iostream>
#include <ranges>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <stb_image.h>

#include <tiny_obj_loader.h>

#include "vertex.hpp"

OpenGLRenderer::OpenGLRenderer(const int windowWidth, const int windowHeight) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    windowSize = {windowWidth, windowHeight};
    window     = glfwCreateWindow(windowWidth, windowHeight, "6-loaded", nullptr, nullptr);
    if (!window) {
        const char *desc;
        const int code = glfwGetError(&desc);
        glfwTerminate();
        throw std::runtime_error("Failed to open GLFW window. Error: " + std::to_string(code) + " " + desc);
    }
    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);

    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLEW");
    }

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_DEBUG_OUTPUT);
#ifndef __APPLE__
    glDebugMessageCallback(reinterpret_cast<GLDEBUGPROC>(&debugCallback), nullptr);
#endif

    glfwSetWindowRefreshCallback(window, windowRefreshCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetWindowUserPointer(window, this);

    shaders = std::make_unique<GLShaders>(
        "../6-loaded/shaders/main.vert",
        "../6-loaded/shaders/main.frag"
    );

    camera = std::make_unique<Camera>(window);

    loadMesh();
    prepareBuffers();

    loadTextures();
}

OpenGLRenderer::~OpenGLRenderer() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void OpenGLRenderer::tickInputEvents() {
    camera->tickInputEvents();

    // reload shaders
    static bool wasPressedLastFrame = false;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (!wasPressedLastFrame) {
            shaders = std::make_unique<GLShaders>(
                "../6-loaded/shaders/main.vert",
                "../6-loaded/shaders/main.frag"
            );
        }
        wasPressedLastFrame = true;
    } else {
        wasPressedLastFrame = false;
    }
}

void OpenGLRenderer::startRendering() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderer::render() {
    shaders->enable();

    shaders->setUniform("model", glm::scale(glm::identity<glm::mat4>(), glm::vec3(10.0f)));
    shaders->setUniform("view", camera->getViewMatrix());
    shaders->setUniform("projection", camera->getPerspectiveMatrix());
    shaders->setUniform("colorTexture", 0);

    glDrawElements(GL_TRIANGLES, meshIndices.size(), GL_UNSIGNED_INT, 0);
}

void OpenGLRenderer::finishRendering() const {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void OpenGLRenderer::prepareBuffers() {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * meshVertices.size(), meshVertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * meshIndices.size(), meshIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void *>(offsetof(Vertex, position))
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void *>(offsetof(Vertex, uv))
    );
    glEnableVertexAttribArray(1);
}

void OpenGLRenderer::loadTextures() {
    stbi_set_flip_vertically_on_load(true); // needed as the y-axis (or rather the v coordinate) is flipped

    int width, height, channelCount;
    unsigned char *data = stbi_load("../assets/textures/kettle-albedo.png", &width, &height, &channelCount, 0);
    if (!data) {
        throw std::runtime_error("failed to load texture!");
    }

    glGenTextures(1, &colorTextureID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTextureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
}

void OpenGLRenderer::loadMesh() {
    tinyobj::ObjReaderConfig reader_config{};
    tinyobj::ObjReader reader{};

    if (!reader.ParseFromFile("../assets/meshes/kettle.obj", reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error();
        }

        throw std::runtime_error("failed to load mesh with tinyobjloader");
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto &attrib = reader.GetAttrib();
    auto &shapes = reader.GetShapes();

    std::unordered_map<Vertex, GLuint> vertexToIndexMapping;

    // Loop over shapes (there's only one in kettle.obj)
    for (const auto &shape : shapes) {
        size_t indexOffset = 0;
        size_t nextFreeIndex = 0;

        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            const size_t numFaceVertices = static_cast<size_t>(shape.mesh.num_face_vertices[f]);

            // Loop over vertices in the face.
            for (size_t v = 0; v < numFaceVertices; v++) {
                const tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                const glm::vec3 position = {
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]
                };

                // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                if (idx.texcoord_index < 0) {
                    throw std::runtime_error("no texcoord index in mesh");
                }

                const glm::vec2 uv = {
                    attrib.texcoords[2 * idx.texcoord_index + 0],
                    attrib.texcoords[2 * idx.texcoord_index + 1]
                };

                const Vertex newVertex { position, uv };

                if (vertexToIndexMapping.contains(newVertex)) {
                    meshIndices.emplace_back(vertexToIndexMapping.at(newVertex));
                } else {
                    const GLuint index = nextFreeIndex++;
                    vertexToIndexMapping.emplace(newVertex, index);
                    meshVertices.push_back(newVertex);
                    meshIndices.emplace_back(index);
                }
            }

            indexOffset += numFaceVertices;
        }
    }
}

void OpenGLRenderer::debugCallback(const GLenum source, const GLenum type, const GLuint id, const GLenum severity,
                                   const GLsizei length, const GLchar *message, const void *userParam) {
    std::stringstream ss;
    ss << "[ERROR]\n\tsource: ";

    switch (source) {
        case GL_DEBUG_SOURCE_API:
            ss << "API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            ss << "WINDOW_SYSTEM";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            ss << "SHADER_COMPILER";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            ss << "THIRD_PARTY";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            ss << "APPLICATION";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            ss << "OTHER";
            break;
        default:
            break;
    }

    ss << "\n\ttype: ";

    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            ss << "ERROR";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            ss << "DEPRECATED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            ss << "UNDEFINED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            ss << "PORTABILITY";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            ss << "PERFORMANCE";
            break;
        case GL_DEBUG_TYPE_MARKER:
            ss << "MARKER";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            ss << "PUSH_GROUP";
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            ss << "POP_GROUP";
            break;
        case GL_DEBUG_TYPE_OTHER:
            ss << "OTHER";
            break;
        default:
            break;
    }

    ss << "\n\tid: " << id;
    ss << "\n\tseverity: ";

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            ss << "HIGH";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            ss << "MEDIUM";
            break;
        case GL_DEBUG_SEVERITY_LOW:
            ss << "LOW";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            ss << "NOTIFICATION";
            break;
        default:
            break;
    }

    ss << "\n\tmessage: " << message << "\n";

    std::cout << ss.str();

    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
        throw std::runtime_error(ss.str());
    }
}

void OpenGLRenderer::windowRefreshCallback(GLFWwindow *window) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    OpenGLRenderer *renderer = static_cast<OpenGLRenderer *>(glfwGetWindowUserPointer(window));
    renderer->render();
    glfwSwapBuffers(window);
    glFinish(); // important, this waits until rendering result is actually visible, thus making resizing less ugly
}

void OpenGLRenderer::framebufferSizeCallback(GLFWwindow *window, const int width, const int height) {
    if (width > 0 && height > 0) {
        glViewport(0, 0, width, height);
    }
}
