#include "component/Model.h"

#include <maths/glm/glm.hpp>
#include <maths/glm/gtc/matrix_transform.hpp>

#include "component/physics/EditorCollider.h"
#include "system/editor/Outliner.h"
#include "system/editor/Gizmo.h"

std::map<PrimitiveType, std::unique_ptr<Model>> Model::PrimitivesModels;

#pragma region Static Methods

void Model::LoadPrimitives()
{
	PrimitivesModels[CubePrimitive] = std::make_unique<Model>("resources/models/primitive/cube.obj", Material::Default);
	PrimitivesModels[SpherePrimitive] = std::make_unique<Model>("resources/models/primitive/sphere.obj", Material::Default);
    PrimitivesModels[PlanePrimitive] = std::make_unique<Model>("resources/models/primitive/plane.obj", Material::Default);
    PrimitivesModels[CylinderPrimitive] = std::make_unique<Model>("resources/models/primitive/cylinder.obj", Material::Default);
    PrimitivesModels[TorusPrimitive] = std::make_unique<Model>("resources/models/primitive/torus.obj", Material::Default);
}

#pragma endregion

#pragma region Public Methods

Model::Model(std::string path, Material mat) : Component(),
    material(mat), modelPath(path)
{
	loadModel(path);
}

Model::Model(PrimitiveType type, Material mat) : Component(),
    material(mat),
    ModelType(type)
{
    loadPrimitiveModel(type);
}

Model::Model(const Mesh& mesh, Material mat)
    : Component(), material(mat)
{
    meshes.push_back(mesh);
}


int Model::GetNumberOfTriangles() const
{
   int sum = 0;

   for (const Mesh& mesh : meshes)
   {
      sum += mesh.GetNumberOfTriangles();
   }

   return sum;
}

std::vector<Triangle> Model::GetTriangles() const
{
    std::vector<Triangle> triangles = {};
    for (Mesh m : meshes)
    {
        std::vector<Triangle> mTriangles = m.GetTriangles();
        triangles.insert(triangles.end(), mTriangles.begin(), mTriangles.end());
    }
    return triangles;
}

Material& Model::GetMaterial()
{
	return material;
}

const std::vector<Texture>& Model::GetTextures() const
{
    return texturesLoaded;
}

const BoundingBox& Model::GetBoundingBox() const
{
   return editorCollider->GetBoundingBox();
}

const BVH& Model::GetBVH() const
{
    return editorCollider->GetBVH();
}

const std::vector<Mesh>& Model::GetMeshes() const
{
    return meshes;
}

void Model::Compute()
{
    shader->Use();

	// disable instancing for regular model rendering
    shader->SetBool("useInstancing", false);
    shader->SetBool("useInstanceColor", false);

    // binding material data
    shader->SetVec3("material.ambient", material.Ambient);
    shader->SetVec3("material.diffuse", material.Diffuse);
    shader->SetVec3("material.specular", material.Specular);
    shader->SetFloat("material.shininess", material.Shininess);

    // check if the model has textures
    shader->SetBool("textured", texturesLoaded.size() > 0);

    draw();
}

void Model::Update(float deltaTime)
{
    // do nothing for now
}

Component* Model::Clone()
{
	Model* model = new Model();

    for (Mesh mesh : meshes)
		model->meshes.push_back(Mesh(mesh));

    for (Texture texture : texturesLoaded)
        model->texturesLoaded.push_back(Texture(texture));

    model->material = material;
    model->directory = directory;
    model->ModelType = ModelType;
	model->modelPath = modelPath;

	return model;
}

void Model::ComputeOutline(Shader* outlineShader)
{
	outlineShader->Use();
    outlineShader->SetFloat("outlining", Outliner::OutlineWidth);
    outlineShader->SetVec3("color", Outliner::OutlineColor.Value);
	draw();
}

void Model::SetMaterialFromName(std::string name)
{
    if (material.Name == name)
        return;

	const Material& mat = Material::GetMaterialFromName(name);

    material.Ambient = mat.Ambient;
    material.Diffuse = mat.Diffuse;
    material.Specular = mat.Specular;
    material.Shininess = mat.Shininess;
    material.Name = mat.Name;
}

void Model::SetEditorCollider(EditorCollider* cl)
{
    Component::SetEditorCollider(cl);
    editorCollider->UpdateBoundingBox(meshes);
}

nlohmann::ordered_json Model::Serialize() const
{
    nlohmann::ordered_json json;

	json["type"] = "Model";
	json["material"] = material.Serialize();
	json["directory"] = directory;
	json["modelType"] = ModelType;
	json["modelPath"] = modelPath;

	return json;
}

void Model::Deserialize(const nlohmann::ordered_json& json)
{
	material.Deserialize(json["material"]);
	directory = json["directory"];
	ModelType = json["modelType"];
	modelPath = json["modelPath"];

    if (ModelType == PrimitiveType::None)
        loadModel(modelPath);
	else
		loadPrimitiveModel(ModelType);
}

void Model::BuildBVH() const
{
    editorCollider->BuildBVH(meshes);
}

#pragma endregion

#pragma region Private Methods

void Model::draw()
{
    for (unsigned int i = 0; i < meshes.size(); i++)
        meshes[i].Draw(shader);
}

void Model::loadModel(std::string path)
{
    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return;
    }
    directory = path.substr(0, path.find_last_of('/'));

    processNode(scene->mRootNode, scene);
}

void Model::loadPrimitiveModel(PrimitiveType type)
{
    if (PrimitivesModels[type] == nullptr)
    {
        std::cerr << "Primitive type not found" << std::endl;
        return;
    }

    meshes.push_back(Mesh(PrimitivesModels[type]->meshes[0]));
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }
    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    // data to fill
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    // walk through each of the mesh's vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
        // positions
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;
        // normals
        if (mesh->HasNormals())
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        }
        // texture coordinates
        if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            glm::vec2 vec;
            // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
            // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.UV = vec;
        }
        else
            vertex.UV = glm::vec2(0.0f, 0.0f);

        vertices.push_back(vertex);
    }
    // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        // retrieve all indices of the face and store them in the indices vector
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    // process materials
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // 1. diffuse maps
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. specular maps
        std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. normal maps
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 4. height maps
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    }
    // return a mesh object created from the extracted mesh data
    return Mesh(vertices, indices, textures);
}

// checks all material textures of a given type and loads the textures if they're not loaded yet.
// the required info is returned as a Texture struct.
std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
{
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
        bool skip = false;
        for (unsigned int j = 0; j < texturesLoaded.size(); j++)
        {
            if (std::strcmp(texturesLoaded[j].Path.data(), str.C_Str()) == 0)
            {
                textures.push_back(texturesLoaded[j]);
                skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                break;
            }
        }
        if (!skip)
        {   // if texture hasn't been loaded already, load it
            std::string filename = directory + '/' + str.C_Str();
            Texture texture(filename.c_str(), typeName, TextureParam {false, TextureFormat::RGB});
            texture.Path = str.C_Str();
            textures.push_back(texture);
            texturesLoaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
        }
    }
    return textures;
}

#pragma endregion
