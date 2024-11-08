#ifndef PLANET_CLASS_H
#define PLANET_CLASS_H

#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<vector>
#include<iostream>
#include<random>

struct Moon {
	float radius;
	float tradius;
	float theta;
	float phi;
	float speed;
	glm::vec3 color;
};
class Planet {
public:
	float radius;
	float traslation_radius;
	float theta_init;
	float phi_init;
	float speed;
	glm::vec3 color;
	std::vector <Moon> moons = {};
	glm::vec3 position;
	Planet(float pRadius, float pTRadius, float pThInit, float pPhInit, float pSpd, glm::vec3 pColor);
	glm::mat4 GetModel(glm::mat4 parent, float ts, int index, float parent_radius, glm::vec3 par_pos);
	void CreateMoon();
	void RemoveMoon();
	glm::mat4 GetMoonModel(glm::mat4 parent, float ts, int index);
};
#endif