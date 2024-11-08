#version 410

in vec2 fragTexCoord;

uniform sampler2D depthTexture;
uniform sampler2D sceneTexture;
uniform int width;
uniform int height;
uniform float focalLength;
uniform float focalRange;
uniform bool horizontal;

layout(location = 0) out vec4 fragColor;

void main()
{
	float weight = 0.0;
	vec3 result = vec3(0.0);
	float weightSum = 0.0;

	if (focalRange == 0.0)
	{
		fragColor = texture(sceneTexture, fragTexCoord);
		return;
	}
	float depth = texture(depthTexture, fragTexCoord).r;
	int dofRadius = int(abs(depth - focalLength) / focalRange);
	if (dofRadius > 10)
		dofRadius = 10;	

	for (int i = -dofRadius; i <= dofRadius; i++)
	{
		//sum of gaussian weights in one direction
		if (horizontal && texture(sceneTexture, fragTexCoord + vec2(float(i) / width, 0.0)).r > depth + focalRange) {
			weight = 0.0;
		}
		else if (!horizontal && texture(sceneTexture, fragTexCoord + vec2(0.0, float(i) / height)).r > depth + focalRange) {
			weight = 0.0;
		}
		else {
			weight = exp(-(float(i * i)) / (dofRadius * dofRadius * 2.0));
			}
		weightSum += weight;
	}
	if (weightSum == 0.0)
		weightSum = 1.0;

	for (int i = -dofRadius; i <= dofRadius; i++)
	{
		//sum of gaussian weights in one direction
		weight = exp(-(i * i) / (dofRadius * dofRadius * 2.0));
		if (horizontal) {
			if (texture(sceneTexture, fragTexCoord + vec2(float(i) / width, 0.0)).r < depth) {
				weight = 0.0;
			}
			result += weight * texture(sceneTexture, fragTexCoord + vec2(float(i) / width, 0.0)).rgb / weightSum;
		}
		else {
			if (texture(sceneTexture, fragTexCoord + vec2(0.0, float(i) / height)).r < depth) {
				weight = 0.0;
			}
			result += weight * texture(sceneTexture, fragTexCoord + vec2(0.0, float(i) / height)).rgb / weightSum;
		}
	}
	fragColor = vec4(result, 1.0);
}