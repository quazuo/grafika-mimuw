#include "renderer.hpp"

#include <stdexcept>
#include <sstream>
#include <iostream>
#include <ranges>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "vertex.hpp"

OpenGLRenderer::OpenGLRenderer(const int windowWidth, const int windowHeight) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(windowWidth, windowHeight, "2-triangle", nullptr, nullptr);
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

    glEnable(GL_DEBUG_OUTPUT);
#ifndef __APPLE__
    glDebugMessageCallback(reinterpret_cast<GLDEBUGPROC>(&debugCallback), nullptr);
#endif

    glfwSetWindowRefreshCallback(window, windowRefreshCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetWindowUserPointer(window, this);

    shaders = std::make_unique<GLShaders>(
        "../2-triangle/shaders/main.vert",
        "../2-triangle/shaders/main.frag"
    );

    prepareBuffers();
}

OpenGLRenderer::~OpenGLRenderer() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void OpenGLRenderer::startRendering() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderer::render() {
    shaders->enable();
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // wireframe mode -- try it
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void OpenGLRenderer::finishRendering() const {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void OpenGLRenderer::prepareBuffers() {
    const std::vector<Vertex> vertices {
        { .position = { 0.5f, -0.5f, 1.0f}, .color = {1.0f, 0.0f, 0.0f} },
        { .position = {-0.5f, -0.5f, 0.0f}, .color = {0.0f, 1.0f, 0.0f} },
        { .position = { 0.0f,  0.5f, 0.0f}, .color = {0.0f, 0.0f, 1.0f} }
    };

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(
        0,                          // index of configured vertex attribute (here: position)
        3,                          // size of configured vertex attribute (it's a vec3, so 3 components)
        GL_FLOAT,                   // type of data stored in the attribute (float)
        GL_FALSE,                   // do we want the data to be normalized? (no)
        sizeof(Vertex),             // stride = how many bytes between each vertex
        // offset of where the data begins. for legacy reasons, this is a pointer
        reinterpret_cast<void *>(offsetof(Vertex, position))
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,                      // index of configured vertex attribute (here: color)
        3,                      // size of configured vertex attribute (again it's a vec3, so 3 components)
        GL_FLOAT,               // type of data stored in the attribute (float)
        GL_FALSE,               // do we want the data to be normalized? (no)
        sizeof(Vertex),         // stride = how many bytes between each vertex
        // offset of where the data begins. for legacy reasons, this is a pointer
        reinterpret_cast<void *>(offsetof(Vertex, color))
    );
    glEnableVertexAttribArray(1);
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
    OpenGLRenderer* renderer = static_cast<OpenGLRenderer *>(glfwGetWindowUserPointer(window));
    renderer->render();
    glfwSwapBuffers(window);
    glFinish(); // important, this waits until rendering result is actually visible, thus making resizing less ugly
}

void OpenGLRenderer::framebufferSizeCallback(GLFWwindow *window, const int width, const int height) {
    if (width > 0 && height > 0) {
        glViewport(0, 0, width, height);
    }
}
