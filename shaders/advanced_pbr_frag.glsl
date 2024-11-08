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
uniform bool useNormalMap;
uniform bool useRoughnessMap;

uniform float metallic;
uniform float roughness;
uniform vec3 albedo;
uniform bool overrideBase;
uniform vec3 baseColor;

uniform sampler2D normalMap;
uniform sampler2D roughnessMap;
uniform sampler2D kaMap;

uniform vec3 cameraPos;
uniform int num_lights;
uniform vec3 lightPositions[32];
uniform vec3 lightColors[32];

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 T; 
in vec3 B;
in vec3 N;

layout(location = 0) out vec4 fragColor;

float pi = 3.1415926535;

// Calculations mostly based on https://www.youtube.com/watch?v=XK_p2MxGBQs 
// Everything except for the interpretation of the metallic parameter is from there.
float normalDistribution(vec3 normal, vec3 h, float a) {
    // according to Disney and Epic Games, we take a^4
    return (a * a * a * a) / (pi * pow(pow(dot(normal, h), 2) * (a * a * a * a - 1) + 1, 2));
}

float geometryFunction(vec3 normal, vec3 view, float a) {
    float k = (a + 1) * (a + 1) / 8.0;
    float dp = dot(normal, view);
    float div = dp / (dp * (1 - k) + k);
    return div * div;
}

vec3 fresnel(vec3 view, vec3 h) {
    vec3 F0 = mix(vec3(0.04), overrideBase ? albedo : ks, metallic);
    return F0 + (vec3(1) - F0) * pow(1 - dot(view, h), 5);
}

void main()
{
    mat3 TBN = mat3(T, B, N);
    fragColor = vec4(0, 0, 0, 1);
    vec3 normal = N;
    if (useNormalMap) {
        normal = normalize(TBN * (texture(normalMap, fragTexCoord).rgb * 2.0 - vec3(1.0)));
    }
    vec3 view = normalize(cameraPos - fragPosition);
    
    if (!useMaterial) {
        fragColor = vec4(normal, 1);
        return;
    }
    
    vec3 fLambert = kd;
    if (hasTexCoords) { 
        fLambert = texture(colorMap, fragTexCoord).rgb;
    } else if (overrideBase) {
        fLambert = baseColor;
    }
    
    vec3 diffuseBRDF = (1 - metallic) * fLambert / pi;
    float realRoughness = roughness;
    if (useRoughnessMap) {
        realRoughness = texture(roughnessMap, fragTexCoord).x;
    }

    for (int i = 0; i < num_lights; i++) {
        vec3 light = normalize(lightPositions[i] - fragPosition);
        vec3 h = normalize(view + light);
        
        float D = normalDistribution(normal, h, realRoughness);
        float G = geometryFunction(normal, view, realRoughness);
        vec3 F = fresnel(view, h);
        vec3 specularBRDF = D * G * F / (4 * dot(normal, light) * dot(normal, view));
        
        vec3 lightIntensity = lightColors[i] / pow(length(lightPositions[i] - fragPosition), 2);
        
        fragColor.rgb += (diffuseBRDF + specularBRDF) * lightIntensity * dot(normal, light);
    }

    // 0.03 just looked good haha
    float ambientOcclusion = texture(kaMap, fragTexCoord).r;
    vec3 ambient = vec3(0.03) * albedo * ambientOcclusion;

    fragColor.rgb += ambient;
}
