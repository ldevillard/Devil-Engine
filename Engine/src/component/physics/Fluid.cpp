#include "component/physics/Fluid.h"

#include "component/Model.h"
#include "system/editor/Editor.h"
#include "system/editor/Gizmo.h"

#pragma region Public Methods

Fluid::Fluid() : Component()
{
	sphereMesh = Model::PrimitivesModels[PrimitiveType::SpherePrimitive]->GetMeshes()[0];
	fluidBoxTransform = Transform();

	resetParticles();

	onPlayModeStopListenerID = Editor::Get().OnPlayModeStop.AddListener([this]()
		{
			resetParticles();
		});

	updateInstanceData();

	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(glm::vec4), instanceData.data(), GL_DYNAMIC_DRAW);

	glBindVertexArray(sphereMesh.GetVAO());
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
	glVertexAttribDivisor(3, 1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

Fluid::~Fluid()
{
	Editor::Get().OnPlayModeStop.RemoveListener(onPlayModeStopListenerID);
	glDeleteBuffers(1, &instanceVBO);
}

void Fluid::Compute()
{
	updateFluidBoxTransform();
	Gizmo::DrawWireCube(Color::Blue, fluidBoxTransform);

	if (!Editor::Get().GetSettings().isPlaying)
	{
		resetParticles();
	}

	updateInstanceData();

	shader->Use();
	Transform().Compute(shader);
	shader->SetBool("useInstancing", true);
	shader->SetBool("textured", false);

	shader->SetVec3("material.ambient", material.Ambient);
	shader->SetVec3("material.diffuse", material.Diffuse);
	shader->SetVec3("material.specular", material.Specular);
	shader->SetFloat("material.shininess", material.Shininess);

	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(glm::vec4), instanceData.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(sphereMesh.GetVAO());
	glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(sphereMesh.Indices.size()), GL_UNSIGNED_INT, 0, static_cast<GLsizei>(instanceData.size()));
	glBindVertexArray(0);
}

void Fluid::Update(float deltaTime)
{
	updateFluidBoxTransform();
	fluidSolver.Update(deltaTime, ParticleRadius, fluidBoxTransform);
}

Component* Fluid::Clone()
{
	Fluid* newFluid = new Fluid();
	
	newFluid->ParticleCount = ParticleCount;
	newFluid->ParticleRadius = ParticleRadius;
	newFluid->FluidBoxWidth = FluidBoxWidth;
	newFluid->FluidBoxHeight = FluidBoxHeight;
	newFluid->FluidBoxDepth = FluidBoxDepth;
	newFluid->material = material;
	newFluid->resetParticles();
	
	return newFluid;
}

nlohmann::ordered_json Fluid::Serialize() const
{
	nlohmann::ordered_json json;

	json["type"] = "Fluid";
	json["particleCount"] = ParticleCount;
	json["particleRadius"] = ParticleRadius;
	json["fluidBoxWidth"] = FluidBoxWidth;
	json["fluidBoxHeight"] = FluidBoxHeight;
	json["fluidBoxDepth"] = FluidBoxDepth;

	return json;
}

void Fluid::Deserialize(const nlohmann::ordered_json& json)
{
	if (json.contains("particleCount"))
	{
		ParticleCount = json["particleCount"];
	}

	if (json.contains("particleRadius"))
	{
		ParticleRadius = json["particleRadius"];
	}

	if (json.contains("fluidBoxWidth"))
	{
		FluidBoxWidth = json["fluidBoxWidth"];
	}

	if (json.contains("fluidBoxHeight"))
	{
		FluidBoxHeight = json["fluidBoxHeight"];
	}

	if (json.contains("fluidBoxDepth"))
	{
		FluidBoxDepth = json["fluidBoxDepth"];
	}

	resetParticles();
}

#pragma endregion

#pragma region Private Methods

void Fluid::resetParticles()
{
	if (transform != nullptr)
	{
		updateFluidBoxTransform();
	}

	fluidSolver.ResetParticles(ParticleCount, ParticleRadius, fluidBoxTransform);
}

void Fluid::updateFluidBoxTransform()
{
	fluidBoxTransform.Position = transform->Position;
	fluidBoxTransform.Rotation = transform->Rotation;
	fluidBoxTransform.Scale = glm::vec3(FluidBoxWidth, FluidBoxHeight, FluidBoxDepth);
}

void Fluid::updateInstanceData()
{
	const std::vector<Particle>& particles = fluidSolver.GetParticles();

	instanceData.clear();
	instanceData.reserve(particles.size());

	for (const Particle& particle : particles)
	{
		instanceData.emplace_back(particle.Position, ParticleRadius);
	}
}

#pragma endregion
