#pragma once

#include "component/Component.h"
#include "component/Transform.h"
#include "data/mesh/Mesh.h"
#include "data/Material.h"
#include "utils/Event.h"

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

	int ParticleCount = 1;
	float ParticleRadius = 0.25f;

	float FluidBoxWidth = 10;
	float FluidBoxHeight = 10;
	float FluidBoxDepth = 1;

private:
	// event listeners
	Event<>::ListenerID onPlayModeStopListenerID;

	// render data
	Mesh sphereMesh;
	Material material = Material::Sapphire;
	unsigned int instanceVBO = 0;

	// fluid simulation data
	Transform fluidBoxTransform;
	std::vector<glm::mat4> instanceMatrices;
	// TODO: Make an object for particles that will embed transform, velocity, and other properties
	glm::vec2 particleVelocity;

	// simulation
	void applyGravity(float deltaTime);
	void solveBoxCollision(glm::mat4& particle);

	void initializeParticleMatrices();
	void updateFluidBoxTransform();
};

REGISTER_COMPONENT_TYPE(Fluid);