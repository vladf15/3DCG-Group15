#version 410

layout(std140) uniform Material // Must match the GPUMaterial defined in src/mesh.h
{
    vec3 kd;
    vec3 ks;
    float shininess;
    float transparency;
};

uniform sampler2D colorMap;
uniform bool hasTexCoords;
uniform bool useMaterial;
uniform vec3 cameraPos;

uniform float metallic;
uniform float roughness;
uniform float ambientOcclusion;
uniform vec3 albedo;

uniform vec3[1] lightPos;
uniform vec3[1] lightColor;
int n_lights = 1;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;

layout(location = 0) out vec4 fragColor;
float pi = 3.14159265359;

//F
vec3 fresnel(vec3 h, vec3 l, vec3 F0) {
    return F0 + (1.0f - F0) * pow(clamp(1.0 - dot(h, l), 0.0f, 1.0f), 5.0f);
}

//G
float schlickGGX(vec3 normalV, vec3 viewV, float roughness) {
    float dot_nv = max(dot(normalV, viewV), 0.0f);
    float k = ((roughness + 1) * (roughness + 1)) / 8.0f;
    return dot_nv / (dot_nv * (1.0f - k) + k);
}

//D
float NDF(vec3 normalV, vec3 H, float a) {
    float dot_nh = max(dot(normalV, H), 0.0);
    float lower = (dot_nh * dot_nh * (a*a - 1.0) + 1.0);

    return (a*a) / (pi * lower * lower);
}

void main()
{
    vec3 normalV = normalize(fragNormal);
    vec3 viewV = normalize(cameraPos - fragPosition);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lout = vec3(0.0);
    for(int i = 0; i < n_lights; i++) {
        vec3 lightV = normalize(lightPos[i] - fragPosition);
        vec3 H = normalize(lightV + viewV);
        float dist = length(lightPos[i] - fragPosition);
        float attenuation = 1.0f/(dist * dist);
        vec3 radiance = lightColor[i] * attenuation;

        // cook torrance formula
        vec3 F = fresnel(H, viewV, F0);

        float gv = schlickGGX(normalV, viewV, roughness);
        float gl = schlickGGX(normalV, lightV, roughness);
        float G = gv * gl;

        float D = NDF(normalV, H, roughness*roughness);

        // result of cook torrance formula
        vec3 fr = (F*G*D) / (4.0f * max(dot(normalV, viewV), 0.0f) * max(dot(normalV, lightV), 0.0f) + 0.00001f);

        vec3 ks = F;
        vec3 kd = (vec3(1.0f) - ks) * (1.0 - metallic);
        Lout += ((kd * albedo) / (pi + fr)) * max(dot(normalV, lightV), 0.0f) * radiance;
    }
    vec3 ambient = vec3(0.03) * albedo * ambientOcclusion;
    vec3 result = Lout + ambient;

    result = result / (result + vec3(1.0f));
    result = pow(result, vec3(1.0f/2.2f));
    fragColor = vec4(result, 1.0);
}
