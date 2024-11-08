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

uniform float bloomThreshold;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 bloomColor;

float pi = 3.1415926535;

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
    fragColor = vec4(0, 0, 0, 1);
    vec3 normal = normalize(fragNormal);
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
    
    for (int i = 0; i < num_lights; i++) {
        vec3 light = normalize(lightPositions[i] - fragPosition);
        vec3 h = normalize(view + light);
        
        float D = normalDistribution(normal, h, roughness);
        float G = geometryFunction(normal, view, roughness);
        vec3 F = fresnel(view, h);
        vec3 specularBRDF = D * G * F;
        
        vec3 lightIntensity = lightColors[i] / pow(length(lightPositions[i] - fragPosition), 2);
        
        fragColor.rgb += (diffuseBRDF + specularBRDF) * lightIntensity * dot(normal, light);
    }

    //extract bright areas for bloom
    vec3 luminance = vec3(0.2126, 0.7152, 0.0722);
	float brightness = dot(luminance, fragColor.rgb);
	if (brightness > bloomThreshold) {
		bloomColor = vec4(fragColor.rgb, 1);
	}
	else {
		bloomColor = vec4(0, 0, 0, 1);
	}
    
}
