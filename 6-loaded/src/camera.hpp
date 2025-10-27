#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

class Camera {
    GLFWwindow* window;

    glm::vec3 position { 0, 0, -3 };
    glm::vec2 rotation { 0, 0 };

    float aspectRatio = 4.0f / 3.0f;
    float fieldOfView = 80.0f;
    float zNear = 0.1f;
    float zFar = 500.0f;

    float movementSpeed = 0.01f;
    float rotationSpeed = 0.006f;

public:
    Camera(GLFWwindow* w) : window(w) {}

    glm::mat4 getViewMatrix() const;

    glm::mat4 getPerspectiveMatrix() const;

    /**
     * Processes all pending input events, e.g. to move and rotate the camera.
     */
    void tickInputEvents();
};

#endif //CAMERA_HPP
