#version 410 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texCoord;

out vec2 TexCoord;

uniform float width;
uniform float height;

void main()
{
    gl_Position = vec4(position.x * width, position.y * height, 0.0f, 1.0f);
    TexCoord = vec2(texCoord.x, 1.0f - texCoord.y);
}
