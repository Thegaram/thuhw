#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

bool keys[1024];

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

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

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

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
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize glad" << std::endl;
        return nullptr;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

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

GLuint createProgram(const std::string& vshaderPath, const std::string& fshaderPath)
{
    GLuint vertexShaderID = loadShader(vshaderPath, GL_VERTEX_SHADER);
    GLuint fragmentShaderID = loadShader(fshaderPath, GL_FRAGMENT_SHADER);

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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)        keys[key] = true;
        else if (action == GLFW_RELEASE) keys[key] = false;
    }
}

#endif // UTIL_H_INCLUDED