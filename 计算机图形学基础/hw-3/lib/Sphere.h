#ifndef SPHERE_H_INCLUDED
#define SPHERE_H_INCLUDED

#include "./util.h"
#include "./MovableObject.h"

#include <string>
#include <vector>

constexpr float PI = 3.14159265;

class Sphere : public MovableObject
{
    enum class RenderMode {
        WIREFRAME,
        FACE,
        FACE_EDGE
    };

private:
    static GLuint programID;

    static GLuint VAO; // VAO
    static GLuint vertexVBO; // VBO
    static GLuint textureVBO; // VBO
    static GLuint EBO;

    static std::vector<glm::vec3> vertices;
    static std::vector<glm::ivec3> faces;
    static std::vector<glm::vec2> texture_coords;

    static RenderMode renderMode;

    static void doubleVertices()
    {
        std::vector<glm::ivec3> new_faces;

        for (glm::ivec3 face : faces)
        {
            auto vec1_id = face.x;
            auto vec2_id = face.y;
            auto vec3_id = face.z;

            glm::vec3 vec1 = vertices[vec1_id];
            glm::vec3 vec2 = vertices[vec2_id];
            glm::vec3 vec3 = vertices[vec3_id];

            glm::vec3 vec12 = (vec1 + vec2) / 2.0f;
            glm::vec3 vec23 = (vec2 + vec3) / 2.0f;
            glm::vec3 vec31 = (vec3 + vec1) / 2.0f;

            auto vec12_id = vertices.size() + 0;
            auto vec23_id = vertices.size() + 1;
            auto vec31_id = vertices.size() + 2;

            vertices.push_back(vec12);
            vertices.push_back(vec23);
            vertices.push_back(vec31);

            new_faces.push_back({vec31_id, vec1_id, vec12_id});
            new_faces.push_back({vec12_id, vec2_id, vec23_id});
            new_faces.push_back({vec23_id, vec3_id, vec31_id});
            new_faces.push_back({vec12_id, vec23_id, vec31_id});

            // TODO: optimize (we add 2 copies of each new vertex)
        }

        faces = new_faces;
    }

    static void normalizeToSphere()
    {
        const glm::vec3 origin(0.0f, 0.0f, 0.0f);

        for (glm::vec3& vec : vertices)
        {
            auto dir = vec - origin;
            vec = origin + glm::normalize(dir);
        }
    }

    static void calculate_texture_coords()
    {
        texture_coords.clear();

        for (glm::vec3 vec : vertices)
        {
            float lat = 90.0f - acos(vec.y) * 180.0 / PI;
            float lon = fmod((270.0f + atan2(vec.x, vec.z) * 180.0f / PI), 360.0f) - 180.0f;

            float tex_x = (lon + 180.0f) / 360.0f;
            float tex_y = (lat +  90.0f) / 180.0f;

            texture_coords.push_back({tex_x, tex_y});
        }
    }

public:
    static void init(const std::string& vertex_shader_path, const std::string& fragment_shader_path)
    {
        // generate mesh approximating a sphere using procedural modeling
        doubleVertices();
        doubleVertices();
        doubleVertices();
        doubleVertices();
        normalizeToSphere();

        // initialize texture coordinates
        calculate_texture_coords();

        programID = util::createProgram(vertex_shader_path, fragment_shader_path);

        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glGenBuffers(1, &vertexVBO);
        glGenBuffers(1, &textureVBO);
        glGenBuffers(1, &EBO);

        glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
        glBufferData(GL_ARRAY_BUFFER, texture_coords.size() * sizeof(glm::vec2), &texture_coords[0], GL_STATIC_DRAW);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*) 0);
        glEnableVertexAttribArray(1);

        // load triangles
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(glm::ivec3), &faces[0], GL_STATIC_DRAW);
    }

    static void release()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &vertexVBO);
        glDeleteBuffers(1, &textureVBO);
    }

    static void stepRenderMode()
    {
        switch (renderMode)
        {
            case RenderMode::FACE:      renderMode = RenderMode::FACE_EDGE; break;
            case RenderMode::FACE_EDGE: renderMode = RenderMode::WIREFRAME; break;
            case RenderMode::WIREFRAME: renderMode = RenderMode::FACE   ; break;
        }
    }

private:
    GLuint texID;

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

            case RenderMode::FACE: // fallthrough
            case RenderMode::FACE_EDGE:
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glUniform1i(overrideId, GL_FALSE);
                break;
        }
    }

public:
    Sphere(const std::string& texture_path)
    {
        texID = util::loadTexture(texture_path);
    }

    void draw(glm::mat4 projection, glm::mat4 view) const
    {
        glUseProgram(programID);
        glBindVertexArray(VAO);

        // set model-view-projection matrix
        glm::mat4 M = model();
        glm::mat4 mvp = projection * view * M;
        glUniformMatrix4fv(glGetUniformLocation(programID, "MVP"), 1, GL_FALSE, &mvp[0][0]);

        // load vertex and color data
        glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
        glBufferData(GL_ARRAY_BUFFER, texture_coords.size() * sizeof(glm::vec2), &texture_coords[0], GL_STATIC_DRAW);

        // load triangles
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(glm::ivec3), &faces[0], GL_STATIC_DRAW);

        // set texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);
        glUniform1i(glGetUniformLocation(programID, "ourTexture"), 0);

        // set polygon mode
        setPolygonMode(renderMode);

        // draw
        glDrawElements(GL_TRIANGLES, faces.size() * 3, GL_UNSIGNED_INT, 0);

        if (renderMode == RenderMode::FACE_EDGE)
        {
            setPolygonMode(RenderMode::WIREFRAME);
            glDrawElements(GL_TRIANGLES, faces.size() * 3, GL_UNSIGNED_INT, 0);
        }
    }
};

GLuint Sphere::programID;
GLuint Sphere::VAO;
GLuint Sphere::vertexVBO;
GLuint Sphere::textureVBO;
GLuint Sphere::EBO;

Sphere::RenderMode Sphere::renderMode = Sphere::RenderMode::FACE;

// regular octahedron
std::vector<glm::vec3> Sphere::vertices = {
    { 0.0f, +1.0f,  0.0f}, // 0 ~ top
    {-1.0f,  0.0f,  0.0f}, // 1
    { 0.0f,  0.0f, +1.0f}, // 2
    {+1.0f,  0.0f,  0.0f}, // 3
    { 0.0f,  0.0f, -1.0f}, // 4
    { 0.0f, -1.0f,  0.0f} //  5 ~ bottom
};

std::vector<glm::ivec3> Sphere::faces = {
    {0, 1, 2},
    {0, 2, 3},
    {0, 3, 4},
    {0, 4, 1},
    {5, 2, 1},
    {5, 3, 2},
    {5, 4, 3},
    {5, 1, 4}
};

std::vector<glm::vec2> Sphere::texture_coords = {
    {0.5f , 1.0f},
    {0.0f , 0.5f},
    {0.25f, 0.5f},
    {0.5f , 0.5f},
    {0.75f, 0.5f},
    {0.5f , 0.0f}
};

#endif // SPHERE_H_INCLUDED