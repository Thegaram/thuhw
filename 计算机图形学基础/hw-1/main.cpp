// g++ --std=c++11 -lglfw3 -lglad -Wall main.cpp

#include <cassert>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>


bool keys[1024];

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);


class Camera
{
private:
    glm::mat4 view;

    Camera()
    {
        view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 3.0f), // camera location
            glm::vec3(0.0f, 0.0f, 0.0f), // look at origin
            glm::vec3(0.0f, 1.0f, 0.0f)  // head up
        );
    }

public:
    static Camera& instance()
    {
        static Camera instance;
        return instance;
    }

    glm::mat4 getView() const
    {
        return view;
    }
};

class ViewPort
{
private:
    GLuint width, height;
    glm::mat4 projection;

    ViewPort()
    {
        width = 800;
        height = 600;
        projection = glm::perspective(glm::radians(45.0f), (float) width / height, 0.1f, 100.0f);
    }

public:
    static ViewPort& instance()
    {
        static ViewPort instance;
        return instance;
    }

    void resize(GLuint newWidth, GLuint newHeight)
    {
        width = newWidth;
        height = newHeight;
        projection = glm::perspective(glm::radians(45.0f), (float) width / height, 0.1f, 100.0f);
        glViewport(0, 0, width, height);
    }

    GLuint getWidth() const { return width; }
    GLuint getHeight() const { return height; }

    glm::mat4 getProjection() const
    {
        return projection;
    }
};

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

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
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

class Model
{
private:
    enum class RenderMode {
        WIREFRAME,
        VERTEX,
        FACE,
        FACE_EDGE
    };

    GLuint programID;

    GLuint vertextArrayID; // VAO
    GLuint vertexBufferID; // VBO
    GLuint colorBufferID;  // VBO

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> colors;

    glm::mat4 T; // translation
    glm::mat4 R; // rotation
    glm::mat4 S; // scaling

    RenderMode renderMode = RenderMode::FACE;

    void readOBJ(const std::string& path)
    {
        assert(vertices.empty());

        std::vector<glm::vec3> vs;
        std::vector<std::tuple<unsigned int, unsigned int, unsigned int>> faces;

        std::ifstream infile(path);

        std::string line;
        while(std::getline(infile, line))
        {
            char type;
            std::istringstream iss(line);
            iss >> type; // assume no error

            if (type == '#') continue;

            if (type == 'v')
            {
                float x, y, z;
                iss >> x >> y >> z;
                vs.push_back(glm::vec3(x, y, z));
            }

            else if (type == 'f')
            {
                unsigned int v1, v2, v3;
                iss >> v1 >> v2 >> v3;
                faces.push_back(std::make_tuple(v1 - 1, v2 - 1, v3 - 1));
            }
        }

        for (auto face : faces)
        {
            // auto [v1, v2, v3] = face;
            auto v1 = std::get<0>(face);
            auto v2 = std::get<1>(face);
            auto v3 = std::get<2>(face);

            vertices.push_back(vs[v1]);
            vertices.push_back(vs[v2]);
            vertices.push_back(vs[v3]);
        }
    }

public:
    void translate(glm::vec3 trans)
    {
        T = glm::translate(T, trans);
    }

    void rotate(GLfloat degrees, glm::vec3 axis)
    {
        R = glm::rotate(R, glm::radians(degrees), axis);
    }

    void stepMode()
    {
        switch (renderMode)
        {
            case RenderMode::FACE:      renderMode = RenderMode::FACE_EDGE; break;
            case RenderMode::FACE_EDGE: renderMode = RenderMode::WIREFRAME; break;
            case RenderMode::WIREFRAME: renderMode = RenderMode::VERTEX   ; break;
            case RenderMode::VERTEX:    renderMode = RenderMode::FACE     ; break;
        }
    }

    void generate_colors()
    {
        assert(vertices.size() != 0);
        colors.clear();

        std::random_device dev;
        std::mt19937 gen(dev());
        std::uniform_real_distribution<float> dist(0.0, 1.0);

        for (int ii = 0; ii < vertices.size(); ii += 3)
        {
            float r = dist(gen);
            float g = dist(gen);
            float b = dist(gen);

            colors.push_back(glm::vec3(r, g, b));
            colors.push_back(glm::vec3(r, g, b));
            colors.push_back(glm::vec3(r, g, b));
        }
    }

