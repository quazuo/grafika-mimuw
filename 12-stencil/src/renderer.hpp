#ifndef RENDERER_H
#define RENDERER_H

#include <memory>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "utilities/gl-shader.hpp"
#include "utilities/camera.hpp"
#include "vertex.hpp"

struct Mesh {
    GLuint vbo = 0;
    GLuint vao = 0;
    GLuint ebo = 0;
};

class OpenGLRenderer {
    glm::ivec2 windowSize;
    GLFWwindow *window;

    std::unique_ptr<GLShaders> mainShaders;
    std::unique_ptr<GLShaders> basicColorShaders;
    std::unique_ptr<GLShaders> hdrQuadShaders;
    std::unique_ptr<GLShaders> skyboxShaders;
    std::unique_ptr<GLShaders> outlineShaders;

    std::vector<MeshVertex> loadedMeshVertices;
    std::vector<GLuint> loadedMeshIndices;

    Mesh loadedMesh;
    Mesh cubeMesh;

    GLuint colorTextureID;
    GLuint normalTextureID;
    GLuint reflectivityTextureID;
    GLuint cubemapTextureID;

    // camera stuff won't change too much; we're moving it to a separate class to avoid clutter
    std::unique_ptr<Camera> camera;

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

    void loadMesh();

    void calculateTbnVectors();

    static void windowRefreshCallback(GLFWwindow *window);

    static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
};

#endif //RENDERER_H
