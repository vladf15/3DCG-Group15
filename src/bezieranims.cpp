#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
DISABLE_WARNINGS_POP()
#include<iostream>
#include<fstream>
#include "bezieranims.h"


using json = nlohmann::json;
using namespace std;

//used similar loader code as last year since it seems like the simplest way to load the bezier splines
//I rewrote the actual functionality from scratch
//load the bezier splines from blender exported json file
vector<BezierSpline> loadSplines(const char* file_path) {
    json j;
    ifstream file(file_path);
    file >> j;
    vector<BezierSpline> splines;
    for (const auto& splineText : j["splines"]) {
        BezierSpline spline;
        //control points for each spline
        for (const auto& control : splineText["control_points"]) {
            spline.control_points.emplace_back(glm::vec3(control[0], control[2], -1.0 * control[1]));
        }
        for (const auto& left : splineText["handles_left"]) {
            spline.handles_left.emplace_back(glm::vec3(left[0], left[2], -1.0 * left[1]));
        }
        for (const auto& right : splineText["handles_right"]) {
            spline.handles_right.emplace_back(glm::vec3(right[0], right[2], -1.0 * right[1]));
        }
        splines.emplace_back(spline);
    }

    return splines;
}

//get the point on the bezier segment at a given t between [0,1]
glm::vec3 interpPoint(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t) {
    //points between control and handle
    glm::vec3 p_left = p0;
    glm::vec3 p_right = p2;
    //handle interpolation
    glm::vec3 handle_interp = p1;
    //points between previous interpolations and handle interp
    glm::vec3 p_left_interp = p_left;
    glm::vec3 p_right_interp = p_right;
    //final point on the curve
    glm::vec3 p_curve = p_left_interp;

    //REFERENCE USED
    //https://miro.medium.com/v2/resize:fit:640/format:webp/1*RdNctOG0RlAfzvrAez2rVQ.gif
    p_left = p0 * (1 - t) + p1 * t;
    p_right = p2 * (1 - t) + p3 * t;
    handle_interp = p1 * (1 - t) + p2 * t;

    p_left_interp = p_left * (1 - t) + handle_interp * t;
    p_right_interp = handle_interp * (1 - t) + p_right * t;

    p_curve = p_left_interp * (1 - t) + p_right_interp * t;

    return p_curve;
 }

//get length of a segment on the bezier curve (p0 and p1 are the left control point and handle, p2 and p3 are the right handle and control point)
float getSegmentLength(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, int subdivisions = 100) {
    float length = 0.0f;
   
    //interp point on the curve
	glm::vec3 p_curve = p0;
	//previous point on the curve
    glm::vec3 p_last = p0;
    for (float i = 1; i <= subdivisions; i++) {
        float t = i / subdivisions;
		p_curve = interpPoint(p0, p1, p2, p3, t);
		length += glm::length(p_curve - p_last);
        p_last = p_curve;
    }
    return length;
}

//get the total length of the combined bezier curve
float getBezierLength(BezierSpline s, int subdivisions = 100) {
	float length = 0.0f;
	for (int i = 0; i < s.control_points.size() - 1; i++) {
		length += getSegmentLength(s.control_points[i], s.handles_right[i], s.handles_left[i + 1], s.control_points[i + 1], subdivisions);
	}
	return length;
}

//get the direction of animated object at the given t
glm::vec3 interpTangent(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t) {
    //reference used:
    //https://stackoverflow.com/questions/4089443/find-the-tangent-of-a-point-on-a-cubic-bezier-curve

    glm::vec3 tangent = glm::normalize(3 * (1 - t) * (1 - t) * (p1 - p0) + 6 * (1 - t) * t * (p2 - p1) + 3 * t * t * (p3 - p2));
    return tangent;
}


//get the point on the bezier curve at a given t between [0,1] as 
std::pair<glm::vec3, glm::vec3> getPointOnCurve(BezierSpline s, float t, int subdivisions = 100) {
	int segments = s.control_points.size() - 1;
	float goal_length = getBezierLength(s, subdivisions) * t;
	float current_length = 0.0f;

    for (int i = 0; i < segments; i++) {
		float segment_length = getSegmentLength(s.control_points[i], s.handles_right[i], s.handles_left[i + 1], s.control_points[i + 1], subdivisions);
		//if the goal point is on the current segment, find the point and direction
        if (current_length + segment_length >= goal_length) {
            float t_segment = (goal_length - current_length) / segment_length;
			glm::vec3 pos = interpPoint(s.control_points[i], s.handles_right[i], s.handles_left[i + 1], s.control_points[i + 1], t_segment);
			glm::vec3 direction = interpTangent(s.control_points[i], s.handles_right[i], s.handles_left[i + 1], s.control_points[i + 1], t_segment);
			return { pos, direction };
        }
		current_length += segment_length;
    }
	return { s.control_points[segments], interpTangent(s.control_points[segments - 1], s.handles_right[segments - 1], s.handles_left[segments], s.control_points[segments], 1.0f) };
}

//convert direction to rotation matrix to update mesh orientation
glm::mat4 getRotationMatrix(glm::vec3 direction) {
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 right = glm::normalize(glm::cross(direction, up));
	up = glm::normalize(glm::cross(right, direction));
	return glm::mat4(glm::vec4(right, 0.0f), glm::vec4(up, 0.0f), glm::vec4(direction, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
}