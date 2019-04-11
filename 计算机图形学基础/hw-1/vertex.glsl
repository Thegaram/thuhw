#version 330 core

uniform mat4 MVP;

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexColor;

out vec3 fragmentColor;

void main() {
    gl_Position = MVP * vec4(vertexPosition, 1);
    fragmentColor = vertexColor;
}
