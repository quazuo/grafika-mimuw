#include "renderer.hpp"

#include <stdexcept>
#include <iostream>
#include <vector>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <stb_image.h>

#include <tiny_obj_loader.h>

#include "utilities/debug.hpp"
#include "vertex.hpp"

const std::vector<BasicVertex> cubeVertices{
    {{ 1.0f, -1.0f, -1.0f}},
    {{-1.0f, -1.0f, -1.0f}},
    {{ 1.0f,  1.0f, -1.0f}},
    {{-1.0f,  1.0f, -1.0f}},
    {{ 1.0f,  1.0f, -1.0f}},
    {{-1.0f, -1.0f, -1.0f}},

    {{-1.0f, -1.0f,  1.0f}},
    {{ 1.0f, -1.0f,  1.0f}},
    {{ 1.0f,  1.0f,  1.0f}},
    {{ 1.0f,  1.0f,  1.0f}},
    {{-1.0f,  1.0f,  1.0f}},
    {{-1.0f, -1.0f,  1.0f}},

    {{-1.0f,  1.0f,  1.0f}},
    {{-1.0f,  1.0f, -1.0f}},
    {{-1.0f, -1.0f, -1.0f}},
    {{-1.0f, -1.0f, -1.0f}},
    {{-1.0f, -1.0f,  1.0f}},
    {{-1.0f,  1.0f,  1.0f}},

    {{ 1.0f,  1.0f, -1.0f}},
    {{ 1.0f,  1.0f,  1.0f}},
    {{ 1.0f, -1.0f, -1.0f}},
    {{ 1.0f, -1.0f,  1.0f}},
    {{ 1.0f, -1.0f, -1.0f}},
    {{ 1.0f,  1.0f,  1.0f}},

    {{-1.0f, -1.0f, -1.0f}},
    {{ 1.0f, -1.0f, -1.0f}},
    {{ 1.0f, -1.0f,  1.0f}},
    {{ 1.0f, -1.0f,  1.0f}},
    {{-1.0f, -1.0f,  1.0f}},
    {{-1.0f, -1.0f, -1.0f}},

    {{ 1.0f,  1.0f, -1.0f}},
    {{-1.0f,  1.0f, -1.0f}},
    {{ 1.0f,  1.0f,  1.0f}},
    {{-1.0f,  1.0f,  1.0f}},
    {{ 1.0f,  1.0f,  1.0f}},
    {{-1.0f,  1.0f, -1.0f}},
};

const std::vector<BasicTexturedVertex> tvQuadVertices{
    {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f}},
    {{-1.0f,  1.0f, 0.0f}, {0.0f, 1.0f}},
    {{ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f}},
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
};

OpenGLRenderer::OpenGLRenderer(const int windowWidth, const int windowHeight) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    windowSize = {windowWidth, windowHeight};
    window     = glfwCreateWindow(windowWidth, windowHeight, "11-framebuffer", nullptr, nullptr);
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
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // final_color = alpha * new_color + (1 - alpha) * old_color

    glEnable(GL_DEBUG_OUTPUT);
#ifndef __APPLE__
    glDebugMessageCallback(reinterpret_cast<GLDEBUGPROC>(&debugCallback), nullptr);
#endif

    glfwSetWindowRefreshCallback(window, windowRefreshCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetWindowUserPointer(window, this);

    mainShaders = std::make_unique<GLShaders>(
        "../11-framebuffer/shaders/blinn-phong.vert",
        "../11-framebuffer/shaders/blinn-phong.frag"
    );
    basicColorShaders = std::make_unique<GLShaders>(
        "../11-framebuffer/shaders/basic-color.vert",
        "../11-framebuffer/shaders/basic-color.frag"
    );
    basicTexturedShaders = std::make_unique<GLShaders>(
        "../11-framebuffer/shaders/basic-textured.vert",
        "../11-framebuffer/shaders/basic-textured.frag"
    );
    skyboxShaders = std::make_unique<GLShaders>(
        "../11-framebuffer/shaders/skybox.vert",
        "../11-framebuffer/shaders/skybox.frag"
    );

    camera = std::make_unique<Camera>(window);

    loadMesh();
    calculateTbnVectors();
    prepareBuffers();

    loadTextures();

    initTvFramebuffer();
}

