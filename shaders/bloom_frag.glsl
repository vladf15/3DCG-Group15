#version 410

in vec2 fragTexCoord;

uniform sampler2D bloomTexture;
uniform sampler2D sceneTexture;
uniform int width;
uniform int height;
uniform int bloomRadius;
uniform float bloomIntensity;
uniform bool horizontal;
uniform bool finalPass;

layout(location = 0) out vec4 fragColor;

void main()
{
	float weight = 0.0;
	vec3 result = vec3(0.0);

	for (float i = -bloomRadius; i <= bloomRadius; i++)
	{
		//sum of gaussian weights in one direction
		weight = exp(-(i * i) / (bloomRadius * bloomRadius * 2.0));
		if (horizontal) {
			result += weight * texture(bloomTexture, fragTexCoord + vec2(i / width, 0.0)).rgb;
		}
		else {
			result += weight * texture(bloomTexture, fragTexCoord + vec2(0.0, i / height)).rgb;
		}
	}
	if (finalPass) {
		result += texture(sceneTexture, fragTexCoord).rgb;
	}
	fragColor = bloomIntensity * vec4(result, 1.0);
}