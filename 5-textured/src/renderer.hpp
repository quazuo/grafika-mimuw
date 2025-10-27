#ifndef RENDERER_H
#define RENDERER_H

#include <memory>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "utilities/gl-shader.hpp"

class OpenGLRenderer {
    glm::ivec2 windowSize;
    GLFWwindow *window;

    std::unique_ptr<GLShaders> shaders;

    GLuint vbo;
    GLuint vao;
    // GLuint ebo; // won't be using indexing for a moment; will bring it back in the next program

    GLuint colorTextureID;

    bool is_free_camera = false;
    glm::vec3 cameraPosition { 0, 0, -3 };
    glm::vec2 cameraRotation { 0, 0 };

    float aspectRatio = 4.0f / 3.0f;
    float fieldOfView = 80.0f;
    float zNear = 0.1f;
    float zFar = 500.0f;

    float movementSpeed = 0.01f;
    float rotationSpeed = 0.006f;

public:
    OpenGLRenderer(int windowWidth, int windowHeight);

    ~OpenGLRenderer();

    GLFWwindow *getWindow() const { return window; }

    /**
     * Processes all pending input events, e.g. to move and rotate the camera.
     */
    void tickInputEvents();

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

    void loadTextures();

    glm::mat4 getViewMatrix() const;

    /**
     * Debug callback used by GLFW to notify the user of errors.
     */
    static void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                              const GLchar *message, const void *userParam);

    static void windowRefreshCallback(GLFWwindow *window);

    static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
};

#endif //RENDERER_H
