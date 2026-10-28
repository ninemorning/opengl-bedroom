#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

enum class Move { Forward, Backward, Left, Right };

class Camera
{
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 right;
    float yaw = -90.0f;
    float pitch = 0.0f;
    float speed = 2.5f;
    float sensitivity = 0.1f;
    float fov = 60.0f;

    Camera(glm::vec3 pos) : position(pos) { update(); }

    glm::mat4 view() const
    {
        return glm::lookAt(position, position + front, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void move(Move dir, float dt)
    {
        glm::vec3 flatFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
        float v = speed * dt;
        if (dir == Move::Forward) position += flatFront * v;
        if (dir == Move::Backward) position -= flatFront * v;
        if (dir == Move::Left) position -= right * v;
        if (dir == Move::Right) position += right * v;
    }

    void look(float dx, float dy)
    {
        yaw += dx * sensitivity;
        pitch += dy * sensitivity;
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
        update();
    }

private:
    void update()
    {
        float y = glm::radians(yaw);
        float p = glm::radians(pitch);
        front = glm::normalize(glm::vec3(std::cos(y) * std::cos(p), std::sin(p), std::sin(y) * std::cos(p)));
        right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    }
};
