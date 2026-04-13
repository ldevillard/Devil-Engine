#pragma once

#include <maths/glm/glm.hpp>

#include "component/Transform.h"

class Particle
{
public:
	Particle();

	Transform ParticleTransform;
	glm::vec2 Velocity;
};