    Model(const std::string& obj_path, const std::string& vertex_shader_path, const std::string& fragment_shader_path)
    {
        readOBJ(obj_path);
        generate_colors();

        programID = createProgram(vertex_shader_path, fragment_shader_path);

        glGenVertexArrays(1, &vertextArrayID);
        glBindVertexArray(vertextArrayID);

        glGenBuffers(1, &vertexBufferID);
        glGenBuffers(1, &colorBufferID);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
        glVertexAttribPointer(
            0,        //
            3,        // size
            GL_FLOAT, // type
            GL_FALSE, // normalized?
            0,        // stride
            (void*) 0 // array buffer offset
        );

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), &colors[0], GL_STATIC_DRAW);
        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            0,
            (void*) 0
        );
    }

    void setPolygonMode(RenderMode mode) const
    {
        GLuint overrideId = glGetUniformLocation(programID, "override");

        switch (mode)
        {
            case RenderMode::WIREFRAME:
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glLineWidth(5.0f);
                glUniform1i(overrideId, GL_TRUE);
                break;

            case RenderMode::VERTEX:
                glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
                glPointSize(5.0f);
                glUniform1i(overrideId, GL_TRUE);
                break;

            case RenderMode::FACE: // fallthrough
            case RenderMode::FACE_EDGE:
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glUniform1i(overrideId, GL_FALSE);
                break;
        }
    }

    void draw(glm::mat4 projection, glm::mat4 view) const
    {
        glUseProgram(programID);
        glBindVertexArray(vertextArrayID);

        // set model-view-projection matrix
        glm::mat4 model = T * R * S * glm::mat4(1.0f);
        GLuint matrixID = glGetUniformLocation(programID, "MVP");
        glm::mat4 mvp = projection * view * model;
        glUniformMatrix4fv(matrixID, 1, GL_FALSE, &mvp[0][0]);

        // load vertex and color data
        glEnableVertexAttribArray(0);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), &colors[0], GL_STATIC_DRAW);

        // set polygon mode
        setPolygonMode(renderMode);

        // draw
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());

        if (renderMode == RenderMode::FACE_EDGE)
        {
            setPolygonMode(RenderMode::WIREFRAME);
            glDrawArrays(GL_TRIANGLES, 0, vertices.size());
        }
    }

    ~Model()
    {
        glDeleteVertexArrays(1, &vertextArrayID);
        glDeleteBuffers(1, &vertexBufferID);
        glDeleteBuffers(1, &colorBufferID);
    }
};

void updateModel(Model& model)
{
    GLfloat t_speed = 0.02f;
    GLfloat r_speed = 10.0f;

    if (keys[GLFW_KEY_J]) model.translate(glm::vec3(-1.0f,  0.0f,  0.0f) * t_speed);
    if (keys[GLFW_KEY_L]) model.translate(glm::vec3(+1.0f,  0.0f,  0.0f) * t_speed);
    if (keys[GLFW_KEY_I]) model.translate(glm::vec3( 0.0f, +1.0f,  0.0f) * t_speed);
    if (keys[GLFW_KEY_K]) model.translate(glm::vec3( 0.0f, -1.0f,  0.0f) * t_speed);
    if (keys[GLFW_KEY_U]) model.translate(glm::vec3( 0.0f,  0.0f, -1.0f) * t_speed);
    if (keys[GLFW_KEY_O]) model.translate(glm::vec3( 0.0f,  0.0f, +1.0f) * t_speed);

    if (keys[GLFW_KEY_W]) model.rotate(-r_speed, glm::vec3(1.0f, 0.0f, 0.0f));
    if (keys[GLFW_KEY_S]) model.rotate(+r_speed, glm::vec3(1.0f, 0.0f, 0.0f));
    if (keys[GLFW_KEY_A]) model.rotate(-r_speed, glm::vec3(0.0f, 1.0f, 0.0f));
    if (keys[GLFW_KEY_D]) model.rotate(+r_speed, glm::vec3(0.0f, 1.0f, 0.0f));
    if (keys[GLFW_KEY_Q]) model.rotate(+r_speed, glm::vec3(0.0f, 0.0f, 1.0f));
    if (keys[GLFW_KEY_E]) model.rotate(-r_speed, glm::vec3(0.0f, 0.0f, 1.0f));

    if (keys[GLFW_KEY_SPACE])
    {
        model.stepMode();
        keys[GLFW_KEY_SPACE] = false;
    }

    if (keys[GLFW_KEY_X])
    {
        model.generate_colors();
        keys[GLFW_KEY_X] = false;
    }
}

void renderLoop(GLFWwindow* window)
{
    auto model = Model("./eight.uniform.obj", "./vertex.glsl", "./fragment.glsl");

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        updateModel(model);

        glm::mat4 projection = ViewPort::instance().getProjection();
        glm::mat4 view = Camera::instance().getView();

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        model.draw(projection, view);

        glfwSwapBuffers(window);
    }
}

int main()
{
    GLFWwindow* window = initGL(ViewPort::instance().getWidth(), ViewPort::instance().getHeight(), "Assignment 01");
    if (!window) return -1;

    std::cout << "Switch modes: SPACE" << std::endl;
    std::cout << "Change color: X" << std::endl << std::endl;

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

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    ViewPort::instance().resize(width, height);
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