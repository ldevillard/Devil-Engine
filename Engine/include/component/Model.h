#pragma once

#include <map>

// data
#include "component/Component.h"
#include "data/mesh/Mesh.h"
#include "data/Material.h"
#include "data/mesh/MeshData.h"
#include "data/BoundingBox.h"
#include "data/Triangle.h"

// assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class BVH;
class EditorCollider;

class Model : public Component
{

public:
    Model() = default;
    Model(PrimitiveType type, Material mat = Material::Default);
    Model(std::string path, Material mat = Material::Default);
    Model(const Mesh& mesh, Material mat = Material::Default);

    int GetNumberOfTriangles() const;
    std::vector<Triangle> GetTriangles() const;

    // not const because we modify the material directly in the inspector class, maybe use a setter instead
    Material& GetMaterial();
    const std::vector<Texture>& GetTextures() const;
    const BoundingBox& GetBoundingBox() const;
    const BVH& GetBVH() const;
    const std::vector<Mesh>& GetMeshes() const;

    void Compute() override;
    void ComputeOutline(Shader* outlineShader);

	void Update(float deltaTime) override;
    Component* Clone() override;

    void SetMaterialFromName(std::string name);
    void SetEditorCollider(EditorCollider* cl) override;

    // static models
    static void LoadPrimitives();
    static std::map<PrimitiveType, std::unique_ptr<Model>> PrimitivesModels;

    // serialization
    nlohmann::ordered_json Serialize() const override;
    void Deserialize(const nlohmann::ordered_json& json) override;

    void BuildBVH() const;

    PrimitiveType ModelType = PrimitiveType::None;

private:
    void draw();

    // model data
    std::string modelPath = "";
    std::vector<Mesh> meshes;
    std::vector<Texture> texturesLoaded;
    std::string directory;

    Material material = Material::Default;

    void loadModel(std::string path);
    void loadPrimitiveModel(PrimitiveType type);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};

REGISTER_COMPONENT_TYPE(Model);