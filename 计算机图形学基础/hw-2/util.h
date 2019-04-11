#ifndef ASSIGNMENT_2_UTIL_H
#define ASSIGNMENT_2_UTIL_H

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// window dimensions
GLuint WIDTH = 800, HEIGHT = 600;

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

inline float fdiv(float a, float b)
{
    return floor(a / b);
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

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    WIDTH = width;
    HEIGHT = height;
    glViewport(0, 0, width, height);
}

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

    // glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
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

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
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

}

#endif // ASSIGNMENT_2_UTIL_H