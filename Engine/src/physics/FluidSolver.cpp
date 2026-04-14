#include "physics/FluidSolver.h"

#include <cmath>

#include "physics/Physics.h"

#pragma region Public Methods

void FluidSolver::ResetParticles(int particleCount, float particleRadius, const Transform& fluidBoxTransform)
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
		
		particles.emplace_back(position, glm::vec3(0.0f));
	}
}

void FluidSolver::Update(float deltaTime, float particleRadius, const Transform& fluidBoxTransform, float bounceEnergyLoss)
{
	for (Particle& particle : particles)
	{
		applyGravity(deltaTime, particle);
		
		particle.Position += particle.Velocity * deltaTime;
		
		solveBoxCollision(particleRadius, fluidBoxTransform, particle, bounceEnergyLoss);
	}
}

const std::vector<Particle>& FluidSolver::GetParticles() const
{
	return particles;
}

#pragma endregion

#pragma region Private Methods

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

#pragma endregion
