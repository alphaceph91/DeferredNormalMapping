#version 330 core
layout (location = 0) out vec4 FragColor;

uniform vec3 lightColor;
uniform float alpha;

void main()
{           
    FragColor = vec4(lightColor, alpha);
}