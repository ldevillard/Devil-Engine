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
	particleVelocity = glm::vec2(0.0f, 0.0f);

	initializeParticleMatrices();

	onPlayModeStopListenerID = Editor::Get().OnPlayModeStop.AddListener([this]()
		{
			initializeParticleMatrices();
			particleVelocity = glm::vec2(0.0f, 0.0f);
		});

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
	applyGravity(deltaTime);
	
	for (glm::mat4& particle : instanceMatrices)
	{
		particle[3] += glm::vec4(particleVelocity.x * deltaTime, particleVelocity.y * deltaTime, 0.0f, 0.0f);

		solveBoxCollision(particle);
	}

	std::cout << particleVelocity.y << std::endl;
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

void Fluid::applyGravity(float deltaTime)
{
	particleVelocity.y -= Physics::Gravity * deltaTime;
}

void Fluid::solveBoxCollision(glm::mat4& particle)
{
	const glm::vec2 halfBoundsSize = glm::vec2(fluidBoxTransform.Scale.x, fluidBoxTransform.Scale.y) - glm::vec2(ParticleRadius);

	glm::vec3 position = glm::vec3(particle[3]);
	const glm::vec2 center = glm::vec2(fluidBoxTransform.Position);

	const glm::vec2 minBounds = center - halfBoundsSize;
	const glm::vec2 maxBounds = center + halfBoundsSize;

	if (position.x < minBounds.x || position.x > maxBounds.x)
	{
		position.x = glm::clamp(position.x, minBounds.x, maxBounds.x);
		particleVelocity.x *= -1.0f;
	}

	if (position.y < minBounds.y || position.y > maxBounds.y)
	{
		position.y = glm::clamp(position.y, minBounds.y, maxBounds.y);
		particleVelocity.y *= -1.0f;
	}

	particle[3] = glm::vec4(position, 1.0f);
}

void Fluid::initializeParticleMatrices()
{
	instanceMatrices.clear();

	int particlesPerRow = static_cast<int>(std::ceil(std::sqrt(ParticleCount)));
	int totalRows = static_cast<int>(std::ceil(static_cast<float>(ParticleCount) / particlesPerRow));

	float spacing = ParticleRadius * 2;

	float offsetX = (particlesPerRow - 1) * spacing / 2.0f;
	float offsetY = (totalRows - 1) * spacing / 2.0f;

	for (int i = 0; i < ParticleCount; ++i)
	{
		int x = i % particlesPerRow;
		int y = i / particlesPerRow;

		glm::vec3 position = glm::vec3(x * spacing - offsetX, y * spacing - offsetY, 0.0f);

		glm::mat4 matrix = glm::translate(glm::mat4(1.0f), position);
		matrix = glm::scale(matrix, glm::vec3(ParticleRadius));

		instanceMatrices.push_back(matrix);
	}
}

void Fluid::updateFluidBoxTransform()
{
	fluidBoxTransform.Position = transform->Position;
	fluidBoxTransform.Rotation = transform->Rotation;
	fluidBoxTransform.Scale = glm::vec3(FluidBoxWidth, FluidBoxHeight, FluidBoxDepth);
}

#pragma endregion