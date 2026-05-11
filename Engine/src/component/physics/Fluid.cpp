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

	if (!Editor::Get().GetSettings().isPlaying)
	{
		resetParticles();
	}

	Gizmo::DrawWireCube(Color::Blue, fluidBoxTransform);
	drawSpatialGridGizmos();

	updateInstanceData();

	shader->Use();
	Transform().Compute(shader);
	shader->SetBool("useInstancing", true);
	shader->SetBool("useInstanceColor", SimulationSettings.VelocityColorView);
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
	fluidSolver.Update(fixedDeltaTime, SimulationSettings, fluidBoxTransform);
}

Component* Fluid::Clone()
{
	Fluid* newFluid = new Fluid();
	
	newFluid->SimulationSettings = SimulationSettings;
	newFluid->FluidBoxWidth = FluidBoxWidth;
	newFluid->FluidBoxHeight = FluidBoxHeight;
	newFluid->FluidBoxDepth = FluidBoxDepth;
	newFluid->ParticleColorMaxSpeed = ParticleColorMaxSpeed;
	newFluid->DebugSpatialGrid = DebugSpatialGrid;
	newFluid->material = material;
	newFluid->resetParticles();
	
	return newFluid;
}

nlohmann::ordered_json Fluid::Serialize() const
{
	nlohmann::ordered_json json;

	json["type"] = "Fluid";
	json["fluidBoxWidth"] = FluidBoxWidth;
	json["fluidBoxHeight"] = FluidBoxHeight;
	json["fluidBoxDepth"] = FluidBoxDepth;
	json["particleColorMaxSpeed"] = ParticleColorMaxSpeed;

	return json;
}

void Fluid::Deserialize(const nlohmann::ordered_json& json)
{
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

	FluidSimulationSettings spawnSettings = SimulationSettings;
	spawnSettings.UseRandomSpawnVelocity = SimulationSettings.UseRandomSpawnVelocity && Editor::Get().GetSettings().isPlaying;
	fluidSolver.ResetParticles(spawnSettings, fluidBoxTransform);
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
		instanceData.emplace_back(particle.Position, SimulationSettings.ParticleRadius);
		instanceColors.emplace_back(computeParticleColor(particle));
	}
}

void Fluid::drawSpatialGridGizmos() const
{
	if (!DebugSpatialGrid)
	{
		return;
	}

	const SpatialGrid& spatialGrid = fluidSolver.GetSpatialGrid();

	if (spatialGrid.Width <= 0 || spatialGrid.Height <= 0 || spatialGrid.CellSize <= 0.0f)
	{
		return;
	}

	const int cellCount = spatialGrid.Width * spatialGrid.Height;
	const glm::vec3 cellScale(spatialGrid.CellSize * 0.5f, spatialGrid.CellSize * 0.5f,glm::max(fluidBoxTransform.Scale.z, 0.05f));

	std::vector<glm::mat4> occupiedCellMatrices;

	occupiedCellMatrices.reserve(cellCount);

	for (int cellY = 0; cellY < spatialGrid.Height; cellY++)
	{
		for (int cellX = 0; cellX < spatialGrid.Width; cellX++)
		{
			const int cellIndex = cellY * spatialGrid.Width + cellX;
			
			const glm::vec3 cellPosition(
				spatialGrid.Origin.x + (static_cast<float>(cellX) + 0.5f) * spatialGrid.CellSize,
				spatialGrid.Origin.y + (static_cast<float>(cellY) + 0.5f) * spatialGrid.CellSize,
				fluidBoxTransform.Position.z);
			
			const glm::mat4 cellMatrix = Transform(cellPosition, glm::vec3(0.0f), cellScale).GetTransformMatrix();
			
			if (!spatialGrid.Cells[cellIndex].empty())
			{
				occupiedCellMatrices.push_back(cellMatrix);
			}
		}
	}

	Gizmo::DrawWireCubeInstanced(Color::Cyan, occupiedCellMatrices);
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
