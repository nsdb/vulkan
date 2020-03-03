#pragma once

#include "cgmath.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define MODE_ROTATION   0
#define MODE_ZOOM       1
#define MODE_PANNING    2


struct Trackball
{
	bool bTracking;
	float scale;              // controls how much rotation is applied
	glm::mat4	view_matrix0; // initial view matrix, current view matrix
	glm::vec2	m0;           // the last mouse position
	int mode;                 // mode of trackball (rotation, zoom, panning)

	Trackball(float rot_scale = 1.0f) : bTracking(false), scale(rot_scale) {}

	void end() { bTracking = false; }

	void begin(const glm::mat4& view_matrix, float x, float y, int mode)
	{
		bTracking = true;                   // enable trackball tracking
		m0 = glm::vec2(x, y) * 2.0f - 1.0f; // convert (x,y) in [0,1] to [-1,1]
		view_matrix0 = view_matrix;         // save current view matrix
		this->mode = mode;
	}
	
	glm::mat4 update(float x, float y)
	{
		// project a 2D mouse position to a unit sphere
		static const glm::vec3 p0 = glm::vec3(0, 0, 1.0f);                           // original position of the camera
		glm::vec3 p1 = glm::vec3(x * 2.0f - 1.0f - m0.x, m0.y - y * 2.0f + 1.0f, 0); // calculate displacement with vertical y-swapping
		if(!bTracking||length(p1) < 0.0001f) return view_matrix0;          // ignore subtle movement
		p1 *= scale;                                                       // apply scaling

		switch (mode) {
		
		case MODE_ROTATION: {

			p1 = glm::vec3(p1.x, p1.y, sqrtf(max(0, 1.0f - glm::length(p1)))); // adjust z to make unit sphere
			p1 = glm::normalize(p1);

			// find rotation axis and angle (with inverse view rotation to the world coordinate)
			glm::vec3 n = glm::cross(p0, p1) * ((glm::mat3)view_matrix0);
			float angle = asin(min(glm::length(n), 0.999f));

			// return resulting rotation matrix
			return glm::rotate(view_matrix0, angle, n);

		} break;

		case MODE_ZOOM: {

			// zoom
			float scaleFactor = 1 + p1.y;
			if (scaleFactor <= 0.1f) scaleFactor = 0.1f;   // prevent zooming out too far
			return glm::scale(view_matrix0, glm::vec3(scaleFactor, scaleFactor, scaleFactor));

		} break;

		case MODE_PANNING: {

			// panning
			p1 = glm::vec3(p1.y, -p1.x, sqrtf(max(0, 1.0f - glm::length(p1)))); // adjust z to make unit sphere
			p1 = glm::normalize(p1) * 10.0f;
			glm::vec3 n = glm::cross(p0, p1) * ((glm::mat3)view_matrix0) * 3.0f;

			return glm::translate(view_matrix0, glm::vec3(n.x, n.y, n.z));

		} break;

		default: return view_matrix0;

		}

		


	}
};
