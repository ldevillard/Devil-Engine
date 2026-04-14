#pragma once

#include <vector>

#include <maths/glm/glm.hpp>

#include "component/Transform.h"
#include "data/physics/Particle.h"

class FluidSolver
{
public:
	void ResetParticles(int particleCount, float particleRadius, const Transform& fluidBoxTransform, bool useRandomSpawnVelocity);
	void Update(float deltaTime, float particleRadius, const Transform& fluidBoxTransform, float bounceEnergyLoss);

	const std::vector<Particle>& GetParticles() const;

private:
	// initialization
	glm::vec3 generateSpawnVelocity(bool useRandomSpawnVelocity, float cosAngle, float sinAngle);
	
	// simulation
	void applyGravity(float deltaTime, Particle& particle) const;
	void solveBoxCollision(float particleRadius, const Transform& fluidBoxTransform, Particle& particle, float bounceEnergyLoss) const;

	std::vector<Particle> particles;
};
