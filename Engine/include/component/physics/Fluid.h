#pragma once

#include "component/Component.h"
#include "data/mesh/Mesh.h"
#include "data/Material.h"

class Fluid : public Component
{
public:
	Fluid();
	~Fluid();

	void Compute() override;
	void Update(float deltaTime) override;
	Component* Clone() override;
	
	// serialization
	nlohmann::ordered_json Serialize() const override;
	void Deserialize(const nlohmann::ordered_json& json) override;

	int ParticleCount = 100;

private:
	// render data
	Mesh sphereMesh;
	Material material = Material::Sapphire;
	unsigned int instanceVBO = 0;

	// init logic
	std::vector<glm::vec3> instancePositions;
	void computeParticlesPosition();
};

REGISTER_COMPONENT_TYPE(Fluid);