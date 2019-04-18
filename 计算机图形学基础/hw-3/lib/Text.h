#ifndef TEXT_H_INCLUDED
#define TEXT_H_INCLUDED

// FreeType
#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <map>
#include <string>

#include "./util.h"
#include "./MovableObject.h"

class Text : public MovableObject
{
    struct Character {
        GLuint texID;       // ID handle of the glyph texture
        glm::ivec2 size;    // Size of glyph
        glm::ivec2 bearing; // Offset from baseline to left/top of glyph
        GLuint advance;     // Horizontal offset to advance to next glyph
    };

private:
    static std::map<GLchar, Character> characters;
    static GLuint programID, VAO, VBO;

    static void initCharacters()
    {
        FT_Library ft;
        if (FT_Init_FreeType(&ft))
            std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;

        FT_Face face;
        if (FT_New_Face(ft, "./fonts/arial.ttf", 0, &face))
            std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;

        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, 48);

        // disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // load first 128 characters of ASCII set
        for (GLubyte c = 0; c < 128; c++)
        {
            // load character glyph
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }

            // generate texture
            GLuint texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                         GL_TEXTURE_2D,
                         0,
                         GL_RED,
                         face->glyph->bitmap.width,
                         face->glyph->bitmap.rows,
                         0,
                         GL_RED,
                         GL_UNSIGNED_BYTE,
                         face->glyph->bitmap.buffer
                         );

            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // store character for later use
            characters.insert({c, {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                GLuint(face->glyph->advance.x)
            }});
        }

        // clean up
        glBindTexture(GL_TEXTURE_2D, 0);
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
    }

public:
    static void init(const std::string& vertex_shader_path, const std::string& fragment_shader_path)
    {
        initCharacters();

        programID = util::createProgram(vertex_shader_path, fragment_shader_path);

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 5, NULL, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    static void release()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

private:
    std::string text;
    glm::vec3 color;

public:
    Text(const std::string& text, glm::vec3 color, glm::vec3 sc = glm::vec3(1.0f)): text(text), color(color)
    {
        scale(glm::vec3(0.01f)); // compensate for 48p font size
        scale(glm::vec3(sc));
    }

    void draw(glm::mat4 projection, glm::mat4 view) const
    {
        // set program and VAO
        glUseProgram(programID);
        glBindVertexArray(VAO);

        glUniform3f(glGetUniformLocation(programID, "textColor"), color.x, color.y, color.z);

        glm::mat4 mvp = projection * view * model();
        glUniformMatrix4fv(glGetUniformLocation(programID, "MVP"), 1, GL_FALSE, &mvp[0][0]);

        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        // iterate through all characters
        for (char c : text)
        {
            Character ch = characters[c];

            GLfloat xpos = x + ch.bearing.x;
            GLfloat ypos = y - (ch.size.y - ch.bearing.y);

            GLfloat w = ch.size.x;
            GLfloat h = ch.size.y;

            // update VBO for character
            GLfloat vertices[6][5] = {
                { xpos,     ypos + h,  z,  0.0, 1.0 },
                { xpos,     ypos,      z,  0.0, 0.0 },
                { xpos + w, ypos,      z,  1.0, 0.0 },

                { xpos,     ypos + h,  z,  0.0, 1.0 },
                { xpos + w, ypos,      z,  1.0, 0.0 },
                { xpos + w, ypos + h,  z,  1.0, 1.0 }
            };

            // render glyph texture over quad
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ch.texID);

            // update content of VBO memory (be sure to use glBufferSubData and not glBufferData)
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            // set polygon mode
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            // render quad
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // advance cursors for next glyph (note that advance is number of 1/64 pixels)
            x += (ch.advance >> 6); // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};

std::map<GLchar, Text::Character> Text::characters;
GLuint Text::programID;
GLuint Text::VAO;
GLuint Text::VBO;

#endif // TEXT_H_INCLUDED