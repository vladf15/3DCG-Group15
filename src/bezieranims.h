#include <glm/vec3.hpp>
#include <vector>

struct BezierSpline {
	std::vector<glm::vec3> control_points;
	std::vector<glm::vec3> handles_left;
	std::vector<glm::vec3> handles_right;
};