#pragma once

// glm
#include <maths/glm/glm.hpp>

#include <string>
#include <vector>

#include "utils/serializer/json/json.hpp"

// Phong model
class Material
{
public:
	// Default materials
	const static Material Default;
	const static Material BlackPlastic;

	const static Material Emerald;
	const static Material Chrome;
	const static Material Turquoise;
	const static Material Gold;
	const static Material Silver;
	const static Material Bronze;
	const static Material Ruby;
	const static Material Sapphire;
	const static Material Cyan;
	const static Material Topaz;
	const static Material Prune;

	static const std::vector<const char*> Names;
	static const std::vector<const char*> Flags;
	static const Material& GetMaterialFromName(const std::string& name);

	void SetEmissive(bool emissive);
	void SetFlag(int flag);

	// serialization
	nlohmann::ordered_json Serialize() const;
	void Deserialize(const nlohmann::ordered_json& json);

	// These vectors are the colors of the different lights
	glm::vec3 Ambient;
	glm::vec3 Diffuse;
	glm::vec3 Specular;
	
	// This impact the radius of the specular light
	float Shininess = 0.6f;

	bool Emissive = false;
	int Flag = 0;
	float EmissiveStrength = 1.0f;
	float Smoothness = 0.0f;
	float SpecularProbability = 0.0f;
	float Transparancy = 0.0f;

	std::string Name;

private:
	// Private constructor to set up default materials
	Material(const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular, float shininess, std::string name = "Custom");
};