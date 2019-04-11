#version 330 core

uniform bool override;
in vec3 fragmentColor;
out vec3 color;

void main() {
    color = override ? vec3(0, 1, 0) : fragmentColor;
}