// fragment shader
#version 330 core
out vec4 FragColor;
in  vec2 TexCoords;
  
uniform sampler2D screenTexture;
uniform float randomNumber;	// CPP: randomNumber = ((double)rand() / (RAND_MAX));

float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898 - randomNumber, 78.233))) * 43758.5453);
}

  
void main()
{
    // sepia RGB value
    vec3 sepia = vec3(112.0 / 255.0, 66.0 / 255.0, 20.0 / 255.0);

    FragColor = texture(screenTexture, TexCoords);
    float average = 0.2126 * FragColor.r +
                    0.7152 * FragColor.g +
                    0.0722 * FragColor.b;
    
    vec3 result = mix(sepia, vec3(average, average, average), 0.4);
    result *= rand(TexCoords);
    FragColor = vec4(result, 1.0);

} 