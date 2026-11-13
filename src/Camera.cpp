#include "Camera.h"

#include <cmath>

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 startPosition, glm::vec3 startWorldUp, float startYaw, float startPitch,
               float startMoveSpeed, float startTurnSpeed)
    : position(startPosition), worldUp(startWorldUp), yaw(startYaw), pitch(startPitch),
      moveSpeed(startMoveSpeed), turnSpeed(startTurnSpeed)
{
    Update();
}

void Camera::Update()
{
    front.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    front.y = std::sin(glm::radians(pitch));
    front.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    front = glm::normalize(front);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::KeyControl(const bool* keys, float deltaTime)
{
    const float velocity = moveSpeed * deltaTime;

    if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])    position += front * velocity;
    if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])  position -= front * velocity;
    if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]) position += right * velocity;
    if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])  position -= right * velocity;
    if (keys[GLFW_KEY_SPACE])                     position += worldUp * velocity;
    if (keys[GLFW_KEY_LEFT_SHIFT])                position -= worldUp * velocity;
}

void Camera::MouseControl(float xChange, float yChange)
{
    yaw += xChange * turnSpeed;
    pitch += yChange * turnSpeed;

    // Evita gimbal flip quando front fica paralelo a worldUp
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    Update();
}

void Camera::SetPose(const glm::vec3& newPosition, float newYaw, float newPitch)
{
    position = newPosition;
    yaw = newYaw;
    pitch = glm::clamp(newPitch, -89.0f, 89.0f);
    Update();
}

glm::mat4 Camera::CalculateViewMatrix() const
{
    return glm::lookAt(position, position + front, up);
}
