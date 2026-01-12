#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <../dependencies/glew/include/GL/glew.h>
#include <../dependencies/glfw/include/GLFW/glfw3.h>

#include <../dependencies/glm/glm/glm.hpp>

class Camera {
    GLFWwindow* window;

    glm::vec3 position { 0, 0, 10 };
    glm::vec2 rotation { 3.14f, 0 };

    float aspectRatio;
    float fieldOfView = 80.0f;
    float zNear = 0.01f;
    float zFar = 500.0f;

    float movementSpeed = 0.01f;
    float rotationSpeed = 0.01f;

public:
    Camera(GLFWwindow* w) : window(w) { updateAspectRatio(); }

    glm::vec3 getPosition() const { return position; }

    glm::mat4 getViewMatrix() const;

    glm::mat4 getSkyboxViewMatrix() const;

    glm::mat4 getPerspectiveMatrix() const;

    /**
     * Processes all pending input events, e.g. to move and rotate the camera.
     */
    void tickInputEvents();

    void updateAspectRatio();
};

#endif //CAMERA_HPP
