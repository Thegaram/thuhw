// g++ --std=c++11 -lglfw3 -lglad -lsoil -Wall -O3 -o main main.cpp

#include <cassert>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// SOIL
#include <SOIL.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

// window dimensions
GLuint WIDTH = 800, HEIGHT = 600;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

GLFWwindow* initGL(const std::string& title)
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return nullptr;
    }

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_RESIZABLE,GL_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize glad" << std::endl;
        return nullptr;
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    return window;
}

GLuint loadShader(const std::string& path, GLuint type)
{
    assert(type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER);

    // create shader
    GLuint shaderID = glCreateShader(type);

    // read code from file
    std::ifstream is(path);
    if (!is.is_open())
    {
        std::cerr << "Unable to open shader file " << path << std::endl;
        return 0;
    }

    std::stringstream ss;
    ss << is.rdbuf();
    std::string code = ss.str();

    // compile shader
    // std::cout << "Compiling shader " << path << std::endl;
    char const* sourcePointer = code.c_str();
    glShaderSource(shaderID, 1, &sourcePointer, nullptr);
    glCompileShader(shaderID);

    // check errors
    int infoLogLength;
    glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);

    if (infoLogLength > 0)
    {
        std::string errorMsg(infoLogLength + 1, 0);
        glGetShaderInfoLog(shaderID, infoLogLength, nullptr, &errorMsg[0]);
        std::cerr << errorMsg << std::endl;
        return 0;
    }

    return shaderID;
}

GLuint createProgram(const std::string& vertex_shader_path, const std::string& fragment_shader_path)
{
    GLuint vertexShaderID = loadShader(vertex_shader_path, GL_VERTEX_SHADER);
    GLuint fragmentShaderID = loadShader(fragment_shader_path, GL_FRAGMENT_SHADER);

    // TODO: handle errors

    // link the program
    // std::cout << "Linking program..." << std::endl;
    GLuint programID = glCreateProgram();
    glAttachShader(programID, vertexShaderID);
    glAttachShader(programID, fragmentShaderID);
    glLinkProgram(programID);

    // check the program
    int infoLogLength;
    glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);

    if (infoLogLength > 0)
    {
        std::string errorMsg(infoLogLength + 1, 0);
        glGetShaderInfoLog(programID, infoLogLength, nullptr, &errorMsg[0]);
        std::cerr << errorMsg << std::endl;
        return 0;
    }

    glDetachShader(programID, vertexShaderID);
    glDetachShader(programID, fragmentShaderID);

    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);

    return programID;
}

float randomFloat(float from, float to)
{
    static std::random_device dev;
    static std::mt19937 gen(dev());
    static std::uniform_real_distribution<float> dist(0.0, 1.0);
    assert(from < to);
    return dist(gen) * (to - from) + from;
}

class Background
{
private:
    GLuint programID;
    GLuint VBO, VAO, EBO;
    GLuint texID;

    GLfloat vertices[4 * 4] = {
        // positions   / texture Coords
         0.5f,  0.5f,  1.0f,  1.0f, // top Right
         0.5f, -0.5f,  1.0f,  0.0f, // bottom Right
        -0.5f, -0.5f,  0.0f,  0.0f, // bottom Left
        -0.5f,  0.5f,  0.0f,  1.0f  // top Left
    };

    GLuint indices[2 * 3] = {
        0, 1, 3, // first Triangle
        1, 2, 3  // second Triangle
    };

public:
    Background(const std::string& path, const std::string& vShader, const std::string& fShader)
    {
        programID = createProgram(vShader, fShader);

        // generate buffers
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glGenTextures(1, &texID);

        glBindVertexArray(VAO);

        // load vertices
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // load triangles
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // set position attribute
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);

        // set texCoord attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, texID);

