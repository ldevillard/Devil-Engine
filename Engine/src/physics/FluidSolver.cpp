#include "physics/FluidSolver.h"

#include <algorithm>
#include <cmath>
#include <execution>
#include <numbers>
#include <random>

#include "maths/Math.h"
#include "physics/Physics.h"

constexpr float MIN_PARTICLE_DENSITY = 0.001f;
constexpr float MIN_PARTICLE_DISTANCE = 0.0001f;
constexpr int SIMULATION_SUBSTEPS = 4;
constexpr float VELOCITY_DAMPING = 0.998f;
constexpr float BOUNDARY_FORCE_MULTIPLIER = 0.5f;


#pragma region Public Methods

void FluidSolver::ResetParticles(int particleCount, float particleRadius, const Transform& fluidBoxTransform, bool useRandomSpawnVelocity)
{
	particles.clear();

	if (particleCount <= 0)
	{
		return;
	}

	particles.reserve(particleCount);

	const int particlesPerRow = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(particleCount))));
	const int totalRows = static_cast<int>(std::ceil(static_cast<float>(particleCount) / particlesPerRow));

	const float spacing = particleRadius * 2.0f;
	
	const float offsetX = (particlesPerRow - 1) * spacing * 0.5f;
	const float offsetY = (totalRows - 1) * spacing * 0.5f;
	
	const float boxAngle = glm::radians(fluidBoxTransform.Rotation.z);
	const float cosAngle = std::cos(boxAngle);
	const float sinAngle = std::sin(boxAngle);

	for (int i = 0; i < particleCount; ++i)
	{
		const int x = i % particlesPerRow;
		const int y = i / particlesPerRow;

		const glm::vec2 localPosition(x * spacing - offsetX, y * spacing - offsetY);
		const glm::vec2 worldOffset(localPosition.x * cosAngle - localPosition.y * sinAngle, localPosition.x * sinAngle + localPosition.y * cosAngle);
		const glm::vec3 position = glm::vec3(glm::vec2(fluidBoxTransform.Position) + worldOffset, fluidBoxTransform.Position.z);
		const glm::vec3 velocity = generateSpawnVelocity(useRandomSpawnVelocity, cosAngle, sinAngle);
		
		particles.emplace_back(position, velocity);
	}
}

void FluidSolver::Update(float deltaTime, float particleRadius, const Transform& fluidBoxTransform, float bounceEnergyLoss)
{
	if (particles.empty() || deltaTime <= 0.0f)
	{
		return;
	}

	const float substepDeltaTime = deltaTime / static_cast<float>(SIMULATION_SUBSTEPS);

	for (int substep = 0; substep < SIMULATION_SUBSTEPS; substep++)
	{
		updateDensities();

		std::for_each(std::execution::par, particles.begin(), particles.end(),
			[this, substepDeltaTime, particleRadius, &fluidBoxTransform](Particle& particle)
			{
				applyGravity(substepDeltaTime, particle);

				const glm::vec2 pressureForce = calulatePressureForce(particle);
				const glm::vec2 boundaryForce = calculateBoundaryForce(particleRadius, fluidBoxTransform, particle);
				const float safeDensity = glm::max(particle.Density, MIN_PARTICLE_DENSITY);
				const glm::vec2 totalAcceleration = (pressureForce + boundaryForce) / safeDensity;

				particle.Velocity.x += totalAcceleration.x * substepDeltaTime;
				particle.Velocity.y += totalAcceleration.y * substepDeltaTime;
				particle.Velocity *= VELOCITY_DAMPING;
			});

		std::for_each(std::execution::par, particles.begin(), particles.end(),
			[this, substepDeltaTime, particleRadius, &fluidBoxTransform, bounceEnergyLoss](Particle& particle)
			{
				particle.Position += particle.Velocity * substepDeltaTime;
				solveBoxCollision(particleRadius, fluidBoxTransform, particle, bounceEnergyLoss);
			});
	}
}

void FluidSolver::SetSmoothingRadius(float value)
{
	smoothingRadius = glm::max(value, MIN_PARTICLE_DISTANCE);
}

void FluidSolver::SetTargetDensity(float value)
{
	targetDensity = glm::max(value, MIN_PARTICLE_DENSITY);
}

void FluidSolver::SetPressureMultiplier(float value)
{
	pressureMultiplier = glm::max(value, 0.0f);
}

const std::vector<Particle>& FluidSolver::GetParticles() const
{
	return particles;
}

#pragma endregion

#pragma region Private Methods

