#pragma once

#include <maths/glm/glm.hpp>

class Particle
{
public:
	Particle();
	Particle(const glm::vec3& position, const glm::vec3& velocity);

	glm::vec3 Position;
	glm::vec3 Velocity;
	
	float Mass = 1.0f;
	float Density = 0.0f;
};
