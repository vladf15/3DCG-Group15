#version 410

uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
// Normals should be transformed differently than positions:
// https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
uniform mat3 normalModelMatrix;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

out vec3 curPos;
out vec3 Normal;

void main()
{
    gl_Position = mvpMatrix * vec4(position, 1);
    
    curPos    = (modelMatrix * vec4(position, 1)).xyz;
    Normal      = normalModelMatrix * normal;

}