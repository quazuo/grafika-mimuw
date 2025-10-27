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

#include "vertex.hpp"

// we'll move to a simple cube for a moment
const std::vector<Vertex> vertices{
//   position               uv
    {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f}},
    {{ 1.0f, -1.0f, -1.0f}, {0.0f, 1.0f}},
    {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f}},
    {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f}},
    {{-1.0f,  1.0f, -1.0f}, {1.0f, 0.0f}},
    {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f}},

    {{-1.0f, -1.0f,  1.0f}, {0.0f, 1.0f}},
    {{ 1.0f, -1.0f,  1.0f}, {1.0f, 1.0f}},
    {{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f}},
    {{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f}},
    {{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
    {{-1.0f, -1.0f,  1.0f}, {0.0f, 1.0f}},

    {{-1.0f,  1.0f,  1.0f}, {1.0f, 0.0f}},
    {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f}},
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f}},
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f}},
    {{-1.0f, -1.0f,  1.0f}, {1.0f, 1.0f}},
    {{-1.0f,  1.0f,  1.0f}, {1.0f, 0.0f}},

    {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
    {{ 1.0f,  1.0f, -1.0f}, {1.0f, 0.0f}},
    {{ 1.0f, -1.0f, -1.0f}, {1.0f, 1.0f}},
    {{ 1.0f, -1.0f, -1.0f}, {1.0f, 1.0f}},
    {{ 1.0f, -1.0f,  1.0f}, {0.0f, 1.0f}},
    {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},

    {{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f}},
    {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f}},
    {{ 1.0f, -1.0f,  1.0f}, {0.0f, 1.0f}},
    {{ 1.0f, -1.0f,  1.0f}, {0.0f, 1.0f}},
    {{-1.0f, -1.0f,  1.0f}, {1.0f, 1.0f}},
    {{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f}},

    {{-1.0f,  1.0f, -1.0f}, {1.0f, 1.0f}},
    {{ 1.0f,  1.0f, -1.0f}, {0.0f, 1.0f}},
    {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
    {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
    {{-1.0f,  1.0f,  1.0f}, {1.0f, 0.0f}},
    {{-1.0f,  1.0f, -1.0f}, {1.0f, 1.0f}},
};

OpenGLRenderer::OpenGLRenderer(const int windowWidth, const int windowHeight) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    windowSize = { windowWidth, windowHeight };
    window = glfwCreateWindow(windowWidth, windowHeight, "5-textured", nullptr, nullptr);
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
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#ifndef __APPLE__
    glDebugMessageCallback(reinterpret_cast<GLDEBUGPROC>(&debugCallback), nullptr);
#endif

    glfwSetWindowRefreshCallback(window, windowRefreshCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetWindowUserPointer(window, this);

    shaders = std::make_unique<GLShaders>(
        "../5-textured/shaders/main.vert",
        "../5-textured/shaders/main.frag"
    );

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
    const glm::vec3 front = {
        std::cos(cameraRotation.y) * std::sin(cameraRotation.x),
        std::sin(cameraRotation.y),
        std::cos(cameraRotation.y) * std::cos(cameraRotation.x)
    };

    const glm::vec3 right = glm::vec3(
        std::sin(cameraRotation.x - 3.14f / 2.0f),
        0,
        std::cos(cameraRotation.x - 3.14f / 2.0f)
    );

    const glm::vec3 up = glm::cross(right, front);

    if (glfwGetKey(window, GLFW_KEY_W)          == GLFW_PRESS) cameraPosition += front * movementSpeed;
    if (glfwGetKey(window, GLFW_KEY_S)          == GLFW_PRESS) cameraPosition -= front * movementSpeed;
    if (glfwGetKey(window, GLFW_KEY_A)          == GLFW_PRESS) cameraPosition -= right * movementSpeed;
    if (glfwGetKey(window, GLFW_KEY_D)          == GLFW_PRESS) cameraPosition += right * movementSpeed;
    if (glfwGetKey(window, GLFW_KEY_SPACE)      == GLFW_PRESS) cameraPosition += up    * movementSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cameraPosition -= up    * movementSpeed;

    if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) cameraRotation.x += rotationSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) cameraRotation.x -= rotationSpeed;
    if (glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS) cameraRotation.y += rotationSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS) cameraRotation.y -= rotationSpeed;

    constexpr float half_pi = glm::pi<float>() / 2.0f;
    constexpr float eps = 0.001f;
    cameraRotation.y = glm::clamp(cameraRotation.y, -half_pi + eps, half_pi - eps);

    // reload shaders
    static bool wasPressedLastFrame = false;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (!wasPressedLastFrame) {
            shaders = std::make_unique<GLShaders>(
                "../5-textured/shaders/main.vert",
                "../5-textured/shaders/main.frag"
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

    shaders->setUniform("model", glm::identity<glm::mat4>());
    shaders->setUniform("view", getViewMatrix());
    shaders->setUniform("projection", glm::perspective(glm::radians(fieldOfView), aspectRatio, zNear, zFar));
    shaders->setUniform("colorTexture", 0);
    shaders->setUniform("colorTexture", 1);

    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

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
    int width, height, channelCount;
    unsigned char *data = stbi_load("../assets/textures/uvtest.png", &width, &height, &channelCount, 0);
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

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
}

glm::mat4 OpenGLRenderer::getViewMatrix() const {
    const glm::vec3 front = {
        std::cos(cameraRotation.y) * std::sin(cameraRotation.x),
        std::sin(cameraRotation.y),
        std::cos(cameraRotation.y) * std::cos(cameraRotation.x)
    };

    return glm::lookAt(cameraPosition, cameraPosition + front, glm::vec3(0, 1, 0));
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
