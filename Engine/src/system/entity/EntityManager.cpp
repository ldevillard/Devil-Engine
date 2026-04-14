#include <algorithm>
#include <iostream>
#include <regex>
#include <thread>

#include "system/entity/EntityManager.h"
#include "system/editor/Editor.h"
#include "component/Model.h"
#include "component/Light.h"
#include "component/Transform.h"
#include "utils/serializer/json/json.hpp"

#pragma region Singleton Methods

// singleton override
void EntityManager::initialize()
{
	Singleton<EntityManager>::initialize();
}

#pragma endregion

#pragma region Public Methods

EntityManager::~EntityManager()
{
	std::for_each(entities.begin(), entities.end(),
		[this](Entity* e) { unregisterEntity(e); });
	entities.clear();
}

void EntityManager::Initialize(Shader* shader)
{
	Get();
	instance->shader = shader;
	instance->initialize();
}

Entity* EntityManager::CreateEntity(const std::string& name)
{
	Entity* entity = new Entity(name, shader);
	registerEntity(entity);
	return entity;
}

void EntityManager::DestroyEntity(Entity* entity)
{
	auto it = unregisterEntity(entity);
	if (it != entities.end())
		entities.erase(it);

	// refresh lights index for shader binding
	UpdateLightsIndex();
}

Entity* EntityManager::DuplicateEntity(Entity* entity)
{
	Entity* newEntity = new Entity(*entity);
	newEntity->Name = GenerateNewEntityName(newEntity->Name);
	registerEntity(newEntity);
	return newEntity;
}

void EntityManager::UpdateEntities(float deltaTime) const
{
	for (Entity* e : entities)
	{
		e->Update(deltaTime);
	}
}	

void EntityManager::FixedUpdateEntities(float fixedDeltaTime) const
{
	for (Entity* e : entities)
	{
		e->FixedUpdate(fixedDeltaTime);
	}
}

void EntityManager::ComputeEntities() const
{
	shader->Use();
	shader->SetInt("lightsCount", lightsCount);

	for (Entity* e : entities)
	{
		e->Compute();
	}
}

bool EntityManager::ComputeSelectedEntity() const
{
	if (!Editor::Get().GetSettings().Wireframe)
	{
		const Entity* selectedEntity = Editor::Get().GetSelectedEntity();
		if (selectedEntity != nullptr)
		{
			return selectedEntity->ComputeOutline();
		}
	}
	return false;
}

void EntityManager::DrawAllMeshes(Shader* shader) const
{
	std::vector<Model*> models = GetModels();

	for (const Model* model : models)
	{
		model->transform->Compute(shader);
		const std::vector<Mesh>& meshes = model->GetMeshes();
		for (const Mesh& mesh : meshes)
		{
			mesh.Draw(shader);
		}
	}
}

const unsigned int EntityManager::GetNumberOfTriangles() const
{
	unsigned int sum = 0;

	for (const Entity* e : entities)
	{
		Model* model = nullptr;
		if (e->TryGetComponent<Model>(model))
		{
			sum += model->GetNumberOfTriangles();
		}
	}

	return sum;
}

unsigned int EntityManager::GetLightIndex(Transform* transform) const
{
	unsigned int index = 0;

	for (Entity* e : entities)
	{
		Light* light = nullptr;
		if (e->TryGetComponent<Light>(light))
		{
			if (e->transform == transform)
				return index;
		}
		index++;
		if (index >= MAX_LIGHTS)
		{
			std::cerr << "Too many lights in the scene!" << std::endl;
		}
	}

	std::cerr << "Couldn't find the light index!" << std::endl;

	return 0;
}

void EntityManager::UpdateLightsIndex()
{
	unsigned int index = 0;

	for (Entity* e : entities)
	{
		Light* light = nullptr;
		if (e->TryGetComponent<Light>(light))
		{
			light->SetIndex(index);
			index++;
			if (index >= MAX_LIGHTS)
			{
				std::cerr << "Too many lights in the scene!" << std::endl;
			}
		}
	}

	lightsCount = index;
}

bool EntityManager::IsLoadingEntities() const
{
	return isLoading;
}