glm::vec3 FluidSolver::generateSpawnVelocity(bool useRandomSpawnVelocity, float cosAngle, float sinAngle)
{
	if (!useRandomSpawnVelocity)
	{
		return glm::vec3(0.0f);
	}

	static std::mt19937 randomGenerator(std::random_device{}());
	std::uniform_real_distribution<float> angleDistribution(0.0f, glm::two_pi<float>());
		
	// hardcoded values for min and max random speed
	std::uniform_real_distribution<float> speedDistribution(0, 20);

	const float angle = angleDistribution(randomGenerator);
	const float speed = speedDistribution(randomGenerator);
	const glm::vec2 localVelocity(std::cos(angle) * speed, std::sin(angle) * speed);
	const glm::vec2 worldVelocity(localVelocity.x * cosAngle - localVelocity.y * sinAngle, localVelocity.x * sinAngle + localVelocity.y * cosAngle);

	return glm::vec3(worldVelocity, 0.0f);
}

glm::vec2 FluidSolver::getRandomDir()
{
	thread_local std::mt19937 randomGenerator(std::random_device{}());
	std::uniform_real_distribution<float> angleDistribution(0.0f, glm::two_pi<float>());

	const float angle = angleDistribution(randomGenerator);
	return glm::vec2(std::cos(angle), std::sin(angle));
}

void FluidSolver::applyGravity(float deltaTime, Particle& particle) const
{
	particle.Velocity.y -= Physics::Gravity * deltaTime;
}

glm::vec2 FluidSolver::calculateBoundaryForce(float particleRadius, const Transform& fluidBoxTransform, const Particle& particle) const
{
	const glm::vec2 boxHalfExtents = glm::max(glm::vec2(fluidBoxTransform.Scale) - glm::vec2(particleRadius), glm::vec2(0.0f));
	const float wallInfluenceDistance = glm::max(smoothingRadius, particleRadius);

	if (boxHalfExtents.x <= 0.0f || boxHalfExtents.y <= 0.0f || wallInfluenceDistance <= 0.0f)
	{
		return glm::vec2(0.0f);
	}

	const glm::vec2 boxCenter = glm::vec2(fluidBoxTransform.Position);
	const float boxAngle = glm::radians(fluidBoxTransform.Rotation.z);
	const float cosAngle = std::cos(boxAngle);
	const float sinAngle = std::sin(boxAngle);

	const glm::vec2 particleWorldOffset = glm::vec2(particle.Position) - boxCenter;
	const glm::vec2 localPosition(particleWorldOffset.x * cosAngle + particleWorldOffset.y * sinAngle, -particleWorldOffset.x * sinAngle + particleWorldOffset.y * cosAngle);

	glm::vec2 localForce(0.0f);
	const auto addWallForce = [wallInfluenceDistance](float distanceToWall, float direction)
	{
		const float influence = 1.0f - glm::clamp(distanceToWall / wallInfluenceDistance, 0.0f, 1.0f);
		return direction * influence * influence;
	};

	localForce.x += addWallForce(localPosition.x + boxHalfExtents.x, 1.0f);
	localForce.x += addWallForce(boxHalfExtents.x - localPosition.x, -1.0f);
	localForce.y += addWallForce(localPosition.y + boxHalfExtents.y, 1.0f);
	localForce.y += addWallForce(boxHalfExtents.y - localPosition.y, -1.0f);

	localForce *= pressureMultiplier * BOUNDARY_FORCE_MULTIPLIER;

	return glm::vec2(localForce.x * cosAngle - localForce.y * sinAngle, localForce.x * sinAngle + localForce.y * cosAngle);
}

