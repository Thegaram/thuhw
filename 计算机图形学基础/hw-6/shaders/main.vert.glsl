#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out vec3 Normal;
out vec3 FragPos;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

void main()
{
    gl_Position = P * V *  M * vec4(position, 1.0f);
    FragPos = vec3(M * vec4(position, 1.0f));
    Normal = mat3(transpose(inverse(M))) * normal;
}