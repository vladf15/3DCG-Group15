#version 410

uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
// Normals should be transformed differently than positions:
// https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
uniform mat3 normalModelMatrix;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform float ts;
uniform vec2 wave_center;
uniform float wave_speed;
uniform float lambda ;
uniform float phi ;
uniform float dampening ;

out vec3 curPos;
out vec3 Normal;

void main()
{
    vec3 new_pos = vec3(position.x,position.y,position.z);
    Normal      = normalModelMatrix * normal;
    float radius = wave_speed * ts;
    float epsilon = 0.0002f;
    if(ts >= 0){
        float dist_to_cnt = length(vec2(new_pos.x,new_pos.z) - wave_center);
        if(dist_to_cnt <= radius && dist_to_cnt >= radius -  radius * lambda){
            new_pos.y = sin(phi * (radius - dist_to_cnt)) / (dampening * ts);
            float next_y = sin(phi * (radius - dist_to_cnt) + epsilon);
            float prev_y = sin(phi * (radius - dist_to_cnt) + epsilon);
            if(next_y > prev_y){
                Normal = new_pos - vec3(wave_center.x, 0.0f, wave_center.y);
            }else{
                Normal = -(new_pos - vec3(wave_center.x, 0.0f, wave_center.y));
            }
        }
    }


    gl_Position = mvpMatrix * vec4(new_pos, 1);
    curPos    = (modelMatrix * vec4(new_pos, 1)).xyz;

}