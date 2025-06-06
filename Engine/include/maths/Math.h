#pragma once

#include <maths/glm/glm.hpp>

namespace Math
{
	enum Vec3Format
	{
		XYZ,
		RGB
	};

	// z of screenPos is the depth value
	glm::vec3 ScreenToWorldPoint(const glm::vec2& screenPos, const glm::mat4& view, const glm::mat4& projection, const glm::vec4& viewport);
}