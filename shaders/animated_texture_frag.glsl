#version 410

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragTexCoord;

layout(std140) uniform Material // Must match the GPUMaterial defined in src/mesh.h
{
	vec3 kd;
	vec3 ks;
	float shininess;
	float transparency;
};

uniform sampler2D tex;
uniform float time;
uniform int columns;
uniform int rows;
uniform float animationSpeed;

layout(location = 0) out vec4 fragColor;

void main() {
	int currentFrame = int(time * animationSpeed) % (rows * columns);
	int row = currentFrame / columns;
	int column = currentFrame % columns;
	vec2 texPos = vec2(1.0 * column, 1.0 * row);
	//texPos is the top left corner of the current frame, texCoords is the offset from that corner
	vec2 animatedTexCoord = (texPos + fragTexCoord);
	animatedTexCoord.x = animatedTexCoord.x / columns;
	animatedTexCoord.y = animatedTexCoord.y / rows;
	fragColor = vec4(texture(tex, animatedTexCoord).rgb, 1);
}