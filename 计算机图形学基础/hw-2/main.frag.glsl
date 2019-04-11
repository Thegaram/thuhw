#version 410 core

in vec2 TexCoord;

out vec4 color;

uniform sampler2D ourTexture;
uniform vec4 overrideColor;

float threshold = 0.2f;

void main()
{
    color = texture(ourTexture, TexCoord) * overrideColor;

    float brightness = length(vec3(color));

    // fade out towards edges
    if (brightness < threshold)
        color.a = (brightness * (1.0f / threshold)) * overrideColor.a;
}
