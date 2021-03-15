// fragment shader
#version 330 core
out vec4 FragColor;
in  vec2 TexCoords;
  
uniform sampler2D screenTexture;
uniform float randomNumber;	// CPP: randomNumber = ((double)rand() / (RAND_MAX));
uniform float timer;


float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898 - randomNumber, 78.233))) * 43758.5453);
}

  

void main() {

	// read the size of the texture
	vec2 screenSize = textureSize(screenTexture, 0);

	vec4 screenColor = texture(screenTexture, TexCoords);
	float average = 0.2126 * screenColor.r +
		0.7152 * screenColor.g +
		0.0722 * screenColor.b;

	float distanceFromCenter = length(TexCoords - vec2(.5f, .5f));
	float vignetteAmount = 1.0 - distanceFromCenter;
	vignetteAmount = smoothstep(0.55, 1.0, vignetteAmount);

	vec3 result = vec3(0.0, average, 0.0);        // vision color
	result += 0.1 * sin(TexCoords.y * screenSize.y * 2.0); // scanline
	result += 0.05 * sin(timer*100.0f);  		      // screen flicker
	result *= vignetteAmount; // vignette effect
	FragColor = vec4(result, 1.0);

}
