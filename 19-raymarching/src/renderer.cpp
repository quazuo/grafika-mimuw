#include "renderer.hpp"

#include <stdexcept>
#include <iostream>
#include <vector>
#include <string>
#include <random>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <stb_image.h>

#include <tiny_obj_loader.h>

#include "utilities/debug.hpp"
#include "vertex.hpp"

OpenGLRenderer::OpenGLRenderer(const int windowWidth, const int windowHeight) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // important! OpenGL 4.3 is needed for compute shaders
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    windowSize = {windowWidth, windowHeight};
    window     = glfwCreateWindow(windowWidth, windowHeight, "19-raymarching", nullptr, nullptr);
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

    glEnable(GL_DEPTH_TEST);

    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    glEnable(GL_DEBUG_OUTPUT);
#ifndef __APPLE__
    glDebugMessageCallback(reinterpret_cast<GLDEBUGPROC>(&debugCallback), nullptr);
#endif

    glfwSetWindowRefreshCallback(window, windowRefreshCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetWindowUserPointer(window, this);

    // ImGui
    {
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init();
    }

    camera = std::make_unique<Camera>(window);

    raymarchShaders = std::make_unique<GLGraphicsShaders>(
        "../19-raymarching/shaders/raymarch.vert",
        "../19-raymarching/shaders/raymarch.frag"
    );

    prepareBuffers();
    createTextures();
}

OpenGLRenderer::~OpenGLRenderer() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    const std::vector usedBuffers {
        texturedQuadMesh.vbo, texturedQuadMesh.ebo,
    };

    const std::vector usedVertexArrays {
        texturedQuadMesh.vao
    };

    glDeleteBuffers(static_cast<GLsizei>(usedBuffers.size()), usedBuffers.data());
    glDeleteVertexArrays(static_cast<GLsizei>(usedVertexArrays.size()), usedVertexArrays.data());

    glfwDestroyWindow(window);
    glfwTerminate();
}

void OpenGLRenderer::tickInputEvents() {
    camera->tickInputEvents();
}

void OpenGLRenderer::startRendering() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if (needRecreateShaders) {
        raymarchShaders = std::make_unique<GLGraphicsShaders>(
            "../19-raymarching/shaders/raymarch.vert",
            "../19-raymarching/shaders/raymarch.frag"
        );

        needRecreateShaders = false;
    }
}

void OpenGLRenderer::render() {
    static float unscaledTime = 0.0f;
    static float time = 0.0f;
    static float timeScale = 1.0f;

    const float deltaTime = static_cast<float>(glfwGetTime()) - unscaledTime;
    time += deltaTime * timeScale;
    unscaledTime = static_cast<float>(glfwGetTime());

    {
        raymarchShaders->enable();
        glBindVertexArray(texturedQuadMesh.vao);

        raymarchShaders->setUniform("inverse_vp", glm::inverse(camera->getPerspectiveMatrix() * camera->getViewMatrix()));
        raymarchShaders->setUniform("aspect_ratio", static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y));

        raymarchShaders->setUniform("camera_pos", camera->getPosition());

        raymarchShaders->setUniform("sphere.center", glm::vec3(0, 0.25 * sin(time + 42.0f), 0));
        raymarchShaders->setUniform("sphere.radius", 1.0f);
        raymarchShaders->setUniform("sphere.color", glm::vec3(1, 0, 0));

        raymarchShaders->setUniform("cube.center", glm::vec3(2 + sin(time), 0, 0));
        raymarchShaders->setUniform("cube.half_size", glm::vec3(0.7f));
        raymarchShaders->setUniform("cube.color", glm::vec3(0, 1, 0));

        raymarchShaders->setUniform("torus.center", glm::vec3(-2 - sin(time), 0, 0));
        raymarchShaders->setUniform("torus.minor_radius", 0.25f);
        raymarchShaders->setUniform("torus.major_radius", 0.5f);
        raymarchShaders->setUniform("torus.color", glm::vec3(0, 0, 1));

        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(screenSpaceQuadVertices.size()));
    }

    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (ImGui::Begin("Renderer settings")) {
            if (ImGui::Button("Reload shaders")) {
                needRecreateShaders = true;
            }

            ImGui::SliderFloat("Time scale", &timeScale, 0.0f, 10.0f);

            const glm::vec3 cameraPos = camera->getPosition();
            ImGui::Text("Camera position: %f %f %f", cameraPos.x, cameraPos.y, cameraPos.z);

            ImGui::End();
        }

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}

void OpenGLRenderer::finishRendering() {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void OpenGLRenderer::prepareBuffers() {
    { // textured quad mesh
        glGenVertexArrays(1, &texturedQuadMesh.vao);
        glBindVertexArray(texturedQuadMesh.vao);

        glGenBuffers(1, &texturedQuadMesh.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, texturedQuadMesh.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(BasicTexturedVertex) * screenSpaceQuadVertices.size(), screenSpaceQuadVertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(BasicTexturedVertex),
            reinterpret_cast<void *>(offsetof(BasicTexturedVertex, position))
        );
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            1,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(BasicTexturedVertex),
            reinterpret_cast<void *>(offsetof(BasicTexturedVertex, tex_coords))
        );
        glEnableVertexAttribArray(1);
    }
}

void OpenGLRenderer::createTextures() {
    stbi_set_flip_vertically_on_load(true);
}

void OpenGLRenderer::windowRefreshCallback(GLFWwindow *window) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    OpenGLRenderer *renderer = static_cast<OpenGLRenderer *>(glfwGetWindowUserPointer(window));
    renderer->render();
    glfwSwapBuffers(window);
    glFinish(); // important, this waits until rendering result is actually visible, thus making resizing less ugly
}

void OpenGLRenderer::framebufferSizeCallback(GLFWwindow *window, const int width, const int height) {
    if (width > 0 && height > 0) {
        glViewport(0, 0, width, height);

        OpenGLRenderer *renderer = static_cast<OpenGLRenderer *>(glfwGetWindowUserPointer(window));
        renderer->windowSize = { width, height };

        renderer->camera->updateAspectRatio();
    }
}
