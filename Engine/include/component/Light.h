#pragma once

#include <vector>
#include <string>

#include "component/Component.h"
#include "data/Color.h"

class Light : public Component
{
public:
	enum LightType
	{
		Directional,
		Point,
		Spot
	};

	Light() = default;
	Light(const LightType& type, const Color& color = Color::White);

	void Compute() override;
	void Update(float deltaTime) override;

	Component* Clone() override;

	const glm::vec3 GetDirection() const;
	
	void SetLightTypeFromString(const std::string& type);
	static const std::vector<const char*> Names; // = { "Directional", "Point", "Spot" };

	void SetIndex(unsigned int i);

	// serialization
	nlohmann::ordered_json Serialize() const override;
	void Deserialize(const nlohmann::ordered_json& json) override;

	LightType lightType;
	Color	  color;
	float	  Intensity = 1.0f;

	// point light
	float	  Radius = 1.0f;

	// spot light
	float	  CutOff = 12.5f;
	float	  OutCutOff = 17.5f;

private:
	unsigned int index = 0;

	void computeDirectional();
	void computePoint();
	void computeSpot();
};

REGISTER_COMPONENT_TYPE(Light);