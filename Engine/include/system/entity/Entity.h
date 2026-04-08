#pragma once

#include <string>
#include <vector>

#include "utils/serializer/json/json.hpp"

class Component;
class EditorCollider;
class Shader;
class Transform;

class Entity
{
public:
	Entity(const std::string &name, Shader* sh);
	Entity(const Entity& other);
	~Entity();

	template<typename T>
	const T* GetComponent() const;

	template<typename T>
	bool TryGetComponent(T*& outComponent) const;

	template<typename T, typename... Args>
	T* AddComponent(Args&&... args);

	const std::vector<Component*>& GetComponents() const;
	const EditorCollider* GetEditorCollider() const;
	bool IsSelectedEntity() const;


	void Compute();
	// return true if the outline is computed successfully
	bool ComputeOutline() const;

	void Update(float deltaTime);

	// serialization
	nlohmann::ordered_json Serialize() const;
	void Deserialize(const nlohmann::ordered_json& json);

	void BuildBVH() const;

	std::string Name;
	Transform* transform = nullptr;

private:
	void setupComponent(Component* component);
	
	// serialization
	Component* createComponentFromName(const std::string& name);

	EditorCollider* editorCollider = nullptr;
	Shader* shader = nullptr;
	
	std::vector<Component*> components = {};
};

// template implementation
template<typename T>
const T* Entity::GetComponent() const
{
	auto it = std::find_if(components.begin(), components.end(), [](const Component* comp) {
		return dynamic_cast<const T*>(comp) != nullptr;
		});

	if (it != components.end())
		return dynamic_cast<const T*>(*it);
	else
		return nullptr;
}

template<typename T>
bool Entity::TryGetComponent(T*& outComponent) const
{
	for (const auto& comp : components)
	{
		if (auto castedComponent = dynamic_cast<T*>(comp))
		{
			outComponent = castedComponent;
			return true;
		}
	}
	return false;
}

template<typename T, typename... Args>
T* Entity::AddComponent(Args&&... args)
{
	T* newComponent = new T(std::forward<Args>(args)...);
	Component* component = dynamic_cast<Component*>(newComponent);
	if (component == nullptr)
	{
		assert(false && "Can't adding a non component object");
		delete newComponent;
		return nullptr;
	}
	setupComponent(component);
	// build bvh only if model and call directly editorCollider->BuildBVH() method
	BuildBVH();
	return newComponent;
}