OpenGLRenderer::~OpenGLRenderer() {
    glDeleteBuffers(1, &loadedMesh.vbo);
    glDeleteVertexArrays(1, &loadedMesh.vao);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void OpenGLRenderer::tickInputEvents() {
    camera->tickInputEvents();

    // reload shaders
    static bool wasPressedLastFrame = false;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (!wasPressedLastFrame) {
            mainShaders = std::make_unique<GLShaders>(
                "../11-framebuffer/shaders/blinn-phong.vert",
                "../11-framebuffer/shaders/blinn-phong.frag"
            );
            basicColorShaders = std::make_unique<GLShaders>(
                "../11-framebuffer/shaders/basic-color.vert",
                "../11-framebuffer/shaders/basic-color.frag"
            );
            basicTexturedShaders = std::make_unique<GLShaders>(
                "../11-framebuffer/shaders/basic-textured.vert",
                "../11-framebuffer/shaders/basic-textured.frag"
            );
            skyboxShaders = std::make_unique<GLShaders>(
                "../11-framebuffer/shaders/skybox.vert",
                "../11-framebuffer/shaders/skybox.frag"
            );
        }
        wasPressedLastFrame = true;
    } else {
        wasPressedLastFrame = false;
    }
}

void OpenGLRenderer::startRendering() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderer::render() {
    glBindFramebuffer(GL_FRAMEBUFFER, tvFramebuffer);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, tvFramebufferWidth, tvFramebufferHeight);
    renderTvView();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowSize.x, windowSize.y);
    renderScene();

    {
        glDisable(GL_CULL_FACE); // disable face culling for a moment as we want to view the quad from both directions

        glBindVertexArray(texturedQuadMesh.vao);
        basicTexturedShaders->enable();

        basicTexturedShaders->setUniform("model", glm::translate(glm::identity<glm::mat4>(), glm::vec3(-5, 0, 5)));
        basicTexturedShaders->setUniform("view", camera->getViewMatrix());
        basicTexturedShaders->setUniform("projection", camera->getPerspectiveMatrix());

        basicTexturedShaders->setUniform("sampled_texture", 4);

        glDrawArrays(GL_TRIANGLES, 0, tvQuadVertices.size());

        glEnable(GL_CULL_FACE);
    }
}

