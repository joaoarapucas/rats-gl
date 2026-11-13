#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
    Camera(glm::vec3 startPosition = glm::vec3(0.0f),
           glm::vec3 startWorldUp = glm::vec3(0.0f, 1.0f, 0.0f),
           float startYaw = -90.0f, float startPitch = 0.0f,
           float startMoveSpeed = 3.0f, float startTurnSpeed = 0.1f);

    // moveSpeed em unidades/segundo; deltaTime em segundos
    void KeyControl(const bool* keys, float deltaTime);
    // turnSpeed em graus/pixel
    void MouseControl(float xChange, float yChange);

    glm::mat4 CalculateViewMatrix() const;
    glm::vec3 GetPosition() const { return position; }

private:
    void Update();

    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;

    float moveSpeed;
    float turnSpeed;
};
