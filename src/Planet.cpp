#include"Planet.h"
Planet::Planet(float pRadius, float pTRadius, float pThInit, float pPhInit, float pSpd, glm::vec3 pColor) {
	radius = pRadius;
	traslation_radius = pTRadius;
	theta_init = pThInit;
	phi_init = pPhInit;
	speed = pSpd;
	color = pColor;

}
glm::mat4 Planet::GetModel(glm::mat4 parent, float ts, int index, float parent_radius, glm::vec3 par_pos) {
	glm::mat4 model = glm::mat4(1.0f);
	float new_theta = glm::radians(theta_init + speed * ts);
	float new_phi = glm::radians(phi_init + speed * ts);
	//std::cout << "Theta: " << new_theta << " Phi: " << new_phi << std::endl;
	float rho = (3 + index) * parent_radius;
	float y = rho * cos(new_phi);
	float r = rho * sin(new_phi);
	float x = r * cos(new_theta);
	float z = r * sin(new_theta);
	x = rho * cos(new_theta);
	z = rho * sin(new_theta);
	y = rho * cos(new_phi);
	//std::cout << "X: " << x << " Y: " << y << " Z: " << z << std::endl;
	model = glm::translate(model, glm::vec3(x, y, z));
	model = glm::scale(model, glm::vec3(radius));
	return  glm::translate(model, par_pos);
}

void Planet::CreateMoon() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> rad_comp_distr(0.1f, 0.3f);
	float r_comp = rad_comp_distr(gen);
	std::uniform_real_distribution<> trad_comp_distr(1.5f, 2.5f);
	float rt_comp = trad_comp_distr(gen);
	std::uniform_real_distribution<> theta_distr(0.0f, 360.0f);
	float th = theta_distr(gen);
	std::uniform_real_distribution<> phi_distr(0.0f, 180.0f);
	float ph = phi_distr(gen);
	std::uniform_real_distribution<> spd_distr(0.0f, 3.5f);
	float sp = spd_distr(gen);
	std::uniform_real_distribution<> clr_distr(0.0f, 1.0f);
	glm::vec3 cl = glm::vec3(clr_distr(gen), clr_distr(gen), clr_distr(gen));
	moons.push_back(Moon{ r_comp,rt_comp,th,ph,sp,cl });
}
void Planet::RemoveMoon() {
	if (moons.size() > 0) {
		moons.pop_back();
	}
}

glm::mat4 Planet::GetMoonModel(glm::mat4 parent, float ts, int index) {
	glm::mat4 model = glm::mat4(1.0f);
	Moon m = moons[index];
	float new_theta = glm::radians(m.theta + m.speed * ts);
	float new_phi = glm::radians(m.phi + m.speed * ts);
	float new_trad = m.tradius * radius;
	float new_rad = m.radius * radius;
	float y = new_trad * cos(new_phi);
	float r = new_trad * sin(new_phi);
	float x = new_trad * cos(new_theta);
	float z = new_trad * sin(new_theta);

	model = glm::translate(model, glm::vec3(x, y, z));
	model = glm::scale(model, glm::vec3(new_rad));
	//model = glm::scale(parent,glm::vec3(2.0f * radius,1.0f,1.0f));
	return parent * model;
	// model = glm::translate(model, glm::vec3(x, y, z));
	// model = glm::scale(model, glm::vec3(radius));
	// return model * parent;
}