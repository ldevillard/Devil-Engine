#pragma once

#include <vector>

#include <maths/glm/glm.hpp>

#include "component/Transform.h"
#include "data/physics/Particle.h"

class FluidSolver
{
public:
	void ResetParticles(int particleCount, float particleRadius, const Transform& fluidBoxTransform);
	void Update(float deltaTime, float particleRadius, const Transform& fluidBoxTransform);

	const std::vector<Particle>& GetParticles() const;

private:
	void applyGravity(float deltaTime, Particle& particle) const;
	void solveBoxCollision(float particleRadius, const Transform& fluidBoxTransform, Particle& particle) const;

	std::vector<Particle> particles;
};