        // set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // set texture filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // load texture and generate mipmaps
        int width, height;
        auto image = SOIL_load_image(path.c_str(), &width, &height, 0, SOIL_LOAD_RGB);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);
        SOIL_free_image_data(image);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void draw() const
    {
        glUseProgram(programID);

        // load texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);
        glUniform1i(glGetUniformLocation(programID, "ourTexture"), 0);

        // set scaling
        glUniform1f(glGetUniformLocation(programID, "width"), 2.0f);
        glUniform1f(glGetUniformLocation(programID, "height"), 2.0f);

        // draw
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

class Snowfall
{
private:
    GLuint programID;
    GLuint VAO;
    GLuint meshVBO, posVBO, scaleVBO;
    GLuint texID;

    const GLfloat vertices[6 * 4] = {
        // positions   / texture Coords
         0.5f,  0.5f,  1.0f,  1.0f, // top Right
         0.5f, -0.5f,  1.0f,  0.0f, // bottom Right
        -0.5f,  0.5f,  0.0f,  1.0f, // top Left

         0.5f, -0.5f,  1.0f,  0.0f, // bottom Right
        -0.5f,  0.5f,  0.0f,  1.0f, // top Left
        -0.5f, -0.5f,  0.0f,  0.0f  // bottom Left
    };

public:
    Snowfall(const std::string& texture, const std::string& vShader, const std::string& fShader)
    {
        programID = createProgram(vShader, fShader);

        // generate buffers
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &meshVBO);
        glGenBuffers(1, &posVBO);
        glGenBuffers(1, &scaleVBO);
        glGenTextures(1, &texID);

        glBindVertexArray(VAO);

        // load mesh
        glBindBuffer(GL_ARRAY_BUFFER, meshVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // vertexCoord attribute
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);

        // texCoord attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);

        // load positions
        glBindBuffer(GL_ARRAY_BUFFER, posVBO);
        glBufferData(GL_ARRAY_BUFFER, position.size() * sizeof(glm::vec2), &position[0], GL_STREAM_DRAW);

        // position attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
        glEnableVertexAttribArray(2);

        // load scales
        glBindBuffer(GL_ARRAY_BUFFER, scaleVBO);
        glBufferData(GL_ARRAY_BUFFER, scale.size() * sizeof(float), &scale[0], GL_STREAM_DRAW);

        // scale attribute
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
        glEnableVertexAttribArray(3);

        // set attribute divisors
        glVertexAttribDivisor(0, 0); // vertexCoord: always reuse the same 6 vertices
        glVertexAttribDivisor(1, 0); // texCoord   : always reuse the same 6 vertices
        glVertexAttribDivisor(2, 1); // position   : advance at normal rate (once per quad)
        glVertexAttribDivisor(3, 1); // scale      : advance at normal rate (once per quad)

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, texID);

        // set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // set texture filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // load texture and generate mipmaps
        int width, height;
        auto image = SOIL_load_image(texture.c_str(), &width, &height, 0, SOIL_LOAD_RGB);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);
        SOIL_free_image_data(image);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    ~Snowfall()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &meshVBO);
        glDeleteBuffers(1, &posVBO);
        glDeleteBuffers(1, &scaleVBO);
    }

    void draw()
    {
        glUseProgram(programID);

        // load texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);
        glUniform1i(glGetUniformLocation(programID, "ourTexture"), 0);

        // load positions
        glBindBuffer(GL_ARRAY_BUFFER, posVBO);
        glBufferData(GL_ARRAY_BUFFER, position.size() * sizeof(glm::vec2), &position[0], GL_STREAM_DRAW);

        // load scales
        glBindBuffer(GL_ARRAY_BUFFER, scaleVBO);
        glBufferData(GL_ARRAY_BUFFER, scale.size() * sizeof(float), &scale[0], GL_STREAM_DRAW);

        // draw
        glBindVertexArray(VAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, position.size());
        glBindVertexArray(0);
    }

private:
    float time = 0.0f;
    float rate = 5.0f;
    float last = 0.0f;

    std::vector<glm::vec2> position;
    std::vector<glm::vec2> velocity;
    std::vector<float> scale;

public:
    int nextPos()
    {
        for (int ii = 0; ii < position.size(); ++ii)
            if (position[ii].y < -1.1f)
                return ii;

        position.push_back({});
        velocity.push_back({});
        scale.push_back({});
        return position.size() - 1;
    }

    void spawnSingle()
    {
        auto ii = nextPos();
        position[ii] = glm::vec2(randomFloat(-1.0f, +1.0f), 1.1f);
        velocity[ii] = glm::vec2(0.0f, randomFloat(-0.6f, -0.2f));
        scale[ii]    = randomFloat(0.05f, 0.15f);
    }

    void spawn()
    {
        if (time - last > 1.0f / rate)
        {
            spawnSingle();
            last = time;
        }
    }

    void step(float dt)
    {
        // advance time
        time += dt;

        // spawn new particles
        spawn();

        // step particles
        for (int ii = 0; ii < position.size(); ++ii)
            position[ii] += velocity[ii] * dt;
    }

    void increaseRate() { rate += 0.1f; }
    // void decreaseRate() { rate -= 0.1f; }
};

int main()
{
    GLFWwindow* window = initGL("Assignment 04");
    if (!window) return -1;

    Background bg("./textures/background.png", "./shaders/background.vert.glsl", "./shaders/background.frag.glsl");
    Snowfall   sf("./textures/snowflake.png" , "./shaders/snowflake.vert.glsl" , "./shaders/snowflake.frag.glsl");

    GLfloat lastFrame = glfwGetTime();
    int counter = 0;

    while (!glfwWindowShouldClose(window))
    {
        GLfloat currentFrame = glfwGetTime();
        GLfloat dt = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        // update
        if (counter++ % 20 == 0) sf.increaseRate();
        sf.step(dt);

        // draw
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        bg.draw();
        sf.draw();

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
