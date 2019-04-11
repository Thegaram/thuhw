#version 410 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;

out vec2 TexCoord;

uniform mat4 M;

void main()
{
    gl_Position = M * vec4(position, 1.0f);
    TexCoord = vec2(texCoord.x, 1.0f - texCoord.y);
}
