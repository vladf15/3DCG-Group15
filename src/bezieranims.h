#include <glm/vec3.hpp>
#include <vector>

struct BezierSpline {
	std::vector<glm::vec3> control_points;
	std::vector<glm::vec3> handles_left;
	std::vector<glm::vec3> handles_right;
};

std::vector<BezierSpline> loadSplines(const char* file_path);

glm::vec3 interpPoint(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t);

float getSegmentLength(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, int subdivisions);

float getBezierLength(BezierSpline s, int subdivisions);

std::pair<glm::vec3, glm::vec3> getPointOnCurve(BezierSpline s, float t, int subdivisions);

glm::mat4 getRotationMatrix(glm::vec3 direction);

