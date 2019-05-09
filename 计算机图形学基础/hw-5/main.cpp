// g++ --std=c++11 -lglfw3 -lglad -lsoil -Wall main.cpp

#include <iostream>

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// SOIL
#include <SOIL.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// other includes
#include "lib/shader.h"
#include "lib/camera.h"

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);

// window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 8.0f));

// key strokes
bool keys[1024];

void updateCamera(float dt)
{
    if (keys[GLFW_KEY_W]) camera.ProcessKeyboard(FORWARD , dt);
    if (keys[GLFW_KEY_S]) camera.ProcessKeyboard(BACKWARD, dt);
    if (keys[GLFW_KEY_A]) camera.ProcessKeyboard(LEFT    , dt);
    if (keys[GLFW_KEY_D]) camera.ProcessKeyboard(RIGHT   , dt);
    if (keys[GLFW_KEY_R]) camera.ProcessKeyboard(UP      , dt);
    if (keys[GLFW_KEY_F]) camera.ProcessKeyboard(DOWN    , dt);
}

void updateSurface(float dt, float& level, bool& wireMode)
{
    // increase tessellation level
    if (keys[GLFW_KEY_X] && level < 40.0f) {
        level += dt * 10.0f;
        level = level >= 40.0f ? 40.0f : level;
    }

    // decrease tessellation level
    if (keys[GLFW_KEY_Z] && level > 1.0f) {
        level -= dt * 10.0f;
        level = level <= 1.0f ? 1.0f : level;
    }

    // change render mode
    if (keys[GLFW_KEY_C]) {
        wireMode = !wireMode;
        keys[GLFW_KEY_C] = false;
    }
}

GLFWwindow* initGL(const std::string& title)
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return nullptr;
    }

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, title.c_str(), nullptr, nullptr);
    if (window == nullptr)
    {
        std::cerr << "Failed to open GLFW window\n";
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mouseCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize glad" << std::endl;
        return nullptr;
    }

    return window;
}

void renderLoop(GLFWwindow* window)
{
    Shader surfaceShader("shaders/main.vert.glsl", "shaders/surface.tcs.glsl", "shaders/surface.tes.glsl", "shaders/surface.frag.glsl");
    Shader controlShader("shaders/main.vert.glsl", "shaders/control.frag.glsl");

    bool wireMode = false;
    float level = 1.0f;
    float lastFrame = glfwGetTime();

    std::cout << "controls:" << std::endl;
    std::cout << "  Z/X:\tchange tessellation level" << std::endl;
    std::cout << "  C:\tchange rendering mode" << std::endl;
    std::cout << "  WASD+mouse:\tmove" << std::endl;

    // Game loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        float dt = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // check events
        glfwPollEvents();
        updateSurface(dt, level, wireMode);
        updateCamera(dt);

        // prepare for drawing
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 model(1);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
        glm::mat4 MVP = projection * view * model;

        // draw Bezier surface
        surfaceShader.Use();
        glUniform1f(glGetUniformLocation(surfaceShader.Program, "uOuter02"), level);
        glUniform1f(glGetUniformLocation(surfaceShader.Program, "uOuter13"), level);
        glUniform1f(glGetUniformLocation(surfaceShader.Program, "uInner0"), level);
        glUniform1f(glGetUniformLocation(surfaceShader.Program, "uInner1"), level);
        glUniformMatrix4fv(glGetUniformLocation(surfaceShader.Program, "MVP"), 1, GL_FALSE, glm::value_ptr(MVP));

        glPolygonMode(GL_FRONT_AND_BACK, wireMode ? GL_LINE : GL_FILL);

        glPatchParameteri(GL_PATCH_VERTICES, 25);
        glDrawArrays(GL_PATCHES, 0, 25);

        // draw control points
        controlShader.Use();
        glUniformMatrix4fv(glGetUniformLocation(controlShader.Program, "MVP"), 1, GL_FALSE, glm::value_ptr(MVP));
        glPointSize(10.0f);
        glDrawArrays(GL_POINTS, 0, 25);

        glfwSwapBuffers(window);
    }
}

int main()
{
    GLFWwindow* window = initGL("Assignment 5");

    GLfloat vertices[] = {
        -2.,  2., -2., // top left
        -1.,  2., -1.,
         0.,  2.,  0.,
         1.,  2., -1.,
         2.,  2., -2., // top right

        -2.,  1.,  0.,
        -1.,  1.,  0.,
         0.,  1.,  0.,
         1.,  1.,  0.,
         2.,  1.,  0.,

        -2.,  0.,  0.,
        -1.,  0.,  0.,
         0.,  0., -4.,
         1.,  0.,  0.,
         2.,  0.,  0.,

        -2., -1.,  0.,
        -1., -1.,  0.,
         0., -1.,  0.,
         1., -1.,  0.,
         2., -1.,  0.,

        -2., -2.,  1., // bottom left
        -1., -2.,  2.,
         0., -2.,  2.,
         1., -2.,  2.,
         2., -2.,  1.  // bottom right
    };

    // initialize VAO and VBO
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // initialize texture
    GLuint texture;
    glGenTextures(1, &texture);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height;
    unsigned char* image = SOIL_load_image("textures/texture.png", &width, &height, 0, SOIL_LOAD_RGB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    SOIL_free_image_data(image);

    // render
    renderLoop(window);

    // free resources
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();

    return 0;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS  ) keys[key] = true;
        if (action == GLFW_RELEASE) keys[key] = false;
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    static GLfloat lastX = 400, lastY = 300;
    static bool firstMouse = true;

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    GLfloat xoffset = xpos - lastX;
    GLfloat yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}
