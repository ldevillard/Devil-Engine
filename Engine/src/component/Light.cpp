#include "component/Light.h"
#include "system/editor/Gizmo.h"

#pragma region Static Variables

const std::vector<const char*> Light::Names = { "Directional", "Point", "Spot" };

#pragma endregion

#pragma region Public Methods

Light::Light(const LightType& type, const Color& color) : Component(),
	lightType(type), color(color)
{

}

void Light::Compute()
{
	shader->SetInt("lights[" + std::to_string(index) + "].type", static_cast<int>(lightType));

	switch (lightType)
	{
		case Light::Directional:
			computeDirectional();
			break;
		case Light::Point:
			computePoint();
			break;
		case Light::Spot:
			computeSpot();
			break;
	}
}

void Light::Update(float deltaTime)
{
	// do nothing for now
}

Component* Light::Clone()
{
	Light* newLight = new Light(lightType, color);
	newLight->Intensity = Intensity;
	newLight->Radius = Radius;
	newLight->CutOff = CutOff;
	newLight->OutCutOff = OutCutOff;
	return newLight;
}

const glm::vec3 Light::GetDirection() const
{
	return transform->GetForwardVector();
}

void Light::SetLightTypeFromString(const std::string& type)
{
	if (type == "Directional")
		lightType = LightType::Directional;
	else if (type == "Point")
		lightType = LightType::Point;
	else if (type == "Spot")
		lightType = LightType::Spot;
}

void Light::SetIndex(unsigned int i)
{
	index = i;
}

nlohmann::ordered_json Light::Serialize() const
{
	nlohmann::ordered_json json;

	json["type"] = "Light";
	json["lightType"] = lightType;
	json["color"] = color.Serialize();
	json["intensity"] = Intensity;
	json["radius"] = Radius;
	json["cutOff"] = CutOff;
	json["outCutOff"] = OutCutOff;

	return json;
}

void Light::Deserialize(const nlohmann::ordered_json& json)
{
	lightType = json["lightType"];
	color.Deserialize(json["color"]);
	Intensity = json["intensity"];
	Radius = json["radius"];
	CutOff = json["cutOff"];
	OutCutOff = json["outCutOff"];
}

#pragma endregion

#pragma region Private Methods

void Light::computeDirectional()
{
	shader->Use();
	// binding light data
	shader->SetVec3("lights[" + std::to_string(index) + "].direction", transform->GetForwardVector());
	shader->SetVec3("lights[" + std::to_string(index) + "].color", color.Value);
	shader->SetFloat("lights[" + std::to_string(index) + "].intensity", Intensity);
	
	// draw gizmo
	Transform tr(*transform);
	tr.Scale *= 0.5f;
	Gizmo::DrawArrow(Color::Yellow, tr);
}

void Light::computePoint()
{
	shader->Use();
	// binding light data
	shader->SetVec3("lights[" + std::to_string(index) + "].position", transform->Position);
	shader->SetVec3("lights[" + std::to_string(index) + "].color", color.Value);
	shader->SetFloat("lights[" + std::to_string(index) + "].intensity", Intensity * 10);

	// attenuation
	shader->SetFloat("lights[" + std::to_string(index) + "].radius", Radius);

	// draw gizmo
	Transform tr(*transform);
	tr.Scale = glm::vec3(Radius);
	Gizmo::DrawWireSphere(Color::Cyan, tr);

}

void Light::computeSpot()
{
	shader->Use();
	// binding light data
	shader->SetVec3("lights[" + std::to_string(index) + "].position", transform->Position);
	shader->SetVec3("lights[" + std::to_string(index) + "].direction", transform->GetForwardVector());
	shader->SetVec3("lights[" + std::to_string(index) + "].color", color.Value);
	shader->SetFloat("lights[" + std::to_string(index) + "].intensity", Intensity);

	// spot light
	shader->SetFloat("lights[" + std::to_string(index) + "].cutOff", glm::cos(glm::radians(CutOff)));
	shader->SetFloat("lights[" + std::to_string(index) + "].outCutOff", glm::cos(glm::radians(OutCutOff)));

	// draw gizmo
	Gizmo::DrawWireConeFrustum(Color::White, *transform);
}

#pragma endregion