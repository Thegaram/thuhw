#version 410 core

layout (location = 0) in vec2 vertexCoord;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec2 position;
layout (location = 3) in float scale;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(vertexCoord * scale + position, 0.0f, 1.0f);
    TexCoord = vec2(texCoord.x, 1.0f - texCoord.y);
}
