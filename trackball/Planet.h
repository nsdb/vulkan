#pragma once

#include "cgmath.h"			// slee's simple math library
#include "cgut.h"			// slee's OpenGL utility

class Planet {

public:

	uint parent_index; // parent planet index (-1 : no parent)
	uint planet_texture_index; // planet texture index (used in render())

	float distance; // distance from Sun
	float radius;   // size proportional to Earth
	float rotation_cycle; // rotation cycle of planet
	float revolution_cycle; // revolution cycle of planet

	float rotation_theta;       // current planet rotation angle
	float revolution_theta;    // current planet revolustion angle


	Planet(uint parent_index, uint planet_texture_index, float distance, float radius, float rotation_cycle, float revolution_cycle) {

		this->parent_index = parent_index;
		this->planet_texture_index = planet_texture_index;
		this->distance = distance;
		this->radius = radius;
		this->rotation_cycle = rotation_cycle;
		this->revolution_cycle = revolution_cycle;

		// initial angle
		rotation_theta = 0.0f;
		revolution_theta = 0.0f;
	}

	// process rotation and revolution, so theta is changed
	void time_process(float elapsed_time) {

		// prevent divided by 0
		if (rotation_cycle > 0) {
			rotation_theta += elapsed_time / rotation_cycle * 2 * PI;
		}
		if (revolution_cycle > 0) { 
			revolution_theta += elapsed_time / revolution_cycle * 2 * PI;
		}

	}

};