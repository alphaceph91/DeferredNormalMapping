// a basic raytracer implementation
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;

uniform mat4 projection;
uniform mat4 view;
uniform vec3 camPos;
uniform vec3 u_lightPos;

//output of this shader
out vec3 upOffset;
out vec3 rightOffset;
out vec3 rayOrigin;
out vec3 rayDir;

void main() {

    // positions in world space:
    rayOrigin = camPos; // ray origin
    mat4 invView = inverse(view);
    //v_lightPos = vec3( invView * vec4(u_lightPos,1) );
    vec4 worldPos = inverse(projection*view) * vec4( a_position.xyz, 1.0 );
    rayDir = worldPos.xyz/worldPos.w - rayOrigin; // ray direction

    upOffset = normalize(invView[1].xyz)*0.005;
    rightOffset = normalize(invView[0].xyz)*0.005;


    gl_Position = vec4(a_position, 1.0);

}
