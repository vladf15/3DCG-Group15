#version 410

uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
// Normals should be transformed differently than positions:
// https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
uniform mat3 normalModelMatrix;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform float ts;

out vec3 curPos;
out vec3 Normal;

void main()
{
    vec3 new_pos = vec3(position.x,position.y,position.z);
    vec2 wave_center = vec2(0.0f,0.0f);
    float wave_speed = 1.0f;
    float radius = wave_speed * ts;
    float lambda = radius / 2;
    float phi = 10;

    if(ts >= 0){
        float dist_to_cnt = length(vec2(new_pos.x,new_pos.z) - wave_center);
        if(dist_to_cnt <= radius && dist_to_cnt >= (radius - lambda)){
            new_pos.y = sin(phi * (radius - dist_to_cnt));
        }


    }


    gl_Position = mvpMatrix * vec4(new_pos, 1);
    curPos    = (modelMatrix * vec4(new_pos, 1)).xyz;
    //Normal      = normalModelMatrix * normal;
    Normal = normal;
}