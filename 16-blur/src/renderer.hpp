#ifndef RENDERER_H
#define RENDERER_H

#include <memory>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

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

    std::unique_ptr<GLGraphicsShaders> mainShaders;
    std::unique_ptr<GLGraphicsShaders> basicColorShaders;
    std::unique_ptr<GLGraphicsShaders> hdrQuadShaders;
    std::unique_ptr<GLGraphicsShaders> skyboxShaders;
    std::unique_ptr<GLComputeShader> oneDimBlurShader;

    std::vector<MeshVertex> loadedMeshVertices;
    std::vector<GLuint> loadedMeshIndices;

    Mesh loadedMesh;
    Mesh cubeMesh;
    Mesh texturedQuadMesh;

    GLuint colorTextureID;
    GLuint normalTextureID;
    GLuint reflectivityTextureID;
    GLuint cubemapTextureID;

    GLuint blurTemporaryTextureID;

    GLuint hdrColorTextureID;
    GLuint hdrDepthTextureID;

    GLuint hdrFramebuffer;
    glm::ivec2 hdrFramebufferSize;

    // camera stuff won't change too much; we're moving it to a separate class to avoid clutter
    std::unique_ptr<Camera> camera;

    bool needRecreateShaders = false;

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

    void createTempBlurTexture();

    void loadMesh();

    void calculateTbnVectors();

    void initHdrFramebuffer();

    static void windowRefreshCallback(GLFWwindow *window);

    static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
};

#endif //RENDERER_H
