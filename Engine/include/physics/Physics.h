#pragma once

#include "data/physics/Ray.h"
#include "data/physics/RaycastHit.h"

namespace Physics
{
	bool EditorRaycast(const Ray& ray, RaycastHit& outRayCastHit);
}