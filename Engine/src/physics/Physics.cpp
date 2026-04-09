#include "physics/Physics.h"

#include <cassert>

#include "maths/Math.h"
#include "system/entity/EntityManager.h"

namespace Physics
{
	bool EditorRaycast(const Ray& ray, RaycastHit& outRayCastHit)
	{
		const std::vector<Entity*>& entities = EntityManager::Get().GetEntities();

		for (Entity* e : entities)
		{
			const EditorCollider* collider = e->GetEditorCollider();
			
			assert(collider != nullptr && "Entity has no editor collider -> mouse picking purpose");

			RaycastHit hit;
			Model* model = nullptr;
			if (e->TryGetComponent<Model>(model))
				collider->IntersectRayBVH(ray, hit);
			else
				collider->IntersectRayBoundingBox(ray, hit);

			if (hit.hitInfo.distance < outRayCastHit.hitInfo.distance)
				outRayCastHit = hit;
		}
		return outRayCastHit.hitInfo.hit;
	}
}