#include "renderer.hpp"

#include <array>
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    windowSize = {windowWidth, windowHeight};
    window     = glfwCreateWindow(windowWidth, windowHeight, "18-ssao", nullptr, nullptr);
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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init();
    }

    mainShaders = std::make_unique<GLGraphicsShaders>(
        "../18-ssao/shaders/blinn-phong.vert",
        "../18-ssao/shaders/blinn-phong.frag"
    );
    basicColorShaders = std::make_unique<GLGraphicsShaders>(
        "../18-ssao/shaders/basic-color.vert",
        "../18-ssao/shaders/basic-color.frag"
    );
    hdrQuadShaders = std::make_unique<GLGraphicsShaders>(
        "../18-ssao/shaders/final-combine-hdr.vert",
        "../18-ssao/shaders/final-combine-hdr.frag"
    );
    skyboxShaders = std::make_unique<GLGraphicsShaders>(
        "../18-ssao/shaders/skybox.vert",
        "../18-ssao/shaders/skybox.frag"
    );
    prepassShaders = std::make_unique<GLGraphicsShaders>(
        "../18-ssao/shaders/pre-pass.vert",
        "../18-ssao/shaders/pre-pass.frag"
    );
    ssaoShaders = std::make_unique<GLGraphicsShaders>(
        "../18-ssao/shaders/ssao.vert",
        "../18-ssao/shaders/ssao.frag"
    );
    oneDimBlurShader = std::make_unique<GLComputeShader>(
        "../18-ssao/shaders/1d-gaussian-blur.comp"
    );

    camera = std::make_unique<Camera>(window);

    loadMesh();
    calculateTbnVectors();
    prepareBuffers();

    loadTextures();
    createTempBlurTexture();
    createSsaoResources();

    initHdrFramebuffer();
    initPrepassFramebuffer();
    initSsaoFramebuffer();
}

OpenGLRenderer::~OpenGLRenderer() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    const std::vector usedBuffers {
        loadedMesh.vbo,         loadedMesh.ebo,
        cubeMesh.vbo,           cubeMesh.ebo,
        texturedQuadMesh.vbo,   texturedQuadMesh.ebo,
    };

    const std::vector usedVertexArrays {
        loadedMesh.vao,
        cubeMesh.vao,
        texturedQuadMesh.vao,
    };

    const std::vector usedTextures {
        colorTextureID,
        normalTextureID,
        reflectivityTextureID,
        cubemapTextureID,

        prepassPositionTextureID,
        prepassNormalTextureID,
        prepassDepthTextureID,

        blurTemporaryTextureID,

        ssaoTextureID,
        ssaoSampleNoiseTextureID,

        hdrColorTextureID,
        hdrDepthTextureID,
    };

    const std::vector usedFramebuffers {
        hdrFramebuffer,
        ssaoFramebuffer,
        prepassFramebuffer,
    };

    glDeleteBuffers(static_cast<GLsizei>(usedBuffers.size()), usedBuffers.data());
    glDeleteVertexArrays(static_cast<GLsizei>(usedVertexArrays.size()), usedVertexArrays.data());
    glDeleteTextures(static_cast<GLsizei>(usedTextures.size()), usedTextures.data());
    glDeleteFramebuffers(static_cast<GLsizei>(usedFramebuffers.size()), usedFramebuffers.data());

    glfwDestroyWindow(window);
    glfwTerminate();
}

void OpenGLRenderer::tickInputEvents() {
    camera->tickInputEvents();
}

