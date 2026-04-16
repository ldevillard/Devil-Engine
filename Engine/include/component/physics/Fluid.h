#pragma once

#include <vector>

#include "component/Component.h"
#include "component/Transform.h"
#include "data/Material.h"
#include "data/mesh/Mesh.h"
#include "physics/FluidSolver.h"
#include "utils/Event.h"

class Fluid : public Component
{
public:
	Fluid();
	~Fluid();

	void Compute() override;
	void FixedUpdate(float fixedDeltaTime) override;
	
	Component* Clone() override;
	
	// serialization
	nlohmann::ordered_json Serialize() const override;
	void Deserialize(const nlohmann::ordered_json& json) override;

	FluidSimulationSettings SimulationSettings;
	
	float ParticleColorMaxSpeed = 15.0f;
	
	float FluidBoxWidth = 25;
	float FluidBoxHeight = 10;
	float FluidBoxDepth = 1;

private:
	// event listeners
	Event<>::ListenerID onPlayModeStartListenerID;
	Event<>::ListenerID onPlayModeStopListenerID;

	// render data
	Mesh sphereMesh;
	Material material = Material::Sapphire;
	unsigned int instanceVBO = 0;
	unsigned int instanceColorVBO = 0;
	std::vector<glm::vec4> instanceData;
	std::vector<glm::vec3> instanceColors;

	// fluid simulation data
	Transform fluidBoxTransform;
	FluidSolver fluidSolver;

	void resetParticles();
	void updateFluidBoxTransform();
	void updateInstanceData();
	glm::vec3 computeParticleColor(const Particle& particle) const;
};

REGISTER_COMPONENT_TYPE(Fluid);
