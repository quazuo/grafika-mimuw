#include "renderer.hpp"

#include <stdexcept>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "utilities/debug.hpp"

OpenGLRenderer::OpenGLRenderer(const int windowWidth, const int windowHeight) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // hide the "old stuff" -- i.e. the immediate mode functions
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // enable the "debug context" -- this will enable OpenGL to give us nice messages when errors occur
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // open a window and create its OpenGL context
    window = glfwCreateWindow(windowWidth, windowHeight, "1-window", nullptr, nullptr);
    if (!window) {
        const char *desc;
        const int code = glfwGetError(&desc);
        glfwTerminate();
        throw std::runtime_error("Failed to open GLFW window. Error: " + std::to_string(code) + " " + desc);
    }

    // the OpenGL context for each window can be "current" on only one thread at a time;
    // make it current on this thread
    glfwMakeContextCurrent(window);

    // enable VSync (explained further inside the `finishRendering()` function)
    glfwSwapInterval(1);

    // initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLEW");
    }

    // ensure we can capture keys being pressed
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // set the color of an empty window to black
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // enable debug logging using our pre-defined callback
    glEnable(GL_DEBUG_OUTPUT);
#ifndef __APPLE__
    glDebugMessageCallback(reinterpret_cast<GLDEBUGPROC>(&debugCallback), nullptr);
#endif

    // set callbacks for refreshing and resizing the window
    glfwSetWindowRefreshCallback(window, windowRefreshCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // attach a pointer to this render to the window
    // this will allow the above callbacks to access the renderer
    glfwSetWindowUserPointer(window, this);
}

OpenGLRenderer::~OpenGLRenderer() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void OpenGLRenderer::startRendering() {
    // clear the window, more on the arguments in the future
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderer::render() {
    // nothing here yet! just an empty window for now
}

void OpenGLRenderer::finishRendering() const {
    // swap the front and back buffers of our current window.
    // - the "front buffer" is the part of memory that contains what is visible in the window at the current point in time.
    // - the "back buffer" is the part of memory that isn't seen on the screen. this is the memory we end up writing to
    //   when we render images using OpenGL.
    //
    // because we called `glfwSwapInterval(1)` in the constructor, this function also waits until the next screen refresh
    // before swapping the buffers. this eliminates screen tearing, which could occur if we swapped the front and back
    // buffers immediately without waiting for the screen to finish scanning the front buffer onto the screen.
    glfwSwapBuffers(window);

    // process any pending "events" for the window: resizing, keyboard inputs, mouse movement, etc...
    // this calls user registered callbacks when applicable -- so the two callbacks we set in the constructor
    glfwPollEvents();
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
