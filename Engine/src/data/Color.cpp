#include "data/Color.h"

#include "utils/serializer/SerializerUtils.h"

#pragma region Statics

Color Color::Black = Color(glm::vec3(0));
Color Color::White = Color(glm::vec3(1));
Color Color::Red = Color(glm::vec3(1, 0, 0));
Color Color::Green = Color(glm::vec3(0, 1, 0));
Color Color::Blue = Color(glm::vec3(0, 0, 1));
Color Color::Yellow = Color(glm::vec3(1, 1, 0));
Color Color::Magenta = Color(glm::vec3(1, 0, 1));
Color Color::Cyan = Color(glm::vec3(0, 1, 1));
Color Color::Orange = Color(glm::vec3(1, 0.5, 0));

#pragma endregion

#pragma region Constructors

Color::Color()
{
	Value = glm::vec3(1);
}

Color::Color(const Color& color)
{
	this->Value = color.Value;
}

Color::Color(const glm::vec3& color)
{
	this->Value = color;
}

Color::~Color()
{
}

#pragma endregion

#pragma region Operators

Color& Color::operator=(const Color& other)
{
	Value = other.Value;
	return *this;
}

Color Color::operator+(const Color& other) const
{
	return Color(Value + other.Value);
}

Color Color::operator-(const Color& other) const
{
	return Color(Value - other.Value);
}

Color Color::operator*(const Color& other) const
{
	return Color(Value * other.Value);
}

Color Color::operator*(float scalar) const
{
	return Color(Value * scalar);
}

Color Color::operator/(float scalar) const
{
	return Color(Value / scalar);
}

#pragma endregion

#pragma region Public Methods

// Hue, Saturation and Luminosity
glm::vec3 Color::HSLToRGB(float h, float s, float v)
{
    float r = 0;
    float g = 0;
    float b = 0;

    int i = static_cast<int>(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (i % 6) 
    {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }

    return glm::vec3(r, g, b);
}

// Hue, Saturation and Value
glm::vec3 Color::HSVToRGB(float h, float s, float v)
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;

    int i = static_cast<int>(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    switch (i % 6)
    {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }

    return glm::vec3(r, g, b);
}

nlohmann::ordered_json Color::Serialize() const
{
	return Serializer::Serialize(Value, Math::Vec3Format::RGB);
}

void Color::Deserialize(const nlohmann::ordered_json& json)
{
	Value = Serializer::Deserialize(json, Math::Vec3Format::RGB);
}

#pragma endregion
