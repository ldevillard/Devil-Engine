#pragma once

#include <vector>

#include "Entity.h"
#include "component/Model.h"
#include "data/template/Singleton.h"
#include "utils/serializer/json/json.hpp"

#define MAX_LIGHTS 8

class Light;

class EntityManager : public Singleton<EntityManager>
{
public:	
	// singleton
	static void Initialize(Shader* shader);

	~EntityManager();
	
	Entity* CreateEntity(const std::string& name);
	void DestroyEntity(Entity* entity);
	Entity* DuplicateEntity(Entity* entity);

	void UpdateEntities(float deltaTime) const;

	void ComputeEntities() const;
	bool ComputeSelectedEntity() const;
	void DrawAllMeshes(Shader* shader) const;
	const unsigned int GetNumberOfTriangles() const;

	unsigned int GetLightIndex(Transform* transform) const;
	void UpdateLightsIndex();

	// getters
	const std::vector<Entity*>& GetEntities() const;
	const Entity* GetEntityFromName(const std::string& name) const;
	const std::vector<Model*> GetModels() const;
	const Light* GetMainLight() const;
	const std::string GenerateNewEntityName(const std::string& prefix) const;

	// loading
	bool IsLoadingEntities() const;
	float GetLoadingProgress() const;
	
	// serialization
	nlohmann::ordered_json Serialize() const;
	void Deserialize(const nlohmann::ordered_json& json);

protected:
	void initialize() override;

private:
	void registerEntity(Entity* e);
	std::vector<Entity*>::iterator unregisterEntity(Entity* e);
	void buildEntitiesAsync();

	Shader* shader = nullptr;

	std::vector<Entity*> entities = {};

	int lightsCount = 0;

	// entities loading
	std::atomic<bool> isLoading;
	std::atomic<int> entitiesLoaded;
	int entitiesToLoad = 0;
};