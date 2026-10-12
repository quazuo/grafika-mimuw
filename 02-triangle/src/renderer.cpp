#include "renderer.hpp"

#include <stdexcept>
#include <vector>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "utilities/debug.hpp"
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

    shaders = std::make_unique<GLGraphicsShaders>(
        "../02-triangle/shaders/main.vert",
        "../02-triangle/shaders/main.frag"
    );

    prepareBuffers();
}

OpenGLRenderer::~OpenGLRenderer() {
    glDeleteBuffers(1, &vertexBufferObject);
    glDeleteVertexArrays(1, &vertexArrayObject);
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
        { .position = { 0.5f, -0.5f, 0.0f}, .color = {1.0f, 0.0f, 0.0f} },
        { .position = {-0.5f, -0.5f, 0.0f}, .color = {0.0f, 1.0f, 0.0f} },
        { .position = { 0.0f,  0.5f, 0.0f}, .color = {0.0f, 0.0f, 1.0f} }
    };

    // Vertex Array Object (VAO) - a "table" managing references to vertex buffer objects
    glGenVertexArrays(1, &vertexArrayObject);
    // we "bind" the VAO we just created, which means "from now on, when we refer to a VAO, it's this one right here"
    glBindVertexArray(vertexArrayObject);

    // Vertex Buffer Object (VBO) - a buffer that lives in GPU memory, containing vertices for the models we'll be drawing to the screen
    glGenBuffers(1, &vertexBufferObject);
    // we "bind" the VBO just like we did with the VAO a few lines before, except now we bind the VBO to the GL_ARRAY_BUFFER "slot".
    // since buffer objects can have more uses than storing vertices, there are multiple bind points we can use.
    // see the table at: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml
    // from now on, when we refer to the GL_ARRAY_BUFFER slot, OpenGL will know that we're talking about the VBO we bound to this slot.
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);

    // upload data from the CPU (the std::vector `vertices`) to this buffer object that lives on the GPU
    //
    // notice that we're not referring to the specific ID of the object; we only refer to `GL_ARRAY_BUFFER`:
    // in the previous line we bound our VBO to the GL_ARRAY_BUFFER slot, so from now on, whenever we refer to this slot,
    // OpenGL knows that we mean the VBO we referred to back then (the one whose ID is in the `vertexBufferObject` variable).
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
        1,                          // index of configured vertex attribute (here: color)
        3,                          // size of configured vertex attribute (again it's a vec3, so 3 components)
        GL_FLOAT,                   // type of data stored in the attribute (float)
        GL_FALSE,                   // do we want the data to be normalized? (no)
        sizeof(Vertex),             // stride = how many bytes between each vertex
        // offset of where the data begins. for legacy reasons, this is a pointer
        reinterpret_cast<void *>(offsetof(Vertex, color))
    );
    glEnableVertexAttribArray(1);
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