void OpenGLRenderer::finishRendering() const {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void OpenGLRenderer::renderScene() {
    const float time = static_cast<float>(glfwGetTime());

    constexpr float lightCubeScale = 0.05f;
    constexpr float lightOrbitRadius = 3.0f;
    constexpr float timeScale = 1.0f;
    const auto lightCubePosition = lightOrbitRadius * glm::vec3 {
        glm::sin(time * timeScale),
        0.0f,
        glm::cos(time * timeScale)
    };
    constexpr glm::vec4 pointLightColor { 1.0f, 0.0f, 0.0f, 1.0f };

    constexpr glm::vec4 directionalLightColor { 1, 0.9, 0.8, 1.0f };
    const glm::vec3 directionalLightDirection = glm::normalize(glm::vec3(-1.0f, -2.0f, -3.0f));

    {
        glBindVertexArray(cubeMesh.vao);
        basicColorShaders->enable();

        basicColorShaders->setUniform("model", glm::translate(glm::identity<glm::mat4>(), lightCubePosition)
                                              * glm::scale(glm::identity<glm::mat4>(), glm::vec3(lightCubeScale)));
        basicColorShaders->setUniform("view", camera->getViewMatrix());
        basicColorShaders->setUniform("projection", camera->getPerspectiveMatrix());

        basicColorShaders->setUniform("color", pointLightColor);

        glDrawArrays(GL_TRIANGLES, 0, cubeVertices.size());

        basicColorShaders->setUniform("model", glm::translate(glm::identity<glm::mat4>(), -directionalLightDirection * 10.0f)
                                              * glm::scale(glm::identity<glm::mat4>(), glm::vec3(lightCubeScale)));
        basicColorShaders->setUniform("color", directionalLightColor);

        glDrawArrays(GL_TRIANGLES, 0, cubeVertices.size());
    }

    {
        glBindVertexArray(loadedMesh.vao);
        mainShaders->enable();

        mainShaders->setUniform("model", glm::scale(glm::identity<glm::mat4>(), glm::vec3(2.0f)));
        mainShaders->setUniform("view", camera->getViewMatrix());
        mainShaders->setUniform("projection", camera->getPerspectiveMatrix());

        mainShaders->setUniform("color_texture", 0);
        mainShaders->setUniform("normal_texture", 1);
        mainShaders->setUniform("reflectivity_texture", 2);
        mainShaders->setUniform("skybox_texture", 3);
        mainShaders->setUniform("camera_position", camera->getPosition());

        mainShaders->setUniform("directional_light.direction", directionalLightDirection);
        mainShaders->setUniform("directional_light.color", glm::vec3(directionalLightColor));

        mainShaders->setUniform("point_light.position", lightCubePosition);
        mainShaders->setUniform("point_light.color", glm::vec3(pointLightColor));
        mainShaders->setUniform("point_light.att_constant", 1.0f);
        mainShaders->setUniform("point_light.att_linear", 0.22f);
        mainShaders->setUniform("point_light.att_quadratic", 0.2f);

        glDrawElements(GL_TRIANGLES, loadedMeshIndices.size(), GL_UNSIGNED_INT, 0);
    }

    {
        glDisable(GL_CULL_FACE);
        glDepthFunc(GL_LEQUAL);

        glBindVertexArray(cubeMesh.vao);
        skyboxShaders->enable();

        skyboxShaders->setUniform("view", camera->getSkyboxViewMatrix());
        skyboxShaders->setUniform("projection", camera->getPerspectiveMatrix());

        skyboxShaders->setUniform("skybox_texture", 3);

        glDrawArrays(GL_TRIANGLES, 0, cubeVertices.size());

        glEnable(GL_CULL_FACE);
        glDepthFunc(GL_LESS);
    }

    {
        glDisable(GL_CULL_FACE); // disable face culling for now as we want to view the quads from both directions

        glBindVertexArray(cubeMesh.vao);
        basicColorShaders->enable();

        // these 3 are easily sortable

        basicColorShaders->setUniform("model", glm::translate(glm::identity<glm::mat4>(), glm::vec3(1, 0, 5)));
        basicColorShaders->setUniform("color", glm::vec4(1.0f, 0.0f, 0.0f, 0.3f));
        glDrawArrays(GL_TRIANGLES, 0, 6); // just render one face of the cube as a quick hack to get a quad :)

        basicColorShaders->setUniform("model", glm::translate(glm::identity<glm::mat4>(), glm::vec3(0, 1, 6)));
        basicColorShaders->setUniform("color", glm::vec4(0.0f, 1.0f, 0.0f, 0.7f));
        glDrawArrays(GL_TRIANGLES, 0, 6);

        basicColorShaders->setUniform("model", glm::translate(glm::identity<glm::mat4>(), glm::vec3(-1, -1, 7)));
        basicColorShaders->setUniform("color", glm::vec4(0.0f, 0.0f, 1.0f, 0.1f));
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // edge case -- these are not sortable
        // a more refined solution is needed here!

        basicColorShaders->setUniform("model", glm::translate(glm::identity<glm::mat4>(), glm::vec3(-1.0, 0, 15)));
        basicColorShaders->setUniform("color", glm::vec4(1.0f, 1.0f, 0.0f, 0.3f));
        glDrawArrays(GL_TRIANGLES, 0, 6);

        basicColorShaders->setUniform("model", glm::translate(glm::identity<glm::mat4>(), glm::vec3(0, 0, 14))
                                               * glm::rotate(glm::identity<glm::mat4>(), 3.14f / 2, glm::vec3(0, 1, 0)));
        basicColorShaders->setUniform("color", glm::vec4(0.0f, 1.0f, 1.0f, 0.3f));
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glEnable(GL_CULL_FACE);
    }
}

void OpenGLRenderer::renderTvView() {
    const float time = static_cast<float>(glfwGetTime());

    constexpr float lightOrbitRadius = 3.0f;
    constexpr float timeScale = 1.0f;
    const auto lightCubePosition = lightOrbitRadius * glm::vec3 {
        glm::sin(time * timeScale),
        0.0f,
        glm::cos(time * timeScale)
    };
    constexpr glm::vec4 pointLightColor { 1.0f, 0.0f, 0.0f, 1.0f };

    constexpr glm::vec4 directionalLightColor { 1, 0.9, 0.8, 1.0f };
    const glm::vec3 directionalLightDirection = glm::normalize(glm::vec3(-1.0f, -2.0f, -3.0f));

    const glm::mat4 lightViewMatrix = glm::lookAt(lightCubePosition, glm::vec3(0), glm::vec3(0, 1, 0));
    const glm::mat4 lightSkyboxViewMatrix = glm::lookAt(glm::vec3(0), -lightCubePosition, glm::vec3(0, 1, 0));

    {
        glBindVertexArray(loadedMesh.vao);
        mainShaders->enable();

        mainShaders->setUniform("model", glm::scale(glm::identity<glm::mat4>(), glm::vec3(2.0f)));
        mainShaders->setUniform("view", lightViewMatrix);
        mainShaders->setUniform("projection", camera->getPerspectiveMatrix());

        mainShaders->setUniform("color_texture", 0);
        mainShaders->setUniform("normal_texture", 1);
        mainShaders->setUniform("reflectivity_texture", 2);
        mainShaders->setUniform("skybox_texture", 3);
        mainShaders->setUniform("camera_position", lightCubePosition);

        mainShaders->setUniform("directional_light.direction", directionalLightDirection);
        mainShaders->setUniform("directional_light.color", glm::vec3(directionalLightColor));

        mainShaders->setUniform("point_light.position", lightCubePosition);
        mainShaders->setUniform("point_light.color", glm::vec3(pointLightColor));
        mainShaders->setUniform("point_light.att_constant", 1.0f);
        mainShaders->setUniform("point_light.att_linear", 0.22f);
        mainShaders->setUniform("point_light.att_quadratic", 0.2f);

        glDrawElements(GL_TRIANGLES, loadedMeshIndices.size(), GL_UNSIGNED_INT, 0);
    }

    {
        glDisable(GL_CULL_FACE);
        glDepthFunc(GL_LEQUAL);

        glBindVertexArray(cubeMesh.vao);
        skyboxShaders->enable();

        skyboxShaders->setUniform("view", lightSkyboxViewMatrix);
        skyboxShaders->setUniform("projection", camera->getPerspectiveMatrix());

        skyboxShaders->setUniform("skybox_texture", 3);

        glDrawArrays(GL_TRIANGLES, 0, cubeVertices.size());

        glEnable(GL_CULL_FACE);
        glDepthFunc(GL_LESS);
    }
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
        glBufferData(GL_ARRAY_BUFFER, sizeof(BasicTexturedVertex) * tvQuadVertices.size(), tvQuadVertices.data(), GL_STATIC_DRAW);

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
        glActiveTexture(GL_TEXTURE0);
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
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalTextureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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
        glActiveTexture(GL_TEXTURE2);
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
        glActiveTexture(GL_TEXTURE3);
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

void OpenGLRenderer::initTvFramebuffer() {
    glGenFramebuffers(1, &tvFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, tvFramebuffer);

    // create the color attachment for this framebuffer, then attach it
    // at least one color attachment is *required*!
    glGenTextures(1, &framebufferColorTextureID);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, framebufferColorTextureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tvFramebufferWidth, tvFramebufferHeight,
                 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr); // no pixel data! it stays uninitialized

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferColorTextureID, 0);

    // create the depth attachment for this framebuffer, then attach it
    // a depth attachment isn't strictly required, but we will need one in this usecase
    glGenTextures(1, &framebufferDepthTextureID);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, framebufferDepthTextureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, tvFramebufferWidth, tvFramebufferHeight,
                 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, framebufferDepthTextureID, 0);

    // just to be sure, check if the framebuffer was created properly
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("framebuffer creation failed! status: " + std::to_string(glCheckFramebufferStatus(GL_FRAMEBUFFER)));
    }

    // go back to the default framebuffer (the one which represents the window)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLRenderer::windowRefreshCallback(GLFWwindow *window) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    OpenGLRenderer *renderer = static_cast<OpenGLRenderer *>(glfwGetWindowUserPointer(window));
    renderer->camera->updateAspectRatio();
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