void FluidSolver::solveBoxCollision(float particleRadius, const Transform& fluidBoxTransform, Particle& particle, float bounceEnergyLoss) const
{
	const float bounceVelocityFactor = 1.0f - glm::clamp(bounceEnergyLoss, 0.0f, 1.0f);
	const glm::vec2 boxHalfExtents = glm::max(glm::vec2(fluidBoxTransform.Scale) - glm::vec2(particleRadius), glm::vec2(0.0f));
	const glm::vec2 boxCenter = glm::vec2(fluidBoxTransform.Position);
	
	const float boxAngle = glm::radians(fluidBoxTransform.Rotation.z);
	const float cosAngle = std::cos(boxAngle);
	const float sinAngle = std::sin(boxAngle);

	const glm::vec2 worldPosition = glm::vec2(particle.Position);
	const glm::vec2 worldVelocity = glm::vec2(particle.Velocity);
	
	const glm::vec2 particleWorldOffset = worldPosition - boxCenter;

	glm::vec2 localPosition(particleWorldOffset.x * cosAngle + particleWorldOffset.y * sinAngle, -particleWorldOffset.x * sinAngle + particleWorldOffset.y * cosAngle);

	glm::vec2 localVelocity(worldVelocity.x * cosAngle + worldVelocity.y * sinAngle, -worldVelocity.x * sinAngle + worldVelocity.y * cosAngle);

	const bool hasCollidedOnX = localPosition.x < -boxHalfExtents.x || localPosition.x > boxHalfExtents.x;
	const bool hasCollidedOnY = localPosition.y < -boxHalfExtents.y || localPosition.y > boxHalfExtents.y;

	localPosition = glm::clamp(localPosition, -boxHalfExtents, boxHalfExtents);

	if (hasCollidedOnX)
	{
		localVelocity.x = -localVelocity.x * bounceVelocityFactor;
	}

	if (hasCollidedOnY)
	{
		localVelocity.y = -localVelocity.y * bounceVelocityFactor;
	}

	const glm::vec2 correctedWorldOffset(localPosition.x * cosAngle - localPosition.y * sinAngle, localPosition.x * sinAngle + localPosition.y * cosAngle);
	const glm::vec2 correctedWorldVelocity(localVelocity.x * cosAngle - localVelocity.y * sinAngle, localVelocity.x * sinAngle + localVelocity.y * cosAngle);

	particle.Position = glm::vec3(boxCenter + correctedWorldOffset, particle.Position.z);
	particle.Velocity.x = correctedWorldVelocity.x;
	particle.Velocity.y = correctedWorldVelocity.y;
}

void FluidSolver::updateDensities()
{
	std::for_each(std::execution::par, particles.begin(), particles.end(),
		[this](Particle& particle)
		{
			calculateDensity(particle);
		});
}

glm::vec2 FluidSolver::calulatePressureForce(Particle& particle)
{
	glm::vec2 pressureForce = glm::vec2(0.0f, 0.0f);
	
	for (Particle& p : particles)
	{
		const glm::vec2 offset = glm::vec2(particle.Position) - glm::vec2(p.Position);
		float distance = glm::length(offset);
		const glm::vec2 direction = distance <= MIN_PARTICLE_DISTANCE ? getRandomDir() : offset / distance;
		distance = glm::max(distance, MIN_PARTICLE_DISTANCE);

		const float slope = smoothingKernelDerivative(smoothingRadius, distance);
		const float density = glm::max(p.Density, MIN_PARTICLE_DENSITY);
		
		const float sharedPressure = calculateSharedPressure(density, glm::max(particle.Density, MIN_PARTICLE_DENSITY));
		pressureForce += -sharedPressure * direction * slope * particle.Mass / density;
	}
	
	return pressureForce;
}

float FluidSolver::calculateSharedPressure(float densityA, float densityB)
{
	float pressureA = convertDensityToPressure(densityA);
	float pressureB = convertDensityToPressure(densityB);
	return (pressureA + pressureB) / 2.0f;
}

void FluidSolver::calculateDensity(Particle& particle)
{
	particle.Density = 0;
	
	for (Particle& p : particles)
	{
		const float distance = glm::length(glm::vec2(p.Position) - glm::vec2(particle.Position));
		const float influence = smoothingKernel(smoothingRadius, distance);
		particle.Density += p.Mass * influence;
	}
}

float FluidSolver::convertDensityToPressure(float density) const
{
	const float densityError = density - targetDensity;
	const float pressure = glm::max(0.0f, densityError * pressureMultiplier);
	return pressure;
}

float FluidSolver::smoothingKernel(float radius, float distance) const
{
	float volume = static_cast<float>(std::numbers::pi) * std::pow(radius, 8.0f) / 4.0f;
	float value = std::max(0.0f, radius * radius - distance * distance);
	return value * value * value / volume;
	
	//if (distance >= radius)
	//{
	//	return 0.0f;
	//}
	//
	//float volume = (static_cast<float>(std::numbers::pi) * std::pow(radius, 4.0f)) / 6.0f;
	//return (radius - distance) * (radius - distance) / volume;
}

float FluidSolver::smoothingKernelDerivative(float radius, float distance) const
{
	if (distance >= radius)
	{
		return 0.0f;
	}
	float f = radius * radius - distance * distance;
	float scale = -24 / (static_cast<float>(std::numbers::pi) * std::pow(radius, 8.0f));
	return scale * distance * f * f;
	
	//if (distance >= radius)
	//{
	//	return 0.0f;
	//}
	//
	//float scale = 12 / (std::pow(radius, 4.0f) * static_cast<float>(std::numbers::pi));
	//return (distance - radius) * scale;
}

#pragma endregion
