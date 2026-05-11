#pragma once

#include <vector>

#include <maths/glm/glm.hpp>

struct SpatialGrid
{
	std::vector<std::vector<int>> Cells;
	
	glm::vec2 Origin = glm::vec2(0.0f);
	
	float CellSize = 1.0f;
	
	int Width = 0;
	int Height = 0;
};
