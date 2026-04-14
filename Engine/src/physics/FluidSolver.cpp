#include "physics/FluidSolver.h"

#include <algorithm>
#include <cmath>
#include <execution>
#include <numbers>
#include <random>

#include "maths/Math.h"
#include "physics/Physics.h"

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
	std::for_each(std::execution::par, particles.begin(), particles.end(),
		[this, deltaTime](Particle& particle)
		{
			applyGravity(deltaTime, particle);
			calculateDensity(particle);
		});
	
	std::for_each(std::execution::par, particles.begin(), particles.end(),
		[this, deltaTime](Particle& particle)
		{
			glm::vec2 pressureForce = calulatePressureForce(particle);
			glm::vec2 pressureAcceleration = pressureForce / particle.Density;
			
			particle.Velocity.x += pressureAcceleration.x * deltaTime;
			particle.Velocity.y += pressureAcceleration.y * deltaTime;
		});
	
	std::for_each(std::execution::par, particles.begin(), particles.end(),
		[this, deltaTime, particleRadius, &fluidBoxTransform, bounceEnergyLoss](Particle& particle)
		{
			particle.Position += particle.Velocity * deltaTime;
			solveBoxCollision(particleRadius, fluidBoxTransform, particle, bounceEnergyLoss);
		});
}

void FluidSolver::SetSmoothingRadius(float value)
{
	smoothingRadius = value;
}

void FluidSolver::SetTargetDensity(float value)
{
	targetDensity = value;
}

void FluidSolver::SetPressureMultiplier(float value)
{
	pressureMultiplier = value;
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
	//std::for_each(std::execution::par, particles.begin(), particles.end(),
	//	[this](Particle& particle)
	//	{
	//		
	//	});
}

glm::vec2 FluidSolver::calulatePressureForce(Particle& particle)
{
	glm::vec2 pressureForce = glm::vec2(0.0f, 0.0f);
	
	for (Particle& p : particles)
	{
		glm::vec2 offset = particle.Position - p.Position;
		float distance = glm::length(offset);
		glm::vec2 direction = distance == 0.0f ? getRandomDir() : offset / distance;
		float slope = smoothingKernelDerivative(smoothingRadius, distance);
		float density = p.Density;
		
		float sharedPressure = calculateSharedPressure(density, particle.Density);
		pressureForce += -sharedPressure * direction * slope * particle.Mass / density;
	
		//pressureForce += -convertDensityToPressure(density) * direction * slope * particle.Mass / density;
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
		float distance = glm::length(p.Position - particle.Position);
		float influence = smoothingKernel(smoothingRadius, distance);
		particle.Density += p.Mass * influence;
	}
}

float FluidSolver::convertDensityToPressure(float density) const
{
	float densityError = density - targetDensity;
	float pressure = densityError * pressureMultiplier;
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
