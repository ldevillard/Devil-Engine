#include <stdexcept>
#include <iostream>
#include <maths/glm/gtc/quaternion.hpp>

#include "component/Component.h"
#include "component/physics/EditorCollider.h"
#include "component/Transform.h"
#include "data/Type.h"
#include "system/editor/Editor.h"
#include "system/editor/Outliner.h"
#include "system/entity/Entity.h"
#include "system/entity/EntityManager.h"

#pragma region Public Methods

Entity::Entity(const std::string &name, Shader* sh) :
    shader(sh),
    Name(name)
{
	transform = new Transform();
    editorCollider = new EditorCollider(this);
}

Entity::Entity(const Entity& other) :
    shader(other.shader),
    Name(other.Name)
{
	transform = new Transform(*other.transform);
	editorCollider = new EditorCollider(*other.editorCollider);
    editorCollider->entity = this;

    for (Component* c : other.components)
    {
		Component* newComponent = c->Clone();
		setupComponent(newComponent);
	}
}

Entity::~Entity()
{
    std::for_each(components.begin(), components.end(),
        [this](Component* c) { delete c; });
    components.clear();

	delete transform;
    delete editorCollider;
}

const std::vector<Component*>& Entity::GetComponents() const
{
	return components;
}

const EditorCollider* Entity::GetEditorCollider() const
{
	return editorCollider;
}

bool Entity::IsSelectedEntity() const
{
	return Editor::Get().GetSelectedEntity() == this;
}

void Entity::Compute()
{
    transform->Compute(shader);
    
    for (Component* c : components)
        c->Compute();
    
    editorCollider->Draw(*transform);
}

bool Entity::ComputeOutline() const
{
    Model* model = nullptr;
    if (!TryGetComponent<Model>(model))
    {
        return false;
    }

    // setup stencil buffer and depth buffer for outline
    Outliner::Setup();

    // render model
    Editor::Get().RenderCamera(Outliner::OutlineShader);
    transform->Compute(Outliner::OutlineShader);
    
	model->ComputeOutline(Outliner::OutlineShader);

    Outliner::Draw();

    // reset them
    Outliner::Reset();
    return true;
}

void Entity::Update(float deltaTime)
{
    for (Component* c : components) 
    {
        c->Update(deltaTime);
    }
}

void Entity::FixedUpdate(float fixedDeltaTime)
{
    for (Component* c : components)
    {
        c->FixedUpdate(fixedDeltaTime);
    }
}

nlohmann::ordered_json Entity::Serialize() const
{
    nlohmann::ordered_json json;

    json["Name"] = Name;
    json["Transform"] = transform->Serialize();

    nlohmann::ordered_json componentsJson = nlohmann::ordered_json::array();
    for (const Component* component : components) 
    {
        componentsJson.push_back(component->Serialize());
    }
    json["Components"] = componentsJson;

    return json;
}

void Entity::Deserialize(const nlohmann::ordered_json& json)
{
	transform->Deserialize(json["Transform"]);

	for (const nlohmann::ordered_json& componentJson : json["Components"]) 
	{
        std::string type = componentJson["type"];
		Component* component = createComponentFromName(type);
		component->Deserialize(componentJson);
		setupComponent(component);
	}
}

void Entity::BuildBVH() const
{
    Model* model = nullptr;
    if (TryGetComponent<Model>(model))
    {
        model->BuildBVH();
    }
}

#pragma endregion

#pragma region Private Methods

void Entity::setupComponent(Component* component)
{
    component->SetEditorCollider(editorCollider);
    component->SetTransform(transform);
    component->SetEntity(this);
    component->SetShader(shader);
    components.push_back(component);

    EntityManager::Get().UpdateLightsIndex();
}

Component* Entity::createComponentFromName(const std::string& name)
{
    auto it = Type::componentTypeMap.find(name);
    if (it != Type::componentTypeMap.end())
    {
        Component* component = (it->second)();
        return component;
    }
    else 
    {
        assert(false && "Error: Component type not registered.");
        return nullptr;
    }
}

#pragma endregion
