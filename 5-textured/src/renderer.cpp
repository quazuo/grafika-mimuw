#include "renderer.hpp"

#include <stdexcept>
#include <vector>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <stb_image.h>

#include "utilities/debug.hpp"
#include "vertex.hpp"

// we'll move to a simple cube for a moment
const std::vector<Vertex> vertices{
//   position               uv
    {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f}},
    {{ 1.0f, -1.0f, -1.0f}, {0.0f, 1.0f}},
    {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f}},
    {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f}},
    {{-1.0f,  1.0f, -1.0f}, {1.0f, 0.0f}},
    {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f}},

    {{-1.0f, -1.0f,  1.0f}, {0.0f, 1.0f}},
    {{ 1.0f, -1.0f,  1.0f}, {1.0f, 1.0f}},
    {{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f}},
    {{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f}},
    {{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
    {{-1.0f, -1.0f,  1.0f}, {0.0f, 1.0f}},

    {{-1.0f,  1.0f,  1.0f}, {1.0f, 0.0f}},
    {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f}},
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f}},
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f}},
    {{-1.0f, -1.0f,  1.0f}, {1.0f, 1.0f}},
    {{-1.0f,  1.0f,  1.0f}, {1.0f, 0.0f}},

    {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
    {{ 1.0f,  1.0f, -1.0f}, {1.0f, 0.0f}},
    {{ 1.0f, -1.0f, -1.0f}, {1.0f, 1.0f}},
    {{ 1.0f, -1.0f, -1.0f}, {1.0f, 1.0f}},
    {{ 1.0f, -1.0f,  1.0f}, {0.0f, 1.0f}},
    {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},

    {{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f}},
    {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f}},
    {{ 1.0f, -1.0f,  1.0f}, {0.0f, 1.0f}},
    {{ 1.0f, -1.0f,  1.0f}, {0.0f, 1.0f}},
    {{-1.0f, -1.0f,  1.0f}, {1.0f, 1.0f}},
    {{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f}},

    {{-1.0f,  1.0f, -1.0f}, {1.0f, 1.0f}},
    {{ 1.0f,  1.0f, -1.0f}, {0.0f, 1.0f}},
    {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
    {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
    {{-1.0f,  1.0f,  1.0f}, {1.0f, 0.0f}},
    {{-1.0f,  1.0f, -1.0f}, {1.0f, 1.0f}},
};

OpenGLRenderer::OpenGLRenderer(const int windowWidth, const int windowHeight) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    windowSize = { windowWidth, windowHeight };
    window = glfwCreateWindow(windowWidth, windowHeight, "5-textured", nullptr, nullptr);
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

    glEnable(GL_DEBUG_OUTPUT);
#ifndef __APPLE__
    glDebugMessageCallback(reinterpret_cast<GLDEBUGPROC>(&debugCallback), nullptr);
#endif

    glfwSetWindowRefreshCallback(window, windowRefreshCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetWindowUserPointer(window, this);

    shaders = std::make_unique<GLShaders>(
        "../5-textured/shaders/main.vert",
        "../5-textured/shaders/main.frag"
    );

    camera = std::make_unique<Camera>(window);

    prepareBuffers();

    loadTextures();
}

OpenGLRenderer::~OpenGLRenderer() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void OpenGLRenderer::tickInputEvents() {
    camera->tickInputEvents();

    // reload shaders
    static bool wasPressedLastFrame = false;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (!wasPressedLastFrame) {
            shaders = std::make_unique<GLShaders>(
                "../5-textured/shaders/main.vert",
                "../5-textured/shaders/main.frag"
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
    shaders->enable();

    shaders->setUniform("model", glm::identity<glm::mat4>());
    shaders->setUniform("view", camera->getViewMatrix());
    shaders->setUniform("projection", camera->getPerspectiveMatrix());
    shaders->setUniform("colorTexture", 0); // the texture is in slot 0 (GL_TEXTURE0) so that's what we set the uniform to

    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}

void OpenGLRenderer::finishRendering() const {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void OpenGLRenderer::prepareBuffers() {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void *>(offsetof(Vertex, position))
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void *>(offsetof(Vertex, uv))
    );
    glEnableVertexAttribArray(1);
}

void OpenGLRenderer::loadTextures() {
    int width, height, channelCount;
    unsigned char *data = stbi_load("../assets/textures/uvtest.png", &width, &height, &channelCount, 0);
    if (!data) {
        throw std::runtime_error("failed to load texture!");
    }

    glGenTextures(1, &colorTextureID);
    glActiveTexture(GL_TEXTURE0); // there are guaranteed 16 slots from GL_TEXTURE0 to GL_TEXTURE16; we put this texture in slot 0
    glBindTexture(GL_TEXTURE_2D, colorTextureID);

    // feel free to experiment with these 4!
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // border color used when we set either of the GL_TEXTURE_WRAP_* to GL_CLAMP_TO_BORDER
    constexpr float borderColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glTexImage2D(
        GL_TEXTURE_2D,    // target bind point -- we called glBindTexture(GL_TEXTURE_2D, ...) so we pick GL_TEXTURE_2D
        0,                // mipmap layer -- when loading images we always load into the first layer and generate the rest
        GL_RGB,           // internal format -- for simplicity it's just RGB, but we can e.g. specify bit lengths for components
        width, height,    // width and height of the texture
        0,                // must be 0 for legacy reasons
        GL_RGB,           // format of the provided data -- the provided texture has 3 channels, so it's in RGB (could be e.g. RGBA if 4 channels)
        GL_UNSIGNED_BYTE, // type of each channel's data -- typically textures have 8-bit unsigned int channels, but some are different (e.g. HDR)
        data              // pointer to the data
    );
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
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
    }
}