float EntityManager::GetLoadingProgress() const
{
	return (float)entitiesLoaded / (float)entitiesToLoad;
}

const std::vector<Entity*>& EntityManager::GetEntities() const
{
	return entities;
}

const Entity* EntityManager::GetEntityFromName(const std::string& name) const
{
	for (const Entity* entity : entities)
	{
		if (entity->Name == name)
			return entity;
	}
	return nullptr;
}

const std::vector<Model*> EntityManager::GetModels() const
{
	std::vector<Model*> models = {};

	for (Entity* e : entities)
	{
		Model* model = nullptr;
		if (e->TryGetComponent<Model>(model))
		{
			models.push_back(model);
		}
	}

	return models;
}

const Light* EntityManager::GetMainLight() const
{
	Light* mainLight = nullptr;
	for (Entity* e : entities)
	{
		Light* light = nullptr;
		if (e->TryGetComponent<Light>(light))
		{
			if (light->lightType == Light::LightType::Directional)
			{
				mainLight = light;
				break;
			}
		}
	}
	return mainLight;
}

const std::string EntityManager::GenerateNewEntityName(const std::string& prefix) const
{
	std::string name(prefix);

	int count = 0;

	// regex pattern to match "(number)" at the end of the string
	std::regex pattern("\\(\\d+\\)$");

	for (Entity* e : entities)
	{
		if (e->Name == name)
		{
			// check if the name ends with a number in parentheses
			if (std::regex_search(name, pattern))
			{
				// if it does, extract the number and increment it
				std::smatch match;
				std::regex_search(name, match, pattern);
				count = std::stoi(match[0].str().substr(1, match[0].str().size() - 2));
				// remove the existing number
				name = std::regex_replace(name, pattern, "");
			}

			// increment the count and append it to the name
			name += "(" + std::to_string(++count) + ")";
		}
	}

	return name;
}

nlohmann::ordered_json EntityManager::Serialize() const
{
	nlohmann::ordered_json json;

	nlohmann::ordered_json entitiesJson = nlohmann::ordered_json::array();
	for (const Entity* e : entities) 
	{
		entitiesJson.push_back(e->Serialize());
	}
	json["Entities"] = entitiesJson;

	return json;
}

void EntityManager::Deserialize(const nlohmann::ordered_json& json)
{
	std::for_each(entities.begin(), entities.end(),
		[this](Entity* e) { unregisterEntity(e); });
	entities.clear();

	for (const nlohmann::ordered_json& entityJson : json["Entities"])
	{
		Entity* entity = CreateEntity(entityJson["Name"]);
		entity->Deserialize(entityJson);
	}

	buildEntitiesAsync();
}

#pragma endregion

#pragma region Private Methods

void EntityManager::registerEntity(Entity* e)
{
	if (e == nullptr)
	{
		std::cerr << "Try to register a null Entity" << std::endl;
		return;
	}

	auto it = std::find(entities.begin(), entities.end(), e);

	if (it == entities.end())
		entities.push_back(e);
	else
		std::cerr << "Entity is already registered!" << std::endl;

	// refresh lights index for shader binding
	UpdateLightsIndex();
}

std::vector<Entity*>::iterator EntityManager::unregisterEntity(Entity* e)
{
	assert(e != nullptr && "Try to unregister a null Entity");

	auto it = std::find(entities.begin(), entities.end(), e);

	if (it != entities.end())
	{
		delete* it;
	}
	else
	{
		assert(false && "Couldn't not find Entity to unregister!");
	}
	return it;
}

void EntityManager::buildEntitiesAsync()
{
	isLoading = true;
	entitiesLoaded = 0;
	
	std::thread([this]()
	{
		entitiesToLoad = static_cast<int>(entities.size());

		std::vector<std::thread> threads;
		threads.reserve(entitiesToLoad);
		
		for (Entity* entity : entities)
		{
			threads.emplace_back([this, entity]()
			{
				entity->BuildBVH();
				// thread safe increment of loadingProgress
				++entitiesLoaded;
			});
		}

		// wait for all entities to be loaded
		for (std::thread& thread : threads)
		{
			if (thread.joinable())
			{
				thread.join();
			}
		}

		isLoading = false;
	}).detach();
}

#pragma endregion
