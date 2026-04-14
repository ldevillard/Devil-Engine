#include "component/physics/Fluid.h"

#include "component/Model.h"
#include "data/Color.h"
#include "system/editor/Editor.h"
#include "system/editor/Gizmo.h"

#pragma region Public Methods

Fluid::Fluid() : Component()
{
	sphereMesh = Model::PrimitivesModels[PrimitiveType::SpherePrimitive]->GetMeshes()[0];
	fluidBoxTransform = Transform();

	resetParticles();

	onPlayModeStartListenerID = Editor::Get().OnPlayModeStart.AddListener([this]()
		{
			resetParticles();
		});

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

	glGenBuffers(1, &instanceColorVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceColorVBO);
	glBufferData(GL_ARRAY_BUFFER, instanceColors.size() * sizeof(glm::vec3), instanceColors.data(), GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glVertexAttribDivisor(4, 1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

Fluid::~Fluid()
{
	Editor::Get().OnPlayModeStart.RemoveListener(onPlayModeStartListenerID);
	Editor::Get().OnPlayModeStop.RemoveListener(onPlayModeStopListenerID);
	glDeleteBuffers(1, &instanceVBO);
	glDeleteBuffers(1, &instanceColorVBO);
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
	shader->SetBool("useInstanceColor", true);
	shader->SetBool("textured", false);

	shader->SetVec3("material.ambient", material.Ambient);
	shader->SetVec3("material.diffuse", material.Diffuse);
	shader->SetVec3("material.specular", material.Specular);
	shader->SetFloat("material.shininess", material.Shininess);

	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(glm::vec4), instanceData.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, instanceColorVBO);
	glBufferData(GL_ARRAY_BUFFER, instanceColors.size() * sizeof(glm::vec3), instanceColors.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(sphereMesh.GetVAO());
	glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(sphereMesh.Indices.size()), GL_UNSIGNED_INT, 0, static_cast<GLsizei>(instanceData.size()));
	glBindVertexArray(0);
}

void Fluid::FixedUpdate(float fixedDeltaTime)
{
	updateFluidBoxTransform();
	fluidSolver.Update(fixedDeltaTime, ParticleRadius, fluidBoxTransform, BounceEnergyLoss);
}

Component* Fluid::Clone()
{
	Fluid* newFluid = new Fluid();
	
	newFluid->ParticleCount = ParticleCount;
	newFluid->ParticleRadius = ParticleRadius;
	newFluid->FluidBoxWidth = FluidBoxWidth;
	newFluid->FluidBoxHeight = FluidBoxHeight;
	newFluid->FluidBoxDepth = FluidBoxDepth;
	newFluid->ParticleColorMaxSpeed = ParticleColorMaxSpeed;
	newFluid->BounceEnergyLoss = BounceEnergyLoss;
	newFluid->UseRandomSpawnVelocity = UseRandomSpawnVelocity;
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
	json["particleColorMaxSpeed"] = ParticleColorMaxSpeed;
	json["bounceEnergyLoss"] = BounceEnergyLoss;
	json["useRandomSpawnVelocity"] = UseRandomSpawnVelocity;

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

	if (json.contains("particleColorMaxSpeed"))
	{
		ParticleColorMaxSpeed = json["particleColorMaxSpeed"];
	}

	if (json.contains("bounceEnergyLoss"))
	{
		BounceEnergyLoss = json["bounceEnergyLoss"];
	}

	if (json.contains("useRandomSpawnVelocity"))
	{
		UseRandomSpawnVelocity = json["useRandomSpawnVelocity"];
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

	const bool useRandomSpawnVelocity = UseRandomSpawnVelocity && Editor::Get().GetSettings().isPlaying;
	fluidSolver.ResetParticles(ParticleCount, ParticleRadius, fluidBoxTransform, useRandomSpawnVelocity);
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
	instanceColors.clear();
	instanceColors.reserve(particles.size());

	for (const Particle& particle : particles)
	{
		instanceData.emplace_back(particle.Position, ParticleRadius);
		instanceColors.push_back(computeParticleColor(particle));
	}
}

glm::vec3 Fluid::computeParticleColor(const Particle& particle) const
{
	const float speed = glm::length(particle.Velocity);
	const float maxSpeed = glm::max(ParticleColorMaxSpeed, 0.001f);
	const float speedT = glm::clamp(speed / maxSpeed, 0.0f, 1.0f);
	
	const float hue = glm::mix(240.0f / 360.0f, 0.0f, speedT);
	const float saturation = 0.65f;
	const float value = 0.6f;

	return Color::HSVToRGB(hue, saturation, value);
}

#pragma endregion
