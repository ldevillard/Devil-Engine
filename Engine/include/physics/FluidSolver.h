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
	
	void SetSmoothingRadius(float value);
	void SetTargetDensity(float value);
	void SetPressureMultiplier(float value);

	const std::vector<Particle>& GetParticles() const;

private:
	// initialization
	glm::vec3 generateSpawnVelocity(bool useRandomSpawnVelocity, float cosAngle, float sinAngle);
	glm::vec2 getRandomDir();
	
	// simulation
	void applyGravity(float deltaTime, Particle& particle) const;
	void solveBoxCollision(float particleRadius, const Transform& fluidBoxTransform, Particle& particle, float bounceEnergyLoss) const;
	void updateDensities();
	glm::vec2 calulatePressureForce(Particle& particle);
	float calculateSharedPressure(float densityA, float densityB);
	void calculateDensity(Particle& particle);
	float convertDensityToPressure(float density) const;
	float smoothingKernel(float radius, float distance) const;
	float smoothingKernelDerivative(float radius, float distance) const;
	
	std::vector<Particle> particles;

	// TODO: variable directly in the inspector
	float smoothingRadius = 0.75f;
	float targetDensity = 4.0f;
	float pressureMultiplier = 500.0f;
};
