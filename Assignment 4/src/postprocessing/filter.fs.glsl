// fragment shader
#version 330 core
out vec4 FragColor;
in  vec2 TexCoords;
  
uniform sampler2D screenTexture;
uniform float kernelSize;
  

void main() {

    // read the size of the texture
    vec2 screenSize = textureSize(screenTexture,0);
    vec2 pxOffset = kernelSize * vec2(1.0) / screenSize;

    vec2 offsets[9] = vec2[](
        vec2(-1.0, 1.0), // top-left
        vec2(0.0f, 1.0), // top-center
        vec2(1.0, 1.0), // top-right
        vec2(-1.0, 0.0f),   // center-left
        vec2(0.0f, 0.0f),   // center-center
        vec2(1.0, 0.0f),   // center-right
        vec2(-1.0, -1.0), // bottom-left
        vec2(0.0f, -1.0), // bottom-center
        vec2(1.0, -1.0)  // bottom-right    
        );

    /*
    float kernel[9] = float[](
        -1, -1, -1,
        -1, 9, -1,
        -1, -1, -1
        );
    */
    float kernel[9] = float[](
        1.0 / 16, 2.0 / 16, 1.0 / 16,
        2.0 / 16, 4.0 / 16, 2.0 / 16,
        1.0 / 16, 2.0 / 16, 1.0 / 16
        );

    vec3 sampleTex[9];
    for (int i = 0; i < 9; i++)
        sampleTex[i] = vec3(texture(screenTexture, TexCoords.st + offsets[i] * pxOffset));

    vec3 col = vec3(0.0);
    for (int i = 0; i < 9; i++)
        col += sampleTex[i] * kernel[i];

    FragColor = vec4(col, 1.0);
}
