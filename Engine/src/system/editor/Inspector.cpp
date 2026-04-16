#include "system/editor/Inspector.h"

#include <algorithm>

#include "component/Transform.h"
#include "system/editor/Editor.h"
#include "utils/ImGui_Utils.h"

#pragma region Public Methods

Inspector::Inspector()
{
}

Inspector::~Inspector()
{
}

void Inspector::Inspect(const Entity* e)
{
	entity = e;

	if (entity == nullptr)
	{
		throw std::runtime_error("FATAL: try to inspect a null entity!");
	}

	ImGui::Begin("Inspector");

	ImGui::Text(std::string("Entity : " + entity->Name).c_str());
	ImGui::Separator();

	// inspect transform
	inspectTransform();

	// inspect components
	for (Component* c : entity->GetComponents())
	{
		ImGui::Spacing();

		if (typeid(*c) == typeid(Model))
		{
			inspectModel(static_cast<Model*>(c));
		}
		if (typeid(*c) == typeid(Light))
		{
			inspectLight(static_cast<Light*>(c));
		}
		if (typeid(*c) == typeid(Fluid))
		{
			inspectFluid(static_cast<Fluid*>(c));
		}
	}

	ImGui::End();
}

#pragma endregion

#pragma region Private Methods

void Inspector::inspectTransform() const
{
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Transform"))
	{
		glm::vec3& position = entity->transform->Position;
		glm::vec3& rotation = entity->transform->Rotation;
		glm::vec3& scale = entity->transform->Scale;

		ImGui_Utils::DrawVec3Control("Position", position);
		ImGui_Utils::DrawVec3Control("Rotation", rotation);
		ImGui_Utils::DrawVec3Control("Scale", scale, 1.0f);

		entity->transform->SetPosition(position);
		entity->transform->SetRotation(rotation);
		entity->transform->SetScale(scale);

		ImGui::TreePop();
	}
}

int getMaterialIndex(const Material& material)
{
	auto it = std::find(Material::Names.begin(), Material::Names.end(), material.Name);
	if (it != Material::Names.end())
	{
		return static_cast<int>(std::distance(Material::Names.begin(), it));
	}
	else
	{
		return 0;
	}
}

void Inspector::inspectModel(Model* model) const
{
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Model"))
	{
		ImGui::Text("Triangles: %d", model->GetNumberOfTriangles());

		int currentItem = getMaterialIndex(model->GetMaterial());
		ImGui_Utils::DrawComboBoxControl("Material", currentItem, Material::Names);

		ImGui_Utils::DrawColorControl("Ambient Color", model->GetMaterial().Ambient, 135);
		ImGui_Utils::DrawColorControl("Diffuse Color", model->GetMaterial().Diffuse, 135);
		ImGui_Utils::DrawColorControl("Specular Color", model->GetMaterial().Specular, 135);
		
		model->SetMaterialFromName(Material::Names[currentItem]);

		// Ray Tracing
		{
			ImGui::NewLine();
			ImGui::Text("Ray Tracing:");
			ImGui_Utils::DrawBoolControl("Emissive", model->GetMaterial().Emissive);
			
			int currentFlag = model->GetMaterial().Flag;
			ImGui_Utils::DrawComboBoxControl("Flag", currentFlag, Material::Flags);
			model->GetMaterial().SetFlag(currentFlag);

			ImGui_Utils::DrawFloatControl("Emissive Strength", model->GetMaterial().EmissiveStrength, 1.f, 135.f);
			ImGui_Utils::SliderFloat("Smoothness", model->GetMaterial().Smoothness, 0.0f, 1.0f, "%.2f", 135.f);
			ImGui_Utils::SliderFloat("Specular Probability", model->GetMaterial().SpecularProbability, 0.0f, 1.0f, "%.2f", 135.f);
			ImGui_Utils::SliderFloat("Transparancy", model->GetMaterial().Transparancy, 0.0f, 1.0f, "%.2f", 135.f);
		}
		ImGui::TreePop();
	}	
}

void Inspector::inspectLight(Light* light) const
{
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Light"))
	{
		int currentItem = static_cast<int>(light->lightType);

		ImGui_Utils::DrawComboBoxControl("Type", currentItem, Light::Names);
		ImGui_Utils::DrawColorControl("Color", light->color.Value);
		ImGui_Utils::DrawFloatControl("Intensity", light->Intensity, 1.f);
		light->SetLightTypeFromString(Light::Names[currentItem]);
		
		switch (light->lightType)
		{
			case Light::Point:
				ImGui_Utils::DrawFloatControl("Radius", light->Radius, 1.f);
				break;
			case Light::Spot:
				ImGui_Utils::DrawFloatControl("CutOff", light->CutOff, 1.f);
				ImGui_Utils::DrawFloatControl("OutCutOff", light->OutCutOff, 1.f);
				break;
		}
		
		ImGui::TreePop();
	}
}

void Inspector::inspectFluid(Fluid* fluid) const
{
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Fluid"))
	{
		FluidSimulationSettings& settings = fluid->SimulationSettings;

		ImGui_Utils::DrawIntControl("Particle Count", settings.ParticleCount, 100, 175.f);
		ImGui_Utils::DrawFloatControl("Particle Radius", settings.ParticleRadius, 0.25f, 175.f);
		
		ImGui::NewLine();
		
		ImGui_Utils::DrawFloatControl("Smoothing Radius", settings.SmoothingRadius, 0.75f, 175.f);
		ImGui_Utils::DrawFloatControl("Target Density", settings.TargetDensity, 4.0f, 175.f);
		ImGui_Utils::DrawFloatControl("Pressure Multiplier", settings.PressureMultiplier, 500.0f, 175.f);
		ImGui_Utils::DrawFloatControl("Gravity Multiplier", settings.GravityMultiplier, 1.0f, 175.f);
		ImGui_Utils::SliderFloat("Bounce Energy Loss", settings.BounceEnergyLoss, 0.0f, 1.0f, "%.2f", 175.f);
		
		ImGui::NewLine();
		
		ImGui_Utils::DrawBoolControl("Random Spawn Velocity", settings.UseRandomSpawnVelocity, 175.f);
		ImGui_Utils::DrawBoolControl("Velocity Color View", settings.VelocityColorView, 175.f);

		ImGui::NewLine();

		ImGui_Utils::DrawFloatControl("Fluid Box Width", fluid->FluidBoxWidth, 10, 175.f);
		ImGui_Utils::DrawFloatControl("Fluid Box Height", fluid->FluidBoxHeight, 10, 175.f);

		ImGui::TreePop();
	}
}

#pragma endregion
