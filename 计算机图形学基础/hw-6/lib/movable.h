#ifndef MOVABLE_H_INCLUDED
#define MOVABLE_H_INCLUDED

#include <glm/glm.hpp>

class Movable
{
private:
    glm::mat4 T; // translation
    glm::mat4 R; // rotation
    glm::mat4 S; // scaling

public:
    void translate(glm::vec3 trans)
    {
        T = glm::translate(T, trans);
    }

    void rotate(GLfloat degrees, glm::vec3 axis)
    {
        R = glm::rotate(R, glm::radians(degrees), axis);
    }

    glm::mat4 model() const
    {
        return T * R * S * glm::mat4(1.0f);
    }

    virtual ~Movable() {};
};

#endif // MOVABLE_H_INCLUDED