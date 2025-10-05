#ifndef RENDERER_H
#define RENDERER_H

#include <memory>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "utilities/gl-shader.hpp"

class OpenGLRenderer {
    GLFWwindow *window;

    std::unique_ptr<GLShaders> shaders;

    GLuint vbo;
    GLuint vao;
    GLuint ebo; // element buffer object

public:
    OpenGLRenderer(int windowWidth, int windowHeight);

    ~OpenGLRenderer();

    GLFWwindow *getWindow() const { return window; }

    /**
     * Starts the rendering process.
     * Should be called before any rendering is done.
     */
    void startRendering();

    /**
     * Starts the rendering process.
     * Renders the actual frame.
     */
    void render();

    /**
     * Wraps up the rendering process.
     * Should be called after all rendering in the current tick has been finished.
     */
    void finishRendering() const;

private:
    void prepareBuffers();

    /**
     * Debug callback used by GLFW to notify the user of errors.
     */
    static void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                              const GLchar *message, const void *userParam);

    static void windowRefreshCallback(GLFWwindow *window);

    static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
};

#endif //RENDERER_H
