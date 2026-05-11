#pragma once

#include <vector>

#include <maths/glm/glm.hpp>

#include "component/Transform.h"
#include "data/physics/Particle.h"
#include "data/physics/SpatialGrid.h"

struct FluidSimulationSettings
{
	int ParticleCount = 1200;
	float ParticleRadius = 0.25f;
	
	float SmoothingRadius = 0.75f;
	float TargetDensity = 4.0f;
	float PressureMultiplier = 500.0f;
	float GravityMultiplier = 1.0f;
	float VelocityDamping = 0.998f;
	float BounceEnergyLoss = 0.75f;
	
	bool UseRandomSpawnVelocity = false;
	bool VelocityColorView = false;
};

class FluidSolver
{
public:
	void ResetParticles(const FluidSimulationSettings& settings, const Transform& fluidBoxTransform);
	void Update(float deltaTime, const FluidSimulationSettings& settings, const Transform& fluidBoxTransform);

	const std::vector<Particle>& GetParticles() const;
	const SpatialGrid& GetSpatialGrid() const;

private:
	// initialization
	glm::vec3 generateSpawnVelocity(bool useRandomSpawnVelocity, float cosAngle, float sinAngle);
	glm::vec2 getRandomDir();
	
	// simulation
	void updateDensities();
	void applyGravity(float deltaTime, Particle& particle) const;
	void solveBoxCollision(float particleRadius, const Transform& fluidBoxTransform, Particle& particle, float bounceEnergyLoss) const;
	
	// computation
	glm::vec2 calculateBoundaryForce(float particleRadius, const Transform& fluidBoxTransform, const Particle& particle) const;
	glm::vec2 calulatePressureForce(Particle& particle);
	float calculateSharedPressure(float densityA, float densityB);
	void calculateDensity(Particle& particle);
	float convertDensityToPressure(float density) const;
	
	// kernels
	float densityKernelPoly6(float radius, float distance) const;
	float pressureKernelSpikyDerivative(float radius, float distance) const;
	
	// spatial grid
	void rebuildSpatialGrid(const Transform& fluidBoxTransform);
	glm::ivec2 getCellCoords(const glm::vec2& position) const;
	int getCellIndex(const glm::ivec2& cellCoords) const;
	bool isCellInBounds(const glm::ivec2& cellCoords) const;
	
	std::vector<Particle> particles;
	SpatialGrid spatialGrid;
	
	FluidSimulationSettings simulationSettings;
};
