#include "data/physics/Particle.h"

#pragma region Public Methods

Particle::Particle() : ParticleTransform(Transform()), Velocity(glm::vec2(0, 0))
{
}

#pragma endregion