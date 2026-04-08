#pragma once

#include "component/Component.h"
#include "component/Transform.h"
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
	float ParticleRadius = 0.25f;

	float FluidBoxWidth = 10;
	float FluidBoxHeight = 10;
	float FluidBoxDepth = 1;

private:
	// render data
	Mesh sphereMesh;
	Material material = Material::Sapphire;
	unsigned int instanceVBO = 0;

	// fluid simulation data
	Transform fluidBoxTransform;
	std::vector<glm::mat4> instanceMatrices;

	void computeParticleMatrices();
	void updateFluidBoxTransform();
};

REGISTER_COMPONENT_TYPE(Fluid);