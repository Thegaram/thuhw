// g++ --std=c++11 -lglfw3 -lglad -Wall main.cpp

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

// others
#include "lib/movable.h"
#include "lib/objmodel.h"
#include "lib/util.h"
#include "lib/wireframe.h"

#include "lib/camera.h"


const GLuint WIDTH = 800;
const GLuint HEIGHT = 600;

glm::vec3 lightPos(0.8f, 1.0f, -0.2f);
// glm::vec3 cameraPos(0.0f, 0.0f, 3.0f);

// glm::mat4 viewMatrix = glm::lookAt(
//     cameraPos,                   // camera location
//     glm::vec3(0.0f, 0.0f, 0.0f), // look at origin
//     glm::vec3(0.0f, 1.0f, 0.0f)  // head up
// );

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

glm::mat4 projectionMatrix = glm::perspective(
    glm::radians(45.0f),
    (float) WIDTH / HEIGHT,
    0.1f,
    100.0f
);

class Model : public Movable, public Wireframe
{
private:
    GLuint program;

    GLuint VAO;
    GLuint VBO;
    GLuint EBO;

    std::vector<GLfloat> data;
    std::vector<GLuint> indices;

    float colorCoeff = 0.0f;

private:
    void readData(const std::string& objPath)
    {
        auto myOBJ = OBJModel(objPath);
        myOBJ.calculateNormals();

        const auto& vertices = myOBJ.vertices();
        const auto& faces    = myOBJ.faces();
        const auto& normals  = myOBJ.normals();

        for (int v = 0; v < vertices.size(); ++v)
        {
            data.push_back(vertices[v].x);
            data.push_back(vertices[v].y);
            data.push_back(vertices[v].z);
            data.push_back(normals[v].x);
            data.push_back(normals[v].y);
            data.push_back(normals[v].z);
        }

        for (int f = 0; f < faces.size(); ++f)
        {
            indices.push_back(faces[f].x);
            indices.push_back(faces[f].y);
            indices.push_back(faces[f].z);
        }
    }

public:

    Model(const std::string& objPath, const std::string& vshaderPath, const std::string& fshaderPath)
    {
        readData(objPath);

        program = createProgram(vshaderPath, fshaderPath);

        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(GLfloat), &data[0], GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*) 0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*) (3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);
    }

    void stepColor(float x)
    {
        colorCoeff += x;
    }

    glm::vec3 color() const
    {
        float r = 0.5f * sin(glm::radians(colorCoeff))          + 0.5f;
        float g = 0.5f * sin(glm::radians(colorCoeff + 120.0f)) + 0.5f;
        float b = 0.5f * sin(glm::radians(colorCoeff + 240.0f)) + 0.5f;
        return {r, g, b};
    }

    void draw(glm::mat4 P, glm::mat4 V) const
    {
        glUseProgram(program);
        glBindVertexArray(VAO);

        auto c = color();
        auto M = model();

        glUniformMatrix4fv(glGetUniformLocation(program, "M"), 1, GL_FALSE, &M[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(program, "V"), 1, GL_FALSE, &V[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(program, "P"), 1, GL_FALSE, &P[0][0]);

        glUniform3f(glGetUniformLocation(program, "objectColor"), c.r, c.g, c.b);
        glUniform3f(glGetUniformLocation(program, "lightColor"),  1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(program, "lightPos"),    lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(program, "viewPos"),     camera.Position.x, camera.Position.y, camera.Position.z);

        setPolygonMode(renderMode);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (void*) 0);
    }

    ~Model()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
};

void updateModel(Model& model, float dt)
{
    GLfloat t_speed = 2.5f;
    GLfloat r_speed = 500.0f;

    if (keys[GLFW_KEY_J]) model.translate(glm::vec3(-1.0f,  0.0f,  0.0f) * t_speed * dt);
    if (keys[GLFW_KEY_L]) model.translate(glm::vec3(+1.0f,  0.0f,  0.0f) * t_speed * dt);
    if (keys[GLFW_KEY_I]) model.translate(glm::vec3( 0.0f, +1.0f,  0.0f) * t_speed * dt);
    if (keys[GLFW_KEY_K]) model.translate(glm::vec3( 0.0f, -1.0f,  0.0f) * t_speed * dt);
    if (keys[GLFW_KEY_U]) model.translate(glm::vec3( 0.0f,  0.0f, -1.0f) * t_speed * dt);
    if (keys[GLFW_KEY_O]) model.translate(glm::vec3( 0.0f,  0.0f, +1.0f) * t_speed * dt);

    if (keys[GLFW_KEY_W]) model.rotate(-r_speed * dt, glm::vec3(1.0f, 0.0f, 0.0f));
    if (keys[GLFW_KEY_S]) model.rotate(+r_speed * dt, glm::vec3(1.0f, 0.0f, 0.0f));
    if (keys[GLFW_KEY_A]) model.rotate(-r_speed * dt, glm::vec3(0.0f, 1.0f, 0.0f));
    if (keys[GLFW_KEY_D]) model.rotate(+r_speed * dt, glm::vec3(0.0f, 1.0f, 0.0f));
    if (keys[GLFW_KEY_Q]) model.rotate(+r_speed * dt, glm::vec3(0.0f, 0.0f, 1.0f));
    if (keys[GLFW_KEY_E]) model.rotate(-r_speed * dt, glm::vec3(0.0f, 0.0f, 1.0f));

    if (keys[GLFW_KEY_X]) model.stepColor(100.0f * dt);

    if (keys[GLFW_KEY_SPACE])
    {
        model.stepMode();
        keys[GLFW_KEY_SPACE] = false;
    }
}

void renderLoop(GLFWwindow* window)
{
    auto model = Model("models/eight.uniform.obj", "shaders/main.vert.glsl", "shaders/main.frag.glsl");

    float t0 = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        float t1 = glfwGetTime();
        float dt = t1 - t0;
        t0 = t1;

        glfwPollEvents();
        updateModel(model, dt);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        model.draw(projectionMatrix, camera.GetViewMatrix());

        glfwSwapBuffers(window);
    }
}

int main()
{
    GLFWwindow* window = initGL(WIDTH, HEIGHT, "Assignment 06");
    if (!window) return -1;

    std::cout << "Switch modes: SPACE" << std::endl;
    std::cout << "Change color: X" << std::endl;

    std::cout << "Rotate model (around model axis):" << std::endl;
    std::cout << "  X-axis: W-S" << std::endl;
    std::cout << "  Y-axis: A-D" << std::endl;
    std::cout << "  Z-axis: Q-E" << std::endl << std::endl;

    std::cout << "Translate model (along model axis):" << std::endl;
    std::cout << "  X-axis: J-L" << std::endl;
    std::cout << "  Y-axis: I-K" << std::endl;
    std::cout << "  Z-axis: U-O" << std::endl << std::endl;

    renderLoop(window);

    glfwTerminate();
    return 0;
}
