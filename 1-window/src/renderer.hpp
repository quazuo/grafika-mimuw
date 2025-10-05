#ifndef RENDERER_H
#define RENDERER_H

#include "GL/glew.h"
#include "GLFW/glfw3.h"

class OpenGLRenderer {
    GLFWwindow *window;

public:
    OpenGLRenderer(int windowWidth, int windowHeight);

    ~OpenGLRenderer();

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

    [[nodiscard]]
    GLFWwindow *getWindow() const { return window; }

private:
    /**
     * Debug callback used by GLFW to notify the user of errors.
     */
    static void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                              const GLchar *message, const void *userParam);

    /**
     * Function called by GLFW whenever the window is refreshed.
     */
    static void windowRefreshCallback(GLFWwindow *window);

    static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
};

#endif //RENDERER_H
