#ifndef RENDERER_H
#define RENDERER_H

#include <memory>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "utilities/gl-shader.hpp"

class OpenGLRenderer {
    GLFWwindow *window;

    std::unique_ptr<GLGraphicsShaders> shaders;

    // we'll be calling these "vbo" and "vao" in the future programs, but we'll leave the long names for now
    GLuint vbo;
    GLuint vao;

public:
    OpenGLRenderer(int windowWidth, int windowHeight);

    ~OpenGLRenderer();

    GLFWwindow *getWindow() const { return window; }

    /**
     * Starts the rendering process.
     * Should be called at the start of every frame, before any rendering is done.
     */
    void startRendering();

    /**
     * * Renders the current frame.
     */
    void render();

    /**
     * Wraps up the rendering process.
     * Should be called at the end of every frame, after all rendering functions have been called.
     */
    void finishRendering() const;

private:
    void prepareBuffers();

    static void windowRefreshCallback(GLFWwindow *window);

    static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
};

#endif //RENDERER_H
