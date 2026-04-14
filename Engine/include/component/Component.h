#pragma once

#include "data/Type.h"
#include "utils/serializer/json/json.hpp"

class Entity;
class EditorCollider;
class Shader;
class Transform;

class Component
{
public:
	virtual void SetEditorCollider(EditorCollider* cl);
	void SetTransform(Transform* tr);
	void SetEntity(Entity* en);
	void SetShader(Shader* sh);

	virtual void Compute() = 0;
	virtual Component* Clone() = 0;

	virtual void Update(float deltaTime);
	virtual void FixedUpdate(float fixedDeltaTime);

	// serialization
	virtual nlohmann::ordered_json Serialize() const = 0;
	virtual void Deserialize(const nlohmann::ordered_json& json) = 0;
	
	Transform* transform		   = nullptr;
	Entity*	entity				   = nullptr;

protected:
	EditorCollider* editorCollider = nullptr;
	Shader*	shader				   = nullptr;
};