void OpenGLRenderer::startRendering() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if (needRecreateShaders) {
        mainShaders = std::make_unique<GLGraphicsShaders>(
                "../18-ssao/shaders/blinn-phong.vert",
                "../18-ssao/shaders/blinn-phong.frag"
            );
        basicColorShaders = std::make_unique<GLGraphicsShaders>(
            "../18-ssao/shaders/basic-color.vert",
            "../18-ssao/shaders/basic-color.frag"
        );
        hdrQuadShaders = std::make_unique<GLGraphicsShaders>(
            "../18-ssao/shaders/final-combine-hdr.vert",
            "../18-ssao/shaders/final-combine-hdr.frag"
        );
        skyboxShaders = std::make_unique<GLGraphicsShaders>(
            "../18-ssao/shaders/skybox.vert",
            "../18-ssao/shaders/skybox.frag"
        );
        prepassShaders = std::make_unique<GLGraphicsShaders>(
            "../18-ssao/shaders/pre-pass.vert",
            "../18-ssao/shaders/pre-pass.frag"
        );
        ssaoShaders = std::make_unique<GLGraphicsShaders>(
            "../18-ssao/shaders/ssao.vert",
            "../18-ssao/shaders/ssao.frag"
        );
        oneDimBlurShader = std::make_unique<GLComputeShader>(
            "../18-ssao/shaders/1d-gaussian-blur.comp"
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

    constexpr float lightCubeScale = 0.05f;
    constexpr float lightOrbitRadius = 3.0f;

    const std::vector<glm::vec3> lightCubePositions {
        lightOrbitRadius * glm::vec3 {
            glm::sin(time),
            0.0f,
            glm::cos(time)
        },
        lightOrbitRadius * glm::vec3 {
            0.0f,
            glm::sin(time),
            glm::cos(time)
        },
    };

    static std::vector<float> pointLightIntensities {
        15.0f,
        8.0f,
    };

    static std::vector<glm::vec4> pointLightColors {
        glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), // red
        glm::vec4(1.0f, 0.0f, 1.0f, 1.0f), // purple
    };

    static float directionalLightIntensity = 1.0f;
    static glm::vec4 directionalLightColor = glm::vec4(1, 0.9, 0.8, 1.0f);
    const glm::vec3 directionalLightDirection = glm::normalize(glm::vec3(-1.0f, -2.0f, -3.0f));

    constexpr float meshScale = 2.0f;
    static float outlineWidth = 0.2f;
    static float exposure = 0.5f;
    static int blurRadius = 3;
    static bool isViewingDebugTexture = false;
    static TexSlots debugTextureID = TexSlots::PREPASS_POS;
    static float ssaoSampleRadius = 0.1f;
    static bool isSsaoEnabled = true;

    // first, the prepass

    glBindFramebuffer(GL_FRAMEBUFFER, prepassFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    {
        glBindVertexArray(loadedMesh.vao);
        prepassShaders->enable();

        prepassShaders->setUniform("model", glm::scale(glm::identity<glm::mat4>(), glm::vec3(meshScale)));
        prepassShaders->setUniform("view", camera->getViewMatrix());
        prepassShaders->setUniform("projection", camera->getPerspectiveMatrix());

        glDrawElements(GL_TRIANGLES, loadedMeshIndices.size(), GL_UNSIGNED_INT, 0); // just draw the object
    }

    // second, the SSAO pass

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    {
        glDisable(GL_CULL_FACE);
        glBindVertexArray(texturedQuadMesh.vao);
        ssaoShaders->enable();

        for (size_t i = 0; i < 64; i++) {
            ssaoShaders->setUniform("samples[" + std::to_string(i) + "]", ssaoSamples[i]);
        }
        ssaoShaders->setUniform("sample_noise", TexSlots::SSAO_NOISE);
        ssaoShaders->setUniform("radius", ssaoSampleRadius);

        ssaoShaders->setUniform("projection", camera->getPerspectiveMatrix());

        ssaoShaders->setUniform("g_position", TexSlots::PREPASS_POS);
        ssaoShaders->setUniform("g_normal",   TexSlots::PREPASS_NORM);

        ssaoShaders->setUniform("window_size", windowSize);

        glDrawArrays(GL_TRIANGLES, 0, screenSpaceQuadVertices.size());
        glEnable(GL_CULL_FACE);
    }

    // blur the SSAO texture a bit so that it isn't too rough

    {
        oneDimBlurShader->enable();

        oneDimBlurShader->setUniform("blur_radius", blurRadius);

        oneDimBlurShader->setUniform("is_horizontal", true);
        glBindImageTexture(0, ssaoTextureID,          0, GL_FALSE, 0, GL_READ_ONLY,  GL_RGBA16F);
        glBindImageTexture(1, blurTemporaryTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        glDispatchCompute(windowSize.x / 16, windowSize.y / 16, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        oneDimBlurShader->setUniform("is_horizontal", false);
        glBindImageTexture(0, blurTemporaryTextureID, 0, GL_FALSE, 0, GL_READ_ONLY,  GL_RGBA16F);
        glBindImageTexture(1, ssaoTextureID,          0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        glDispatchCompute(windowSize.x / 16, windowSize.y / 16, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    // next rendering calls will write to the separate HDR framebuffer

    glBindFramebuffer(GL_FRAMEBUFFER, hdrFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render the main mesh

    {
        glBindVertexArray(loadedMesh.vao);
        mainShaders->enable();

        mainShaders->setUniform("model", glm::scale(glm::identity<glm::mat4>(), glm::vec3(meshScale)));
        mainShaders->setUniform("view", camera->getViewMatrix());
        mainShaders->setUniform("projection", camera->getPerspectiveMatrix());

        mainShaders->setUniform("color_texture", TexSlots::BASE_COLOR);
        mainShaders->setUniform("normal_texture", TexSlots::NORMAL);
        mainShaders->setUniform("reflectivity_texture", TexSlots::REFLECTIVITY);
        mainShaders->setUniform("skybox_texture", TexSlots::CUBEMAP);
        mainShaders->setUniform("ssao_texture", TexSlots::SSAO);

        mainShaders->setUniform("camera_position", camera->getPosition());
        mainShaders->setUniform("window_size", windowSize);
        mainShaders->setUniform("use_ssao", isSsaoEnabled ? 1 : 0);

        mainShaders->setUniform("directional_light.direction", directionalLightDirection);
        mainShaders->setUniform("directional_light.color", directionalLightIntensity * glm::vec3(directionalLightColor));

        mainShaders->setUniform("point_lights[0].position", lightCubePositions[0]);
        mainShaders->setUniform("point_lights[0].color", pointLightIntensities[0] * glm::vec3(pointLightColors[0]));
        mainShaders->setUniform("point_lights[0].att_constant", 1.0f);
        mainShaders->setUniform("point_lights[0].att_linear", 0.22f);
        mainShaders->setUniform("point_lights[0].att_quadratic", 0.2f);

        mainShaders->setUniform("point_lights[1].position", lightCubePositions[1]);
        mainShaders->setUniform("point_lights[1].color", pointLightIntensities[1] * glm::vec3(pointLightColors[1]));
        mainShaders->setUniform("point_lights[1].att_constant", 1.0f);
        mainShaders->setUniform("point_lights[1].att_linear", 0.09f);
        mainShaders->setUniform("point_lights[1].att_quadratic", 0.032f);

        glDrawElements(GL_TRIANGLES, loadedMeshIndices.size(), GL_UNSIGNED_INT, 0); // just draw the object
    }

    // render cubes representing light sources

    {
        glBindVertexArray(cubeMesh.vao);
        basicColorShaders->enable();

        basicColorShaders->setUniform("model", glm::translate(glm::identity<glm::mat4>(), lightCubePositions[0])
                                              * glm::scale(glm::identity<glm::mat4>(), glm::vec3(lightCubeScale)));
        basicColorShaders->setUniform("view", camera->getViewMatrix());
        basicColorShaders->setUniform("projection", camera->getPerspectiveMatrix());

        basicColorShaders->setUniform("color", pointLightIntensities[0] * pointLightColors[0]);

        glDrawArrays(GL_TRIANGLES, 0, cubeVertices.size());

        basicColorShaders->setUniform("model", glm::translate(glm::identity<glm::mat4>(), lightCubePositions[1])
                                              * glm::scale(glm::identity<glm::mat4>(), glm::vec3(lightCubeScale)));

        basicColorShaders->setUniform("color", pointLightIntensities[1] * pointLightColors[1]);

        glDrawArrays(GL_TRIANGLES, 0, cubeVertices.size());

        basicColorShaders->setUniform("model", glm::translate(glm::identity<glm::mat4>(), -directionalLightDirection * 10.0f)
                                              * glm::scale(glm::identity<glm::mat4>(), glm::vec3(lightCubeScale)));
        basicColorShaders->setUniform("color", directionalLightColor);

        glDrawArrays(GL_TRIANGLES, 0, cubeVertices.size());
    }

    // render the skybox

    {
        glDisable(GL_CULL_FACE);
        glDepthFunc(GL_LEQUAL);

        glBindVertexArray(cubeMesh.vao);
        skyboxShaders->enable();

        skyboxShaders->setUniform("view", camera->getSkyboxViewMatrix());
        skyboxShaders->setUniform("projection", camera->getPerspectiveMatrix());

        skyboxShaders->setUniform("skybox_texture", TexSlots::CUBEMAP);

        glDrawArrays(GL_TRIANGLES, 0, cubeVertices.size());

        glEnable(GL_CULL_FACE);
        glDepthFunc(GL_LESS);
    }

    // now we draw to the main (window) framebuffer

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // finally use the results from previous passes to show the result to the window

    {
        glDisable(GL_CULL_FACE);
        glBindVertexArray(texturedQuadMesh.vao);
        hdrQuadShaders->enable();

        hdrQuadShaders->setUniform("model", glm::identity<glm::mat4>());
        hdrQuadShaders->setUniform("view", glm::identity<glm::mat4>());
        hdrQuadShaders->setUniform("projection", glm::identity<glm::mat4>());

        hdrQuadShaders->setUniform("main_color_texture", isViewingDebugTexture ? debugTextureID : TexSlots::HDR_COLOR);
        hdrQuadShaders->setUniform("exposure", exposure);

        glDrawArrays(GL_TRIANGLES, 0, screenSpaceQuadVertices.size());
        glEnable(GL_CULL_FACE);
    }

    // render the GUI

    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (ImGui::Begin("Renderer settings")) {
            ImGui::PushItemWidth(300);

            if (ImGui::Button("Reload shaders")) {
                needRecreateShaders = true;
            }

            ImGui::SliderFloat("Time scale", &timeScale, 0.0f, 10.0f);
            ImGui::SliderFloat("Outline width", &outlineWidth, 0.0f, 2.0f);
            ImGui::SliderFloat("Exposure", &exposure, 0.0f, 10.0f);
            ImGui::SliderInt("Blur radius", &blurRadius, 0, 20);
            ImGui::SliderFloat("SSAO sample radius", &ssaoSampleRadius, 0.0f, 1.0f);
            ImGui::Checkbox("Enable SSAO", &isSsaoEnabled);

            ImGui::Separator();

            ImGui::Checkbox("View debug texture", &isViewingDebugTexture);
            if (isViewingDebugTexture) {
                ImGui::InputInt("Debug texture ID", reinterpret_cast<int*>(&debugTextureID));
            }

            ImGui::Separator();

            ImGui::Text("Directional light");
            ImGui::ColorEdit3("Color##0", &directionalLightColor.x);
            ImGui::SameLine();
            ImGui::SliderFloat("Intensity##0", &directionalLightIntensity, 0.0f, 20.0f);

            ImGui::Text("Point light 1");
            ImGui::ColorEdit3("Color##1", &pointLightColors[0].x);
            ImGui::SameLine();
            ImGui::SliderFloat("Intensity##1", &pointLightIntensities[0], 0.0f, 20.0f);

            ImGui::Text("Point light 2");
            ImGui::ColorEdit3("Color##2", &pointLightColors[1].x);
            ImGui::SameLine();
            ImGui::SliderFloat("Intensity##2", &pointLightIntensities[1], 0.0f, 20.0f);

            ImGui::PopItemWidth();

            ImGui::End();
        }

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}

void OpenGLRenderer::finishRendering() const {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void OpenGLRenderer::prepareBuffers() {
    { // loaded mesh
        glGenVertexArrays(1, &loadedMesh.vao);
        glBindVertexArray(loadedMesh.vao);

        glGenBuffers(1, &loadedMesh.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, loadedMesh.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(MeshVertex) * loadedMeshVertices.size(), loadedMeshVertices.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &loadedMesh.ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, loadedMesh.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * loadedMeshIndices.size(), loadedMeshIndices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(MeshVertex),
            reinterpret_cast<void *>(offsetof(MeshVertex, position))
        );
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            1,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(MeshVertex),
            reinterpret_cast<void *>(offsetof(MeshVertex, tex_coords))
        );
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(
            2,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(MeshVertex),
            reinterpret_cast<void *>(offsetof(MeshVertex, normal))
        );
        glEnableVertexAttribArray(2);

        glVertexAttribPointer(
            3,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(MeshVertex),
            reinterpret_cast<void *>(offsetof(MeshVertex, tangent))
        );
        glEnableVertexAttribArray(3);

        glVertexAttribPointer(
            4,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(MeshVertex),
            reinterpret_cast<void *>(offsetof(MeshVertex, bitangent))
        );
        glEnableVertexAttribArray(4);
    }

    { // light cube mesh
        glGenVertexArrays(1, &cubeMesh.vao);
        glBindVertexArray(cubeMesh.vao);

        glGenBuffers(1, &cubeMesh.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, cubeMesh.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(BasicVertex) * cubeVertices.size(), cubeVertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(BasicVertex),
            reinterpret_cast<void *>(offsetof(BasicVertex, position))
        );
        glEnableVertexAttribArray(0);
    }

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

void OpenGLRenderer::loadTextures() {
    stbi_set_flip_vertically_on_load(true);

    // color texture
    {
        int width, height, channelCount;
        unsigned char *data = stbi_load("../assets/helmet/albedo.png", &width, &height, &channelCount, 0);
        if (!data) {
            throw std::runtime_error("failed to load texture!");
        }

        glGenTextures(1, &colorTextureID);
        glActiveTexture(GL_TEXTURE0 + TexSlots::BASE_COLOR);
        glBindTexture(GL_TEXTURE_2D, colorTextureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
    }

    // normal texture
    {
        int width, height, channelCount;
        unsigned char *data = stbi_load("../assets/helmet/normal.png", &width, &height, &channelCount, 0);
        if (!data) {
            throw std::runtime_error("failed to load texture!");
        }

        glGenTextures(1, &normalTextureID);
        glActiveTexture(GL_TEXTURE0 + TexSlots::NORMAL);
        glBindTexture(GL_TEXTURE_2D, normalTextureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
    }

    // reflectivity texture
    {
        int width, height, channelCount;
        unsigned char *data = stbi_load("../assets/helmet/reflectivity.png", &width, &height, &channelCount, 0);
        if (!data) {
            throw std::runtime_error("failed to load texture!");
        }

        glGenTextures(1, &reflectivityTextureID);
        glActiveTexture(GL_TEXTURE0 + TexSlots::REFLECTIVITY);
        glBindTexture(GL_TEXTURE_2D, reflectivityTextureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
    }

    // cubemap textures
    {
        const std::map<int, std::filesystem::path> cubemapTexturePaths {
            { GL_TEXTURE_CUBE_MAP_POSITIVE_X, "../assets/skybox/right.jpg"  },
            { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, "../assets/skybox/left.jpg"   },
            { GL_TEXTURE_CUBE_MAP_POSITIVE_Y, "../assets/skybox/top.jpg"    },
            { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, "../assets/skybox/bottom.jpg" },
            { GL_TEXTURE_CUBE_MAP_POSITIVE_Z, "../assets/skybox/front.jpg"  },
            { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, "../assets/skybox/back.jpg"   },
        };

        glGenTextures(1, &cubemapTextureID);
        glActiveTexture(GL_TEXTURE0 + TexSlots::CUBEMAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureID);

        stbi_set_flip_vertically_on_load(false);

        for (const auto& [cubemapFaceID, path] : cubemapTexturePaths) {
            int width, height, channelCount;
            unsigned char *data = stbi_load(path.string().c_str(), &width, &height, &channelCount, 0);
            if (!data) {
                throw std::runtime_error("failed to load texture!");
            }

            glTexImage2D(cubemapFaceID, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }
}

void OpenGLRenderer::createTempBlurTexture() {
    glGenTextures(1, &blurTemporaryTextureID);
    glActiveTexture(GL_TEXTURE0 + TexSlots::BLUR_TEMP);
    glBindTexture(GL_TEXTURE_2D, blurTemporaryTextureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowSize.x, windowSize.y, 0, GL_RGBA, GL_FLOAT, nullptr);
}

void OpenGLRenderer::createSsaoResources() {
    std::uniform_real_distribution<float> dist(0.0, 1.0);
    std::default_random_engine engine;

    // samples
    
    for (size_t i = 0; i < ssaoSamples.size(); ++i) {
        glm::vec3& sample = ssaoSamples[i];

        do {
            sample = glm::vec3 {
                dist(engine) * 2.0 - 1.0,
                dist(engine) * 2.0 - 1.0,
                dist(engine)
            };
        } while (glm::length(sample) <= 1.0f);

        float scale = static_cast<float>(i) / 64.0f;
        scale   = std::lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
    }

    // noise texture
    
    glGenTextures(1, &ssaoSampleNoiseTextureID);
    glActiveTexture(GL_TEXTURE0 + TexSlots::SSAO_NOISE);
    glBindTexture(GL_TEXTURE_2D, ssaoSampleNoiseTextureID);

    // we'll tile this texture across the whole screen during the SSAO pass
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    constexpr GLsizei noiseTexWidth = 4;
    std::vector<glm::vec3> noise(noiseTexWidth * noiseTexWidth);

    for (auto &elem : noise) {
        elem = glm::vec3 {
            dist(engine) * 2.0 - 1.0,
            dist(engine) * 2.0 - 1.0,
            0.0f
        };
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, noiseTexWidth, noiseTexWidth, 0, GL_RGB, GL_FLOAT, noise.data());
}

void OpenGLRenderer::loadMesh() {
    tinyobj::ObjReaderConfig reader_config{};
    tinyobj::ObjReader reader{};

    if (!reader.ParseFromFile("../assets/helmet/helmet.obj", reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error();
        }

        throw std::runtime_error("failed to load mesh with tinyobjloader");
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto &attrib = reader.GetAttrib();
    auto &shapes = reader.GetShapes();

    std::unordered_map<MeshVertex, GLuint> vertexToIndexMapping;

    // Loop over shapes (there's only one in kettle.obj)
    for (const auto &shape : shapes) {
        size_t indexOffset = 0;
        size_t nextFreeIndex = 0;

        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            const size_t numFaceVertices = static_cast<size_t>(shape.mesh.num_face_vertices[f]);

            // Loop over vertices in the face.
            for (size_t v = 0; v < numFaceVertices; v++) {
                const tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                const glm::vec3 position = {
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]
                };

                // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                if (idx.texcoord_index < 0) {
                    throw std::runtime_error("no texcoords in loaded mesh");
                }

                const glm::vec2 uv = {
                    attrib.texcoords[2 * idx.texcoord_index + 0],
                    attrib.texcoords[2 * idx.texcoord_index + 1]
                };

                // Check if `normal_index` is zero or positive. negative = no normals data
                if (idx.normal_index < 0) {
                    throw std::runtime_error("no normals in loaded mesh");
                }

                const glm::vec3 normal = {
                    attrib.normals[3 * idx.normal_index + 0],
                    attrib.normals[3 * idx.normal_index + 1],
                    attrib.normals[3 * idx.normal_index + 2]
                };

                const MeshVertex newVertex { position, uv, normal };

                if (vertexToIndexMapping.contains(newVertex)) {
                    loadedMeshIndices.emplace_back(vertexToIndexMapping.at(newVertex));
                } else {
                    const GLuint index = nextFreeIndex++;
                    vertexToIndexMapping.emplace(newVertex, index);
                    loadedMeshVertices.push_back(newVertex);
                    loadedMeshIndices.emplace_back(index);
                }
            }

            indexOffset += numFaceVertices;
        }
    }
}

void OpenGLRenderer::calculateTbnVectors() {
    const size_t triangleCount = static_cast<size_t>(loadedMeshIndices.size() / 3);

    for (size_t i = 0; i < triangleCount; i++) {
        const uint32_t index1 = loadedMeshIndices[i * 3 + 0];
        const uint32_t index2 = loadedMeshIndices[i * 3 + 1];
        const uint32_t index3 = loadedMeshIndices[i * 3 + 2];

        MeshVertex& vertex1 = loadedMeshVertices[index1];
        MeshVertex& vertex2 = loadedMeshVertices[index2];
        MeshVertex& vertex3 = loadedMeshVertices[index3];

        const glm::vec3 delta_pos_1 = vertex2.position - vertex1.position;
        const glm::vec3 delta_pos_2 = vertex3.position - vertex1.position;

        const glm::vec2 delta_uv_1 = vertex2.tex_coords - vertex1.tex_coords;
        const glm::vec2 delta_uv_2 = vertex3.tex_coords - vertex1.tex_coords;

        const float scale = 1.0f / (delta_uv_1.x * delta_uv_2.y - delta_uv_2.x * delta_uv_1.y);

        const glm::vec3 tangent = scale * glm::vec3 {
            delta_uv_2.y * delta_pos_1.x - delta_uv_1.y * delta_pos_2.x,
            delta_uv_2.y * delta_pos_1.y - delta_uv_1.y * delta_pos_2.y,
            delta_uv_2.y * delta_pos_1.z - delta_uv_1.y * delta_pos_2.z
        };

        const glm::vec3 bitangent = scale * glm::vec3 {
            delta_uv_2.x * delta_pos_1.x - delta_uv_1.x * delta_pos_2.x,
            delta_uv_2.x * delta_pos_1.y - delta_uv_1.x * delta_pos_2.y,
            delta_uv_2.x * delta_pos_1.z - delta_uv_1.x * delta_pos_2.z
        };

        vertex1.tangent = tangent;
        vertex2.tangent = tangent;
        vertex3.tangent = tangent;

        vertex1.bitangent = bitangent;
        vertex2.bitangent = bitangent;
        vertex3.bitangent = bitangent;
    }
}

void OpenGLRenderer::initPrepassFramebuffer() {
    // we recreate the framebuffer in `windowRefreshCallback`, so we delete these resources if they already exist
    if (prepassFramebuffer) {
        glDeleteFramebuffers(1, &prepassFramebuffer);
        const std::array texturesToDelete {
            prepassPositionTextureID,
            prepassNormalTextureID,
            prepassDepthTextureID
        };
        glDeleteTextures(static_cast<GLsizei>(texturesToDelete.size()), texturesToDelete.data());
    }

    glGenFramebuffers(1, &prepassFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, prepassFramebuffer);

    { // per-fragment position
        glGenTextures(1, &prepassPositionTextureID);
        glActiveTexture(GL_TEXTURE0 + TexSlots::PREPASS_POS);
        glBindTexture(GL_TEXTURE_2D, prepassPositionTextureID);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowSize.x, windowSize.y,
                     0, GL_RGBA, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, prepassPositionTextureID, 0);
    }

    { // per-fragment normal
        glGenTextures(1, &prepassNormalTextureID);
        glActiveTexture(GL_TEXTURE0 + TexSlots::PREPASS_NORM);
        glBindTexture(GL_TEXTURE_2D, prepassNormalTextureID);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowSize.x, windowSize.y,
                     0, GL_RGBA, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, prepassNormalTextureID, 0);
    }

    const std::array<GLuint, 2> attachments = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glNamedFramebufferDrawBuffers(prepassFramebuffer, attachments.size(), attachments.data());

    { // depth and stencil
        glGenTextures(1, &prepassDepthTextureID);
        glActiveTexture(GL_TEXTURE0 + TexSlots::PREPASS_DEPTH);
        glBindTexture(GL_TEXTURE_2D, prepassDepthTextureID);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, windowSize.x, windowSize.y,
                     0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, prepassDepthTextureID, 0);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("framebuffer creation failed! status: " + std::to_string(glCheckFramebufferStatus(GL_FRAMEBUFFER)));
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLRenderer::initSsaoFramebuffer() {
    // we recreate the framebuffer in `windowRefreshCallback`, so we delete these resources if they already exist
    if (ssaoFramebuffer) {
        glDeleteFramebuffers(1, &ssaoFramebuffer);
        glDeleteTextures(1, &ssaoTextureID);
    }

    glGenFramebuffers(1, &ssaoFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFramebuffer);

    { // just the ssao texture
        glGenTextures(1, &ssaoTextureID);
        glActiveTexture(GL_TEXTURE0 + TexSlots::SSAO);
        glBindTexture(GL_TEXTURE_2D, ssaoTextureID);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowSize.x, windowSize.y,
                     0, GL_RGBA, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoTextureID, 0);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("framebuffer creation failed! status: " + std::to_string(glCheckFramebufferStatus(GL_FRAMEBUFFER)));
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLRenderer::initHdrFramebuffer() {
    // we recreate the framebuffer in `windowRefreshCallback`, so we delete these resources if they already exist
    if (hdrFramebuffer) {
        glDeleteFramebuffers(1, &hdrFramebuffer);
        const std::array texturesToDelete {
            hdrColorTextureID,
            hdrDepthTextureID
        };
        glDeleteTextures(static_cast<GLsizei>(texturesToDelete.size()), texturesToDelete.data());
    }

    glGenFramebuffers(1, &hdrFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFramebuffer);

    { // color
        glGenTextures(1, &hdrColorTextureID);
        glActiveTexture(GL_TEXTURE0 + TexSlots::HDR_COLOR);
        glBindTexture(GL_TEXTURE_2D, hdrColorTextureID);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowSize.x, windowSize.y,
                     0, GL_RGBA, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColorTextureID, 0);
    }

    { // depth and stencil
        glGenTextures(1, &hdrDepthTextureID);
        glActiveTexture(GL_TEXTURE0 + TexSlots::HDR_DEPTH);
        glBindTexture(GL_TEXTURE_2D, hdrDepthTextureID);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, windowSize.x, windowSize.y,
                     0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, hdrDepthTextureID, 0);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("framebuffer creation failed! status: " + std::to_string(glCheckFramebufferStatus(GL_FRAMEBUFFER)));
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLRenderer::windowRefreshCallback(GLFWwindow *window) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    OpenGLRenderer *renderer = static_cast<OpenGLRenderer *>(glfwGetWindowUserPointer(window));
    renderer->camera->updateAspectRatio();
    renderer->initHdrFramebuffer();
    renderer->initPrepassFramebuffer();
    renderer->initSsaoFramebuffer();
    renderer->createTempBlurTexture();
    renderer->render();
    glfwSwapBuffers(window);
    glFinish(); // important, this waits until rendering result is actually visible, thus making resizing less ugly
}

void OpenGLRenderer::framebufferSizeCallback(GLFWwindow *window, const int width, const int height) {
    if (width > 0 && height > 0) {
        glViewport(0, 0, width, height);

        OpenGLRenderer *renderer = static_cast<OpenGLRenderer *>(glfwGetWindowUserPointer(window));
        renderer->windowSize = { width, height };
    }
}
