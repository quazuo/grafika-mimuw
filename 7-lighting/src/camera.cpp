#include "camera.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

glm::mat4 Camera::getViewMatrix() const {
    const glm::vec3 front = {
        std::cos(rotation.y) * std::sin(rotation.x),
        std::sin(rotation.y),
        std::cos(rotation.y) * std::cos(rotation.x)
    };

    return glm::lookAt(position, position + front, glm::vec3(0, 1, 0));
}

glm::mat4 Camera::getPerspectiveMatrix() const {
    return glm::perspective(glm::radians(fieldOfView), aspectRatio, zNear, zFar);
}

void Camera::tickInputEvents() {
    const glm::vec3 front = {
        std::cos(rotation.y) * std::sin(rotation.x),
        std::sin(rotation.y),
        std::cos(rotation.y) * std::cos(rotation.x)
    };

    const glm::vec3 right = glm::vec3(
        std::sin(rotation.x - 3.14f / 2.0f),
        0,
        std::cos(rotation.x - 3.14f / 2.0f)
    );

    const glm::vec3 up = glm::cross(right, front);

    if (glfwGetKey(window, GLFW_KEY_W)          == GLFW_PRESS) position += front * movementSpeed;
    if (glfwGetKey(window, GLFW_KEY_S)          == GLFW_PRESS) position -= front * movementSpeed;
    if (glfwGetKey(window, GLFW_KEY_A)          == GLFW_PRESS) position -= right * movementSpeed;
    if (glfwGetKey(window, GLFW_KEY_D)          == GLFW_PRESS) position += right * movementSpeed;
    if (glfwGetKey(window, GLFW_KEY_SPACE)      == GLFW_PRESS) position += up    * movementSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) position -= up    * movementSpeed;

    if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) rotation.x += rotationSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) rotation.x -= rotationSpeed;
    if (glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS) rotation.y += rotationSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS) rotation.y -= rotationSpeed;

    constexpr float half_pi = glm::pi<float>() / 2.0f;
    constexpr float eps = 0.001f;
    rotation.y = glm::clamp(rotation.y, -half_pi + eps, half_pi - eps);
}
