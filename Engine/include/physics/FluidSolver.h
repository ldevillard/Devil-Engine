#pragma once

#include <vector>

#include <maths/glm/glm.hpp>

#include "component/Transform.h"
#include "data/physics/Particle.h"

struct FluidSimulationSettings
{
	int ParticleCount = 1200;
	float ParticleRadius = 0.25f;
	float SmoothingRadius = 0.75f;
	float TargetDensity = 4.0f;
	float PressureMultiplier = 500.0f;
	float GravityMultiplier = 1.0f;
	float BounceEnergyLoss = 0.75f;
	bool UseRandomSpawnVelocity = false;
};

class FluidSolver
{
public:
	void ResetParticles(const FluidSimulationSettings& settings, const Transform& fluidBoxTransform);
	void Update(float deltaTime, const FluidSimulationSettings& settings, const Transform& fluidBoxTransform);

	const std::vector<Particle>& GetParticles() const;

private:
	// initialization
	glm::vec3 generateSpawnVelocity(bool useRandomSpawnVelocity, float cosAngle, float sinAngle);
	glm::vec2 getRandomDir();
	
	// simulation
	void applyGravity(float deltaTime, Particle& particle) const;
	glm::vec2 calculateBoundaryForce(float particleRadius, const Transform& fluidBoxTransform, const Particle& particle) const;
	void solveBoxCollision(float particleRadius, const Transform& fluidBoxTransform, Particle& particle, float bounceEnergyLoss) const;
	void updateDensities();
	glm::vec2 calulatePressureForce(Particle& particle);
	float calculateSharedPressure(float densityA, float densityB);
	void calculateDensity(Particle& particle);
	float convertDensityToPressure(float density) const;
	float densityKernelPoly6(float radius, float distance) const;
	float pressureKernelSpikyDerivative(float radius, float distance) const;
	
	std::vector<Particle> particles;
	FluidSimulationSettings simulationSettings;
};
