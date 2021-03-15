#version 330 core
layout (location = 0) out vec4 gFragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

void main()
{    
    // the diffuse color
    gFragColor.rgba = vec4(texture(texture_diffuse1, TexCoords).rgb, 1.0);
}