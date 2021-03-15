#version 330 core
out vec4 fragColor;

in vec3 normal;
in vec2 texCoord;
in vec3 fragPos;

uniform vec3 cameraPos;
uniform samplerCube cubeTex;
uniform int mode = 0;
void main()
{    	
	vec3 nnormal = normalize(normal);
    vec3 I = normalize(fragPos - cameraPos);
	vec3 reflection = reflect(I, nnormal);
	vec4 reflectionColor = vec4(texture(cubeTex, reflection).rgb, 1.0);

	float ratio = 1.00 / 1.52;
	vec3 refraction = refract(I, nnormal, ratio);
	vec4 refractionColor = vec4(texture(cubeTex, refraction).rgb, 1.0);


	if (mode == 0) // reflection
		fragColor = reflectionColor;
	else if (mode == 1) // refraction 
		fragColor = refractionColor;
 }