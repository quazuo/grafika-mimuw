#include <stdexcept>

#include "renderer.hpp"

int main(const int argc, char *argv[]) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    OpenGLRenderer renderer {1200, 800};

    while (!glfwWindowShouldClose(renderer.getWindow())) {
        renderer.tickInputEvents();

        renderer.startRendering();
        renderer.render();
        renderer.finishRendering();
    }

    return 0;
}
