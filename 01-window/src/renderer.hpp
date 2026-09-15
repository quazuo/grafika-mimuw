#ifndef RENDERER_H
#define RENDERER_H

#include "GL/glew.h" // include this *before* GLFW
#include "GLFW/glfw3.h"

class OpenGLRenderer {
    GLFWwindow *window;

public:
    OpenGLRenderer(int windowWidth, int windowHeight);

    ~OpenGLRenderer();

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

    [[nodiscard]]
    GLFWwindow *getWindow() const { return window; }

private:
    /**
     * Function called by GLFW whenever the window is refreshed.
     *
     * This can mean different things on different platforms, but a "refresh" mostly refers
     * to resizing or moving the window. This callback is fired whenever the windowing system
     * decides that the window's contents should be redrawn.
     */
    static void windowRefreshCallback(GLFWwindow *window);

    /**
     * Function called by GLFW whenever the window "framebuffer" changes its size.
     *
     * The term "framebuffer" will be explained much further in the class, so for now
     * just consider this function to be called whenever the window is resized.
     */
    static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
};

#endif //RENDERER_H
