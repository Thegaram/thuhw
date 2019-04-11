// g++ --std=c++11 -lglfw3 -lglad -lsoil -Wall -O3 -o main main.cpp

#include <cassert>
#include <cmath>
#include <iostream>
#include <list>
#include <string>

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// SOIL
#include <SOIL.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

// other includes
#include "util.h"


GLfloat dt = 0.0f; // time between current frame and last frame
GLfloat lastFrame = 0.0f; // time of last frame

namespace Params
{
    float SCALE = 0.20f;             // scaling factor of each particle
    float ANGLE = 22.5f;             // emit angles in degrees
    float MAX_DISTANCE = 1.0f;       // distance after which particles are destroyed
    float P_VEL = 0.10f;             // velocity of each particle
    float R_VEL = 200.0f;            // circular velocity of emitter
    float START_ALPHA = 0.8f;        // starting alpha color component of each particle
    float DISPLACEMENT = 0.05f;      // starting displacement of each particle
    auto EMIT_POS = glm::vec3(0.0f); // position of emitter
};


class Particle
{
    // note: most resources are shared among particles

private:
    static bool initialized;

    static GLuint programID;
    static GLuint VBO, VAO, EBO;
    static GLuint texID;

    static constexpr GLfloat vertices[] = {
        // positions          // texture Coords
         0.5f,  0.5f,  0.0f,  1.0f,  1.0f, // top Right
         0.5f, -0.5f,  0.0f,  1.0f,  0.0f, // bottom Right
        -0.5f, -0.5f,  0.0f,  0.0f,  0.0f, // bottom Left
        -0.5f,  0.5f,  0.0f,  0.0f,  1.0f  // top Left
    };

    static constexpr GLuint indices[] = {
        0, 1, 3, // first Triangle
        1, 2, 3  // second Triangle
    };

    static void init_vertices()
    {
        // generate buffers
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // load vertices
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // load triangles
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // set position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);

        // set texCoord attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);

        // unbind VAO
        glBindVertexArray(0);
    }

    static void init_textures()
    {
        // generate texture buffer
        glGenTextures(1, &texID);

        glBindTexture(GL_TEXTURE_2D, texID);

        // set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // set texture filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // load texture and generate mipmaps
        int width, height;
        auto image = SOIL_load_image("star.bmp", &width, &height, 0, SOIL_LOAD_RGB);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);
        SOIL_free_image_data(image);

        // unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);
    }

public:
    static void init(const std::string& tex_path, const std::string& vertex_shader_path, const std::string& fragment_shader_path)
    {
        assert(!initialized);

        // load shaders
        programID = util::createProgram(vertex_shader_path, fragment_shader_path);

        // initialize everything
        init_vertices();
        init_textures();

        initialized = true;
    }

    void static release()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteTextures(1, &texID);
    }

private:
    // particle-specific properties
    glm::vec4 color;
    glm::vec3 pos, vel;
    float scale;

public:
    Particle(glm::vec4 color, glm::vec3 pos, glm::vec3 vel, float scale): color(color), pos(pos), vel(vel), scale(scale)
    {
        assert(Particle::initialized);
    }

    float distanceFrom(glm::vec3 point) const
    {
        return glm::length(pos - point);
    }

    void step(float dt)
    {
        // update position
        pos += vel * dt;

        // update alpha (so that it fades gradually)
        color.w = Params::START_ALPHA * (Params::MAX_DISTANCE - distanceFrom(Params::EMIT_POS)) / Params::MAX_DISTANCE;
    }

    void draw() const
    {
        // note: possible optimization
        // only load VBO and texture once, since we use
        // the same buffers throughout. having tried,
        // this seems to bring no speed-up

        glUseProgram(programID);

        // load texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);
        glUniform1i(glGetUniformLocation(programID, "ourTexture"), 0);

        // set color
        glUniform4f(glGetUniformLocation(programID, "overrideColor"), color.r, color.g, color.b, color.a);

        // set scale and position
        glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), pos), glm::vec3(scale));
        glUniformMatrix4fv(glGetUniformLocation(programID, "M"), 1, GL_FALSE, &model[0][0]);

        // draw
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

bool Particle::initialized = false;
GLuint Particle::programID;
GLuint Particle::VBO;
GLuint Particle::VAO;
GLuint Particle::EBO;
GLuint Particle::texID;
constexpr GLfloat Particle::vertices[];
constexpr GLuint Particle::indices[];

class ParticleSystem
{
private:
    float omega = 0.0f; // rotation angle in degrees
    std::list<Particle> particles;

    Particle& emit(float angle)
    {
        // std::cout << "emitting @ " << angle << std::endl;

        // generate components using overlapping sines
        float r = 0.5f * sin(glm::radians(angle))          + 0.5f;
        float g = 0.5f * sin(glm::radians(angle + 120.0f)) + 0.5f;
        float b = 0.5f * sin(glm::radians(angle + 240.0f)) + 0.5f;
        auto color = glm::vec4(r, g, b, Params::START_ALPHA);

        // calculate velocity
        auto vel = glm::vec4(0.0f, Params::P_VEL, 0.0f, 1.0f);
        vel = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, -1.0f)) * vel;
        auto vel3d = glm::vec3(vel);

        // calculate position
        auto pos = Params::EMIT_POS;
        pos += vel3d * Params::DISPLACEMENT;

        // generate particle
        particles.push_front({color, pos, vel3d, Params::SCALE});
        return particles.front();
    }

    void rotateAndEmit(float delta_omega)
    {
        float omega_original = omega;

        // while delta_omega makes us step over sector boundaries, we emit a new particle
        while (util::fdiv(omega, Params::ANGLE) < util::fdiv(omega + delta_omega, Params::ANGLE))
        {
            // calculate next sector boundary
            float emit_omega = util::fdiv(omega + Params::ANGLE, Params::ANGLE) * Params::ANGLE;

            // emit particle
            Particle& p = emit(emit_omega);

            // update loop parameters
            omega += Params::ANGLE;
            delta_omega -= Params::ANGLE;

            // compensate for the subsequent step(dt) for this particle:
            //    we step for dt, but we should step for a shorter time
            //    because the particle was emitted later
            p.step(- (emit_omega - omega_original) / Params::R_VEL);
        }

        omega += delta_omega;
    }

public:
    void step(float dt)
    {
        // emit new particles
        rotateAndEmit(Params::R_VEL * dt);

        // update each particle and delete the old ones
        auto it = std::begin(particles);
        while (it != std::end(particles))
        {
            auto& p = *it;
            p.step(dt);

            if (p.distanceFrom(Params::EMIT_POS) > Params::MAX_DISTANCE)
                particles.erase(it++);
            else
                it++;
        }
    }

    void draw() const
    {
        for (auto& p : particles)
            p.draw();
    }
};

int main()
{
    GLFWwindow* window = util::initGL("Assignment 02");
    if (!window) return -1;

    Particle::init("star.bmp", "main.vert.glsl", "main.frag.glsl");
    ParticleSystem ps;

    lastFrame = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        GLfloat currentFrame = glfwGetTime();
        dt = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ps.step(dt);
        ps.draw();

        glfwSwapBuffers(window);
    }

    Particle::release();
    glfwTerminate();

    return 0;
}
