#version 410


in vec3 curPos;
in vec3 Normal;
uniform vec4 theColor;
layout(location = 0) out vec4 fragColor;

vec4 direcLight()
{
	vec4 finColor = theColor;
	// ambient lighting
	float ambient = 0.20f;

	// // diffuse lighting
	vec3 normal = normalize(Normal);
	vec3 lightDirection = normalize(vec3(3.0f, 1.0f, -10.0f));
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	// // specular lighting
	float specularLight = 0.50f;
	vec3 viewDirection = normalize(vec3(5.0f,5.0f,5.0f) - curPos);
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 8);
	float specular = specAmount * specularLight;

	
	finColor = (vec4(theColor) * (diffuse + specular + ambient)) * vec4(1.0f);
	return finColor;
}

void main()
{
    fragColor = direcLight();
}
