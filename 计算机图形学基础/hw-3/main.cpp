// g++ --std=c++14 -lglfw3 -lglad -lsoil -lfreetype -Wall -O3 main.cpp

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

// SOIL
#include <SOIL.h>

// standard libraries
#include <iostream>

// local libraries
#include "./lib/Camera.h"
#include "./lib/Sphere.h"
#include "./lib/util.h"
#include "./lib/Viewport.h"

#include "./SolarSystem.h"

void handleKeys(SolarSystem& solar, float dt)
{
    if (util::keys[GLFW_KEY_W]) Camera::handleKey(GLFW_KEY_W, dt);
    if (util::keys[GLFW_KEY_S]) Camera::handleKey(GLFW_KEY_S, dt);
    if (util::keys[GLFW_KEY_A]) Camera::handleKey(GLFW_KEY_A, dt);
    if (util::keys[GLFW_KEY_D]) Camera::handleKey(GLFW_KEY_D, dt);

    if (util::keys[GLFW_KEY_LEFT])  solar.decreaseSpeed(dt);
    if (util::keys[GLFW_KEY_RIGHT]) solar.increaseSpeed(dt);

    if (util::keys[GLFW_KEY_SPACE])
    {
        Sphere::stepRenderMode();
        util::keys[GLFW_KEY_SPACE] = false;
    }
}

void renderLoop(GLFWwindow* window)
{
    SolarSystem solar;

    GLfloat lastFrame = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        // calculate dt
        GLfloat currentFrame = glfwGetTime();
        GLfloat dt = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // check keystrokes
        glfwPollEvents();
        handleKeys(solar, dt);

        // update solar system
        solar.step(dt);

        // draw
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = Viewport::projectionMatrix();
        glm::mat4 view = Camera::viewMatrix();
        solar.draw(projection, view);

        glfwSwapBuffers(window);
    }
}

int main()
{
    GLFWwindow* window = util::initGL(Viewport::getWidth(), Viewport::getHeight(), "Assignment 03");
    if (!window) return -1;

    Sphere::init("./shaders/sphere.vert.glsl", "./shaders/sphere.frag.glsl");
    Text::init("./shaders/text.vert.glsl", "./shaders/text.frag.glsl");

    std::cout << "controls:" << std::endl;
    std::cout << "  W-A-S-D   : move around" << std::endl;
    std::cout << "  mouse     : look around" << std::endl;
    std::cout << "  R/L arrows: adjust speed" << std::endl;
    std::cout << "  SPACE     : change polygon mode" << std::endl;

    renderLoop(window);

    Sphere::release();
    Text::release();

    glfwTerminate();
    return 0;
}
