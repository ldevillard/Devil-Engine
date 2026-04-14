#include "component/Component.h"

#include "component/physics/EditorCollider.h"
#include "component/Transform.h"
#include "render/Shader.h"
#include "system/entity/Entity.h"

#pragma region Public Methods

void Component::SetEditorCollider(EditorCollider* cl)
{
	editorCollider = cl;
}

void Component::SetTransform(Transform*tr)
{
	transform = tr;
}

void Component::SetEntity(Entity* en)
{
	entity = en;
}

void Component::SetShader(Shader* sh)
{
	shader = sh;
}

void Component::Update(float deltaTime)
{
}

void Component::FixedUpdate(float fixedDeltaTime)
{
}

#pragma endregion