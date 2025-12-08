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
    window     = glfwCreateWindow(windowWidth, windowHeight, "15-compute", nullptr, nullptr);
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

    basicTexturedShaders = std::make_unique<GLGraphicsShaders>(
        "../15-compute/shaders/basic-textured.vert",
        "../15-compute/shaders/basic-textured.frag"
    );
    gameOfLifeShader = std::make_unique<GLComputeShader>(
        "../15-compute/shaders/game-of-life.comp"
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

    const std::vector usedTextures {
        gameOfLifeTextureIDs[0],
        gameOfLifeTextureIDs[1],
    };

    glDeleteBuffers(static_cast<GLsizei>(usedBuffers.size()), usedBuffers.data());
    glDeleteVertexArrays(static_cast<GLsizei>(usedVertexArrays.size()), usedVertexArrays.data());
    glDeleteTextures(static_cast<GLsizei>(usedTextures.size()), usedTextures.data());

    glfwDestroyWindow(window);
    glfwTerminate();
}

void OpenGLRenderer::tickInputEvents() {
    // nothing here this time
}

void OpenGLRenderer::startRendering() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if (needRecreateShaders) {
        basicTexturedShaders = std::make_unique<GLGraphicsShaders>(
            "../15-compute/shaders/basic-textured.vert",
            "../15-compute/shaders/basic-textured.frag"
        );
        gameOfLifeShader = std::make_unique<GLComputeShader>(
            "../15-compute/shaders/game-of-life.comp"
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

    const GLint readTextureIdx = static_cast<size_t>(timeScale * time) % 2;
    const GLint writtenTextureIdx = 1 - readTextureIdx;

    {
        gameOfLifeShader->enable();

        gameOfLifeShader->setUniform("curr_state", readTextureIdx);
        gameOfLifeShader->setUniform("next_state", writtenTextureIdx);

        glDispatchCompute(textureSize.x / 16, textureSize.y / 16, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    {
        basicTexturedShaders->enable();
        glBindVertexArray(texturedQuadMesh.vao);

        basicTexturedShaders->setUniform("model", glm::identity<glm::mat4>());
        basicTexturedShaders->setUniform("view", glm::identity<glm::mat4>());
        basicTexturedShaders->setUniform("projection", glm::identity<glm::mat4>());

        basicTexturedShaders->setUniform("sampled_texture", readTextureIdx);

        glDrawArrays(GL_TRIANGLES, 0, screenSpaceQuadVertices.size());
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

    for (size_t i = 0; i < TEXTURES_COUNT; ++i) {
        glGenTextures(1, &gameOfLifeTextureIDs[i]);
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, gameOfLifeTextureIDs[i]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, 1);

        std::vector<std::uint8_t> textureData(textureSize.x * textureSize.y, 0);
        for (int x = 0; x < textureSize.x; ++x) {
            for (int y = 0; y < textureSize.y; ++y) {
                textureData[textureSize.x * y + x] = dist(rng);
            }
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, textureSize.x, textureSize.y,
                     0, GL_RED, GL_UNSIGNED_BYTE, textureData.data());

        glBindImageTexture(i, gameOfLifeTextureIDs[i], 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8);
    }
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
        // glViewport(0, 0, width, height);
        //
        // OpenGLRenderer *renderer = static_cast<OpenGLRenderer *>(glfwGetWindowUserPointer(window));
        // renderer->windowSize = { width, height };
    }
}
