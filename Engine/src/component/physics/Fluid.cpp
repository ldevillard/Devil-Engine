#include "component/physics/Fluid.h"

#include "component/Model.h"
#include "system/editor/Editor.h"
#include "system/editor/Gizmo.h"
#include "physics/Physics.h"

#pragma region Public Methods

Fluid::Fluid() : Component()
{
	sphereMesh = Model::PrimitivesModels[PrimitiveType::SpherePrimitive]->GetMeshes()[0];
	fluidBoxTransform = Transform();
	particles = std::vector<Particle>(ParticleCount, Particle());

	initializeParticleMatrices();

	onPlayModeStopListenerID = Editor::Get().OnPlayModeStop.AddListener([this]()
		{
			initializeParticleMatrices();
		});

	computeParticleMatrices();

	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size() * sizeof(glm::mat4), instanceMatrices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(sphereMesh.GetVAO());
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

	std::size_t vec4Size = sizeof(glm::vec4);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(vec4Size));
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * vec4Size));
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * vec4Size));

	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);

	glBindVertexArray(0);
}

Fluid::~Fluid()
{
	Editor::Get().OnPlayModeStop.RemoveListener(onPlayModeStopListenerID);

	// delete instance VBO
	glDeleteBuffers(1, &instanceVBO);
}

void Fluid::Compute()
{
	updateFluidBoxTransform();
	Gizmo::DrawWireCube(Color::Blue, fluidBoxTransform);

	// update particle matrices from edited settings only if the simulation isn't running 
	if (!Editor::Get().GetSettings().isPlaying)
	{
		initializeParticleMatrices();
	}
	
	// update the instance matrices buffer
	computeParticleMatrices();

	shader->Use();

	// disable entity transform
	Transform().Compute(shader);

	// enable instancing in the shader
	shader->SetBool("useInstancing", true);

	// binding material data
	shader->SetVec3("material.ambient", material.Ambient);
	shader->SetVec3("material.diffuse", material.Diffuse);
	shader->SetVec3("material.specular", material.Specular);
	shader->SetFloat("material.shininess", material.Shininess);

	// update instance VBO with new matrices
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size() * sizeof(glm::mat4), instanceMatrices.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// bind VAO then draw
	glBindVertexArray(sphereMesh.GetVAO());
	glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(sphereMesh.Indices.size()), GL_UNSIGNED_INT, 0, static_cast<GLsizei>(instanceMatrices.size()));
	glBindVertexArray(0);
}

void Fluid::Update(float deltaTime)
{
	for (Particle& particle : particles)
	{
		applyGravity(deltaTime, particle);

		particle.ParticleTransform.Position += glm::vec3(particle.Velocity * deltaTime, 0.0f);
		
		solveBoxCollision(particle);
	}
}

Component* Fluid::Clone()
{
	Fluid* newFluid = new Fluid();
	return newFluid;
}

nlohmann::ordered_json Fluid::Serialize() const
{
	nlohmann::ordered_json json;

	json["type"] = "Fluid";

	return json;
}

void Fluid::Deserialize(const nlohmann::ordered_json& json)
{
	// deserialize implementation
}

#pragma endregion

#pragma region Private Methods

void Fluid::applyGravity(float deltaTime, Particle& particle)
{
	particle.Velocity.y -= Physics::Gravity * deltaTime;
}

void Fluid::solveBoxCollision(Particle& particle)
{
	const glm::vec2 boxHalfExtents = glm::vec2(fluidBoxTransform.Scale.x, fluidBoxTransform.Scale.y) - glm::vec2(ParticleRadius);
	const glm::vec2 boxCenter = glm::vec2(fluidBoxTransform.Position);

	const float boxAngle = glm::radians(fluidBoxTransform.Rotation.z);
	const float cosAngle = std::cos(boxAngle);
	const float sinAngle = std::sin(boxAngle);

	// particle data in world space.
	const glm::vec2 worldPosition = glm::vec2(particle.ParticleTransform.Position);
	const glm::vec2 worldVelocity = particle.Velocity;

	// convert particle data from world space to box local space.
	const glm::vec2 particleWorldOffset = worldPosition - boxCenter;

	glm::vec2 localPosition =
	{
		particleWorldOffset.x * cosAngle + particleWorldOffset.y * sinAngle,
		-particleWorldOffset.x * sinAngle + particleWorldOffset.y * cosAngle
	};

	glm::vec2 localVelocity =
	{
		worldVelocity.x * cosAngle + worldVelocity.y * sinAngle,
		-worldVelocity.x * sinAngle + worldVelocity.y * cosAngle
	};

	const bool hasCollidedOnX = localPosition.x < -boxHalfExtents.x || localPosition.x > boxHalfExtents.x;
	const bool hasCollidedOnY = localPosition.y < -boxHalfExtents.y || localPosition.y > boxHalfExtents.y;

	localPosition = glm::clamp(localPosition, -boxHalfExtents, boxHalfExtents);

	if (hasCollidedOnX)
	{
		localVelocity.x = -localVelocity.x;
	}

	if (hasCollidedOnY)
	{
		localVelocity.y = -localVelocity.y;
	}

	// convert corrected particle data back to world space.
	const glm::vec2 correctedWorldOffset =
	{
		localPosition.x * cosAngle - localPosition.y * sinAngle,
		localPosition.x * sinAngle + localPosition.y * cosAngle
	};

	const glm::vec2 correctedWorldVelocity =
	{
		localVelocity.x * cosAngle - localVelocity.y * sinAngle,
		localVelocity.x * sinAngle + localVelocity.y * cosAngle
	};

	particle.Velocity = correctedWorldVelocity;
	particle.ParticleTransform.Position = glm::vec3(boxCenter + correctedWorldOffset, particle.ParticleTransform.Position.z);
}

void Fluid::initializeParticleMatrices()
{
	particles.clear();
	particles.reserve(ParticleCount);

	int particlesPerRow = static_cast<int>(std::ceil(std::sqrt(ParticleCount)));
	int totalRows = static_cast<int>(std::ceil(static_cast<float>(ParticleCount) / particlesPerRow));

	float spacing = ParticleRadius * 2;

	float offsetX = (particlesPerRow - 1) * spacing / 2.0f;
	float offsetY = (totalRows - 1) * spacing / 2.0f;

	for (int i = 0; i < ParticleCount; ++i)
	{
		int x = i % particlesPerRow;
		int y = i / particlesPerRow;

		glm::vec3 position = glm::vec3(x * spacing - offsetX, y * spacing - offsetY, 0.0f) + fluidBoxTransform.Position;

		glm::mat4 matrix = glm::translate(glm::mat4(1.0f), position);
		matrix = glm::scale(matrix, glm::vec3(ParticleRadius));

		particles.emplace_back();
		particles.back().ParticleTransform = Transform(position, glm::vec3(), glm::vec3(ParticleRadius));
	}
}

void Fluid::updateFluidBoxTransform()
{
	fluidBoxTransform.Position = transform->Position;
	fluidBoxTransform.Rotation = transform->Rotation;
	fluidBoxTransform.Scale = glm::vec3(FluidBoxWidth, FluidBoxHeight, FluidBoxDepth);
}

void Fluid::computeParticleMatrices()
{
	instanceMatrices.clear();
	instanceMatrices.reserve(particles.size());

	for (const Particle& particle : particles)
	{
		instanceMatrices.push_back(particle.ParticleTransform.GetTransformMatrix());
	}
}


#pragma endregion