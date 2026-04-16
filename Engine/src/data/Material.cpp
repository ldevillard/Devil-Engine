#include "data/Material.h"

#include <unordered_map>

#include "utils/serializer/SerializerUtils.h"

#pragma region Default Materials

const std::vector<const char*> Material::Names = 
{ 
	"Default", 
	"Black Plastic",
	"Emerald", 
	"Chrome", 
	"Turquoise", 
	"Gold", 
	"Silver", 
	"Bronze", 
	"Ruby", 
	"Sapphire", 
	"Cyan",
	"Topaz", 
	"Prune",
};

const std::vector<const char*> Material::Flags =
{
	"None",
	"CheckerPattern",
	"HideEmissive"
};

const Material& Material::GetMaterialFromName(const std::string& name)
{
	static const std::unordered_map<std::string, const Material*> materialMap = {
		{"Default", &Material::Default},
		{"Black Plastic", &Material::BlackPlastic},
		{"Emerald", &Material::Emerald},
		{"Chrome", &Material::Chrome},
		{"Turquoise", &Material::Turquoise},
		{"Gold", &Material::Gold},
		{"Silver", &Material::Silver},
		{"Bronze", &Material::Bronze},
		{"Ruby", &Material::Ruby},
		{"Sapphire", &Material::Sapphire},
		{"Cyan", &Material::Cyan},
		{"Topaz", &Material::Topaz},
		{"Prune", &Material::Prune}
	};

	auto it = materialMap.find(name);
	if (it != materialMap.end()) {
		return *(it->second);
	}
	else {
		return Material::Default;
	}
}

const Material Material::Default(glm::vec3(.2f), // Ambient
	glm::vec3(1.f), // Diffuse
	glm::vec3(1.f), // Specular
	0.6f, // Shininess
	"Default"); // Name

const Material Material::BlackPlastic(glm::vec3(0.025f), // Ambient
	glm::vec3(0.025f), // Diffuse
	glm::vec3(0.5f), // Specular
	0.25f, // Shininess
	"Black Plastic"); // Name

const Material Material::Emerald(glm::vec3(0.0215f, 0.1745f, 0.0215f), // Ambient
	glm::vec3(0.07568f, 0.61424f, 0.07568f), // Diffuse
	glm::vec3(0.633f, 0.727811f, 0.633f), // Specular
	0.6f, // Shininess
	"Emerald"); // Name

const Material Material::Chrome(glm::vec3(0.25f, 0.25f, 0.25f), // Ambient
	glm::vec3(0.4f, 0.4f, 0.4f), // Diffuse
	glm::vec3(0.774597f, 0.774597f, 0.774597f), // Specular
	0.6f, // Shininess
	"Chrome"); // Name

const Material Material::Turquoise(glm::vec3(0.1f, 0.18725f, 0.1745f), // Ambient
	glm::vec3(0.396f, 0.74151f, 0.69102f), // Diffuse
	glm::vec3(0.297254f, 0.30829f, 0.306678f), // Specular
	0.1f, // Shininess
	"Turquoise"); // Name

const Material Material::Gold(glm::vec3(0.24725f, 0.1995f, 0.0745f), // Ambient
	glm::vec3(0.75164f, 0.60648f, 0.22648f), // Diffuse
	glm::vec3(0.628281f, 0.555802f, 0.366065f), // Specular
	0.4f, // Shininess
	"Gold"); // Name

const Material Material::Silver(glm::vec3(0.19225f, 0.19225f, 0.19225f), // Ambient
	glm::vec3(0.50754f, 0.50754f, 0.50754f), // Diffuse
	glm::vec3(0.508273f, 0.508273f, 0.508273f), // Specular
	0.4f, // Shininess
	"Silver"); // Name

const Material Material::Bronze(glm::vec3(0.2125f, 0.1275f, 0.054f), // Ambient
	glm::vec3(0.714f, 0.4284f, 0.18144f), // Diffuse
	glm::vec3(0.393548f, 0.271906f, 0.166721f), // Specular
	0.2f, // Shininess
	"Bronze"); // Name

const Material Material::Ruby(glm::vec3(0.1745f, 0.01175f, 0.01175f), // Ambient
	glm::vec3(0.61424f, 0.04136f, 0.04136f), // Diffuse
	glm::vec3(0.727811f, 0.626959f, 0.626959f), // Specular
	0.6f, // Shininess
	"Ruby"); // Name

const Material Material::Sapphire(glm::vec3(0.01175f, 0.01175f, 0.1745f), // Ambient
	glm::vec3(0.04136f, 0.04136f, 0.61424f), // Diffuse
	glm::vec3(0.626959f, 0.626959f, 0.727811f), // Specular
	0.6f, // Shininess
	"Sapphire"); // Name

const Material Material::Cyan(glm::vec3(0.0f, 0.1f, 0.12f), // Ambient
	glm::vec3(0.0f, 0.7f, 0.8f), // Diffuse
	glm::vec3(0.7f, 0.95f, 0.95f), // Specular
	0.35f, // Shininess
	"Cyan"); // Name

const Material Material::Topaz(glm::vec3(0.2f, 0.2f, 0.2f), // Ambient
	glm::vec3(0.8f, 0.8f, 0.8f), // Diffuse
	glm::vec3(0.8f, 0.8f, 0.8f), // Specular
	0.4f, // Shininess
	"Topaz"); // Name

const Material Material::Prune(glm::vec3(0.439216f, 0.172549f, 0.521569f), // Ambient
	glm::vec3(0.615686f, 0.286275f, 0.729412f), // Diffuse
	glm::vec3(0.811804f, 0.811804f, 0.811804f), // Specular
	0.4f, // Shininess
	"Prune"); // Name

#pragma endregion

#pragma region Public Methods

void Material::SetEmissive(bool emissive)
{
	Emissive = emissive;
}

void Material::SetFlag(int flag)
{
	Flag = flag;
}

nlohmann::ordered_json Material::Serialize() const
{
	nlohmann::ordered_json json;

	json["ambient"] = Serializer::Serialize(Ambient, Math::Vec3Format::RGB);
	json["diffuse"] = Serializer::Serialize(Diffuse, Math::Vec3Format::RGB);
	json["specular"] = Serializer::Serialize(Specular, Math::Vec3Format::RGB);
	json["shininess"] = Shininess;
	json["emissive"] = Emissive;
	json["flag"] = Flag;
	json["emissiveStrength"] = EmissiveStrength;
	json["smoothness"] = Smoothness;
	json["specularProbability"] = SpecularProbability;
	json["name"] = Name;

	return json;
}

void Material::Deserialize(const nlohmann::ordered_json& json)
{
	Ambient = Serializer::Deserialize(json["ambient"], Math::Vec3Format::RGB);
	Diffuse = Serializer::Deserialize(json["diffuse"], Math::Vec3Format::RGB);
	Specular = Serializer::Deserialize(json["specular"], Math::Vec3Format::RGB);
	Shininess = json["shininess"];
	Emissive = json["emissive"];
	Flag= json["flag"];
	EmissiveStrength = json["emissiveStrength"];
	Smoothness = json["smoothness"];
	SpecularProbability = json["specularProbability"];
	Name = json["name"];
}

#pragma endregion

#pragma region Private Methods

Material::Material(const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular, float shininess, std::string name)
	: Ambient(ambient)
	, Diffuse(diffuse)
	, Specular(specular)
	, Shininess(shininess)
	, Name(name)
{
}

#pragma endregion
