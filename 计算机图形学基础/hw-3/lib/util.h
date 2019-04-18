#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <glm/glm.hpp>

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string>

// TODO: include gl

#define EPS 1e-5

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);


std::ostream& operator<<(std::ostream& os, glm::vec3 vec)
{
    os << "{" << vec.x << ", " << vec.y << ", " << vec.z << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, glm::vec4 vec)
{
    os << "{" << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << "}";
    return os;
}


namespace util
{
bool keys[1024];

bool isUnit(glm::vec3 vec)
{
    return glm::length(vec) - 1.0f < EPS;
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

GLuint loadTexture(const std::string& path)
{
    GLuint texID;

    // generate texture buffer
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    // set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // set texture filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // load texture and generate mipmaps
    int width, height;
    auto image = SOIL_load_image(path.c_str(), &width, &height, 0, SOIL_LOAD_RGB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    SOIL_free_image_data(image);

    // unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}

GLFWwindow* initGL(GLuint width, GLuint height, const std::string& title)
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return nullptr;
    }

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (window == nullptr)
    {
        std::cerr << "Failed to open GLFW window\n";
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize glad" << std::endl;
        return nullptr;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return window;
}

}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)        util::keys[key] = true;
        else if (action == GLFW_RELEASE) util::keys[key] = false;
    }
}

#endif // UTIL_H_INCLUDED