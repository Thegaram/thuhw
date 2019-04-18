#ifndef VIEWPORT_H_INCLUDED
#define VIEWPORT_H_INCLUDED

class Viewport
{
private:
    static GLuint width, height;

public:
    static void resize(GLuint newWidth, GLuint newHeight)
    {
        width = newWidth;
        height = newHeight;
        glViewport(0, 0, width, height);
    }

    static GLuint getWidth() { return width; }
    static GLuint getHeight() { return height; }

    static glm::mat4 projectionMatrix()
    {
        return glm::perspective(glm::radians(45.0f), (float) width / height, 0.1f, 500.0f);;
    }
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    Viewport::resize(width, height);
}

GLuint Viewport::width = 800;
GLuint Viewport::height = 600;

#endif // VIEWPORT_H_INCLUDED