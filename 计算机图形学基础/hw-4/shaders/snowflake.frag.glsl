#version 410 core

in vec2 TexCoord;
out vec4 color;

uniform sampler2D ourTexture;

void main()
{
    color = texture(ourTexture, TexCoord);
    color.a = 0.8f;

    // cut out background
    if (vec3(color) == vec3(1.0f, 1.0f, 1.0f))
        color.a = 0.0f;
}
