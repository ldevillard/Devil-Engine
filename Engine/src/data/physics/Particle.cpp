#include "data/physics/Particle.h"

#pragma region Public Methods

Particle::Particle()
	: Position(glm::vec3(0.0f))
	, Velocity(glm::vec3(0.0f))
{
}

Particle::Particle(const glm::vec3& position, const glm::vec3& velocity)
	: Position(position)
	, Velocity(velocity)
{
}

#pragma endregion
