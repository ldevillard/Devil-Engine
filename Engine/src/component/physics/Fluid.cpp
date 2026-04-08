#include "component/physics/Fluid.h"

#include "component/Model.h"
#include "system/editor/Gizmo.h"

#pragma region Public Methods

Fluid::Fluid() : Component()
{
	sphereMesh = Model::PrimitivesModels[PrimitiveType::SpherePrimitive]->GetMeshes()[0];

	computeParticlesPosition();

	// create instance VBO
	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, instancePositions.size() * sizeof(glm::vec3), instancePositions.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(sphereMesh.GetVAO());
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

	// instances position
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glVertexAttribDivisor(3, 1);

	glBindVertexArray(0);
}

Fluid::~Fluid()
{
	// delete instance VBO
	glDeleteBuffers(1, &instanceVBO);
}

void Fluid::Compute()
{
	Gizmo::DrawWireCube(Color::Blue, *transform);
	
	// binding material data
	shader->Use();
	shader->SetVec3("material.ambient", material.Ambient);
	shader->SetVec3("material.diffuse", material.Diffuse);
	shader->SetVec3("material.specular", material.Specular);
	shader->SetFloat("material.shininess", material.Shininess);

	// update instance VBO with new positions
	computeParticlesPosition();
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, instancePositions.size() * sizeof(glm::vec3), instancePositions.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// bind VAO then draw
	glBindVertexArray(sphereMesh.GetVAO());
	glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(sphereMesh.Indices.size()), GL_UNSIGNED_INT, 0, static_cast<GLsizei>(instancePositions.size()));
	glBindVertexArray(0);
}

void Fluid::Update(float deltaTime)
{
	// TODO: implement fluid simulation logic here
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

void Fluid::computeParticlesPosition()
{
	instancePositions.clear();

	// grid dimension
	int gridSize = static_cast<int>(std::cbrt(ParticleCount));
	// particles spacing
	float spacing = 2.0f;
	float offset = (gridSize - 1) * spacing / 2.0f;

	// compute default positions of particles in a fluid simulation
	for (int i = 0; i < ParticleCount; ++i)
	{
		int x = i % gridSize;
		int y = (i / gridSize) % gridSize;
		int z = i / (gridSize * gridSize);

		glm::vec3 position = glm::vec3(x * spacing - offset, y * spacing - offset, z * spacing - offset);
		instancePositions.push_back(position);
	}
}

#pragma endregion