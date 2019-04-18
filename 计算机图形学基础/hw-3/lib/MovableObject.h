#ifndef MOVABLE_OBJECT_H_INCLUDED
#define MOVABLE_OBJECT_H_INCLUDED

#include <glm/glm.hpp>

class MovableObject
{
private:
    glm::mat4 T; // translation
    glm::mat4 R; // rotation
    glm::mat4 S; // scaling

    glm::mat4 M; // model (for setting manually)

protected:
    glm::mat4 model() const
    {
        return M * T * R * S * glm::mat4(1.0f);
    }

public:
    void translate(glm::vec3 trans)
    {
        M = glm::mat4(1.0f);
        T = glm::translate(T, trans);
    }

    void resetTranslate()
    {
        T = glm::mat4(1.0f);
    }

    void rotate(GLfloat degrees, glm::vec3 axis)
    {
        M = glm::mat4(1.0f);
        R = glm::rotate(R, glm::radians(degrees), axis);
    }

    void setRotate(glm::mat4 newR)
    {
        M = glm::mat4(1.0f);
        R = newR;
    }

    void scale(glm::vec3 sc)
    {
        M = glm::mat4(1.0f);
        S = glm::scale(S, sc);
    }

    void setModel(glm::mat4 m)
    {
        T = S = R = glm::mat4(1.0f);
        M = m;
    }

    glm::vec3 pos() const
    {
        glm::vec4 pos(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec4 x = model() * pos;
        return glm::vec3(x);
    }

    glm::vec3 top() const
    {
        glm::vec4 top(0.0f, 1.0f, 0.0f, 1.0f); // assume top is this coordinate in model space
        glm::vec4 x = model() * top;
        return glm::vec3(x);
    }
};

#endif // MOVABLE_OBJECT_H_INCLUDED