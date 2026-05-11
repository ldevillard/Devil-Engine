#include "physics/FluidSolver.h"

#include <algorithm>
#include <cmath>
#include <execution>
#include <numbers>
#include <random>

#include "physics/Physics.h"

constexpr float MIN_PARTICLE_DENSITY = 0.001f;
constexpr float MIN_PARTICLE_DISTANCE = 0.0001f;
constexpr int SIMULATION_SUBSTEPS = 4;
constexpr float BOUNDARY_FORCE_MULTIPLIER = 0.5f;

#pragma region Public Methods

void FluidSolver::ResetParticles(const FluidSimulationSettings& settings, const Transform& fluidBoxTransform)
{
	simulationSettings = settings;
	particles.clear();

	if (simulationSettings.ParticleCount <= 0)
	{
		return;
	}

	particles.reserve(simulationSettings.ParticleCount);

	const int particlesPerRow = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(simulationSettings.ParticleCount))));
	const int totalRows = static_cast<int>(std::ceil(static_cast<float>(simulationSettings.ParticleCount) / particlesPerRow));

	const float spacing = simulationSettings.ParticleRadius * 2.0f;
	
	const float offsetX = (particlesPerRow - 1) * spacing * 0.5f;
	const float offsetY = (totalRows - 1) * spacing * 0.5f;
	
	const float boxAngle = glm::radians(fluidBoxTransform.Rotation.z);
	const float cosAngle = std::cos(boxAngle);
	const float sinAngle = std::sin(boxAngle);

	for (int i = 0; i < simulationSettings.ParticleCount; ++i)
	{
		const int x = i % particlesPerRow;
		const int y = i / particlesPerRow;

		const glm::vec2 localPosition(x * spacing - offsetX, y * spacing - offsetY);
		const glm::vec2 worldOffset(localPosition.x * cosAngle - localPosition.y * sinAngle, localPosition.x * sinAngle + localPosition.y * cosAngle);
		const glm::vec3 position = glm::vec3(glm::vec2(fluidBoxTransform.Position) + worldOffset, fluidBoxTransform.Position.z);
		const glm::vec3 velocity = generateSpawnVelocity(simulationSettings.UseRandomSpawnVelocity, cosAngle, sinAngle);
		
		particles.emplace_back(position, velocity);
	}

	rebuildSpatialGrid(fluidBoxTransform);
}

void FluidSolver::Update(float deltaTime, const FluidSimulationSettings& settings, const Transform& fluidBoxTransform)
{
	if (particles.empty() || deltaTime <= 0.0f)
	{
		return;
	}

	simulationSettings = settings;
	const float substepDeltaTime = deltaTime / static_cast<float>(SIMULATION_SUBSTEPS);

	for (int substep = 0; substep < SIMULATION_SUBSTEPS; substep++)
	{
		rebuildSpatialGrid(fluidBoxTransform);
		updateDensities();

		std::for_each(std::execution::par, particles.begin(), particles.end(),
			[this, substepDeltaTime, &fluidBoxTransform](Particle& particle)
			{
				applyGravity(substepDeltaTime, particle);

				const glm::vec2 pressureForce = calulatePressureForce(particle);
				const glm::vec2 boundaryForce = calculateBoundaryForce(simulationSettings.ParticleRadius, fluidBoxTransform, particle);
				const float safeDensity = glm::max(particle.Density, MIN_PARTICLE_DENSITY);
				const glm::vec2 totalAcceleration = (pressureForce + boundaryForce) / safeDensity;
				const float velocityDamping = glm::clamp(simulationSettings.VelocityDamping, 0.0f, 1.0f);

				particle.Velocity.x += totalAcceleration.x * substepDeltaTime;
				particle.Velocity.y += totalAcceleration.y * substepDeltaTime;
				particle.Velocity *= velocityDamping;
			});

		std::for_each(std::execution::par, particles.begin(), particles.end(),
			[this, substepDeltaTime, &fluidBoxTransform](Particle& particle)
			{
				particle.Position += particle.Velocity * substepDeltaTime;
				solveBoxCollision(simulationSettings.ParticleRadius, fluidBoxTransform, particle, simulationSettings.BounceEnergyLoss);
			});
	}
}

