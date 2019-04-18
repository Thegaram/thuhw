#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

#include <glm/glm.hpp>

#include <cassert>
#include <cmath>

#include "./util.h"

class Text;


class Camera
{
private:

    static float yaw;
    static float pitch;

    static glm::vec3 pos;
    static glm::vec3 up;
    static glm::vec3 front;
    static glm::vec3 right;

    static const float movementSpeed;
    static const float mouseSensitivity;

    static void update()
    {
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        front = glm::normalize(newFront);
        right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        up    = glm::normalize(glm::cross(right, front));
    }

public:
    static glm::mat4 viewMatrix()
    {
        return glm::lookAt(pos, pos + front, up);
    }

    static void move(GLfloat xoffset, GLfloat yoffset)
    {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw   += xoffset;
        pitch += yoffset;

        pitch = fmin(pitch, +89.0f);
        pitch = fmax(pitch, -89.0f);

        update();
    }

    static void handleKey(int key, float dt)
    {
        assert(util::isUnit(front));
        assert(util::isUnit(right));

        switch (key)
        {
            case GLFW_KEY_W: pos += front * movementSpeed * dt; break;
            case GLFW_KEY_S: pos -= front * movementSpeed * dt; break;
            case GLFW_KEY_A: pos -= right * movementSpeed * dt; break;
            case GLFW_KEY_D: pos += right * movementSpeed * dt; break;
        }
    }

    static glm::vec3 getRight()
    {
        return right;
    }

    static float getPitch()
    {
        return pitch;
    }
};

const float Camera::movementSpeed = 10.0f;
const float Camera::mouseSensitivity = 0.1f;

glm::vec3 Camera::pos = glm::vec3(0.0f, 0.0f, 5.0f);

glm::vec3 Camera::front = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - Camera::pos);
glm::vec3 Camera::up    = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f));
glm::vec3 Camera::right = glm::normalize(glm::cross(Camera::front, Camera::up));

float Camera::pitch = 0.0f;
float Camera::yaw = -90.0f;

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    static GLfloat lastX, lastY;
    static bool firstMouse = true;

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    GLfloat xoffset = xpos - lastX;
    GLfloat yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to left
    lastX = xpos;
    lastY = ypos;

    Camera::move(xoffset, yoffset);
}

#endif // CAMERA_H_INCLUDED