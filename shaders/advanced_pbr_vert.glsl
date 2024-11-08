#version 410

uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
// Normals should be transformed differently than positions:
// https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
uniform mat3 normalModelMatrix;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;
layout(location = 3) in vec3 e1;
layout(location = 4) in vec3 e2;
layout(location = 5) in vec2 duv1;
layout(location = 6) in vec2 duv2;

out vec3 fragPosition;
out vec2 fragTexCoord;
out vec3 T;
out vec3 B;
out vec3 N;

void main()
{
    mat2x3 eMat = mat2x3(e1, e2);
    mat2 uvMat = mat2(vec2(duv2.y, -duv2.x), vec2(-duv1.y, duv1.x));
    mat2x3 tbMat = transpose(uvMat * transpose(eMat));

    gl_Position = mvpMatrix * vec4(pos, 1.0);
    fragPosition = vec3(modelMatrix * vec4(pos, 1.0));
    fragTexCoord = texCoords;

    // tangent space 
    T = normalize(mat3(normalModelMatrix) * tbMat[0]);
    N = normalize(mat3(normalModelMatrix) * normal);

    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
}