const std::vector<Particle>& FluidSolver::GetParticles() const
{
	return particles;
}

const SpatialGrid& FluidSolver::GetSpatialGrid() const
{
	return spatialGrid;
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
	const float gravityMultiplier = glm::max(simulationSettings.GravityMultiplier, 0.0f);
	particle.Velocity.y -= Physics::Gravity * gravityMultiplier * deltaTime;
}

glm::vec2 FluidSolver::calculateBoundaryForce(float particleRadius, const Transform& fluidBoxTransform, const Particle& particle) const
{
	const glm::vec2 boxHalfExtents = glm::max(glm::vec2(fluidBoxTransform.Scale) - glm::vec2(particleRadius), glm::vec2(0.0f));
	const float wallInfluenceDistance = glm::max(glm::max(simulationSettings.SmoothingRadius, MIN_PARTICLE_DISTANCE), particleRadius);

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

	localForce *= glm::max(simulationSettings.PressureMultiplier, 0.0f) * BOUNDARY_FORCE_MULTIPLIER;

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

void FluidSolver::rebuildSpatialGrid(const Transform& fluidBoxTransform)
{
	const glm::vec2 boxCenter = glm::vec2(fluidBoxTransform.Position);
	const glm::vec2 boxHalfExtents = glm::max(glm::vec2(fluidBoxTransform.Scale), glm::vec2(0.0f));
	
	const float cellSize = glm::max(simulationSettings.SmoothingRadius, MIN_PARTICLE_DISTANCE);
	
	const float boxAngle = glm::radians(fluidBoxTransform.Rotation.z);
	const float absCosAngle = std::abs(std::cos(boxAngle));
	const float absSinAngle = std::abs(std::sin(boxAngle));

	// The grid stays axis-aligned in world space, so a rotated box needs a world-space AABB
	const glm::vec2 worldAabbHalfExtents(
		absCosAngle * boxHalfExtents.x + absSinAngle * boxHalfExtents.y,
		absSinAngle * boxHalfExtents.x + absCosAngle * boxHalfExtents.y);

	spatialGrid.Origin = boxCenter - worldAabbHalfExtents;
	spatialGrid.CellSize = cellSize;
	
	// Keep one extra cell so clamped positions on the max edge still fit in the grid
	spatialGrid.Width = glm::max(1, static_cast<int>(std::floor((worldAabbHalfExtents.x * 2.0f) / cellSize)) + 1);
	spatialGrid.Height = glm::max(1, static_cast<int>(std::floor((worldAabbHalfExtents.y * 2.0f) / cellSize)) + 1);

	spatialGrid.Cells.assign(spatialGrid.Width * spatialGrid.Height, {});

	const int particleCount = static_cast<int>(particles.size());

	for (int particleIndex = 0; particleIndex < particleCount; particleIndex++)
	{
		// Each cell stores particle indices only, particle data stays in the main array
		const int cellIndex = getCellIndex(getCellCoords(glm::vec2(particles[particleIndex].Position)));
		spatialGrid.Cells[cellIndex].push_back(particleIndex);
	}
}

glm::ivec2 FluidSolver::getCellCoords(const glm::vec2& position) const
{
	const glm::vec2 relativePosition = (position - spatialGrid.Origin) / spatialGrid.CellSize;

	return glm::ivec2(
		glm::clamp(static_cast<int>(std::floor(relativePosition.x)), 0, spatialGrid.Width - 1),
		glm::clamp(static_cast<int>(std::floor(relativePosition.y)), 0, spatialGrid.Height - 1));
}

bool FluidSolver::isCellInBounds(const glm::ivec2& cellCoords) const
{
	return cellCoords.x >= 0 && cellCoords.x < spatialGrid.Width && cellCoords.y >= 0 && cellCoords.y < spatialGrid.Height;
}

int FluidSolver::getCellIndex(const glm::ivec2& cellCoords) const
{
	return cellCoords.y * spatialGrid.Width + cellCoords.x;
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
	const float smoothingRadius = glm::max(simulationSettings.SmoothingRadius, MIN_PARTICLE_DISTANCE);
	const glm::ivec2 particleCell = getCellCoords(glm::vec2(particle.Position));

	for (int offsetX = -1; offsetX <= 1; offsetX++)
	{
		for (int offsetY = -1; offsetY <= 1; offsetY++)
		{
			const glm::ivec2 neighborCell = particleCell + glm::ivec2(offsetX, offsetY);

			if (!isCellInBounds(neighborCell))
			{
				continue;
			}

			for (int neighborIndex : spatialGrid.Cells[getCellIndex(neighborCell)])
			{
				const Particle& p = particles[neighborIndex];

				if (&p == &particle)
				{
					continue;
				}

				const glm::vec2 offset = glm::vec2(particle.Position) - glm::vec2(p.Position);
				float distance = glm::length(offset);
				const glm::vec2 direction = distance <= MIN_PARTICLE_DISTANCE ? getRandomDir() : offset / distance;
				distance = glm::max(distance, MIN_PARTICLE_DISTANCE);

				const float slope = pressureKernelSpikyDerivative(smoothingRadius, distance);
				const float density = glm::max(p.Density, MIN_PARTICLE_DENSITY);

				const float sharedPressure = calculateSharedPressure(density, glm::max(particle.Density, MIN_PARTICLE_DENSITY));
				pressureForce += -sharedPressure * direction * slope * p.Mass / density;
			}
		}
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
	const float smoothingRadius = glm::max(simulationSettings.SmoothingRadius, MIN_PARTICLE_DISTANCE);
	const glm::ivec2 particleCell = getCellCoords(glm::vec2(particle.Position));

	for (int offsetX = -1; offsetX <= 1; ++offsetX)
	{
		for (int offsetY = -1; offsetY <= 1; ++offsetY)
		{
			const glm::ivec2 neighborCell = particleCell + glm::ivec2(offsetX, offsetY);

			if (!isCellInBounds(neighborCell))
			{
				continue;
			}

			for (int neighborIndex : spatialGrid.Cells[getCellIndex(neighborCell)])
			{
				const Particle& p = particles[neighborIndex];
				const float distance = glm::length(glm::vec2(p.Position) - glm::vec2(particle.Position));
				const float influence = densityKernelPoly6(smoothingRadius, distance);
				particle.Density += p.Mass * influence;
			}
		}
	}
}

float FluidSolver::convertDensityToPressure(float density) const
{
	const float densityError = density - glm::max(simulationSettings.TargetDensity, MIN_PARTICLE_DENSITY);
	const float pressure = glm::max(0.0f, densityError * glm::max(simulationSettings.PressureMultiplier, 0.0f));
	return pressure;
}

float FluidSolver::densityKernelPoly6(float radius, float distance) const
{
	if (distance >= radius)
	{
		return 0.0f;
	}

	// 2D poly6 kernel: smooth density estimate with compact support on [0, h].
	const float radiusSquared = radius * radius;
	const float difference = radiusSquared - distance * distance;
	const float differenceCubed = difference * difference * difference;
	const float radiusFourth = radiusSquared * radiusSquared;
	const float radiusEighth = radiusFourth * radiusFourth;
	const float scale = 4.0f / (static_cast<float>(std::numbers::pi) * radiusEighth);
	return differenceCubed * scale;
}

float FluidSolver::pressureKernelSpikyDerivative(float radius, float distance) const
{
	if (distance >= radius)
	{
		return 0.0f;
	}

	// 2D spiky radial derivative: steeper near the center, better for pressure.
	const float radiusDifference = radius - distance;
	const float radiusDifferenceSquared = radiusDifference * radiusDifference;
	const float radiusSquared = radius * radius;
	const float radiusFifth = radiusSquared * radiusSquared * radius;
	const float scale = -30.0f / (static_cast<float>(std::numbers::pi) * radiusFifth);
	return radiusDifferenceSquared * scale;
}

#pragma endregion
