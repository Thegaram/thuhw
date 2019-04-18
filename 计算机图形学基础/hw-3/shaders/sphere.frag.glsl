#version 410 core

in vec2 TexCoord;

out vec4 color;

uniform sampler2D ourTexture;
uniform bool override;

void main()
{
    color = override ? vec4(0.0f, 1.0f, 0.0f, 1.0f) : texture(ourTexture, TexCoord);
}
