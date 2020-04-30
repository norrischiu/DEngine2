#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>
#include <assimp/Exceptional.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <filesystem>

static char s_inputDirectory[256];

struct float2
{
	float x, y;
};

struct float3
{
	float x, y, z;
};

struct uint2
{
	uint32_t x, y;
};

struct uint3
{
	uint32_t x, y, z;
};

struct uintfloat
{
	uint32_t x;
	float y;
};

enum TextureType
{
	Albedo = 0,
	Normal,
	Metallic,
	Roughness,
	AO,
	Count
};

struct Texture
{
	uint8_t* data;
	uint32_t width;
	uint32_t height;
	uint32_t numComponent;
	std::size_t size;
	std::string name;
};

struct Material
{
	std::string name;
	// param
	float3 albedo;
	float metallic;
	float roughness;
	// texture
	Texture textures[TextureType::Count];
	uint32_t numTextures = 0;
};

struct Mesh
{
	std::string name;
	std::vector<float3> vertices;
	std::vector<float3> normals;
	std::vector<float3> tangents;
	std::vector<float2> uvs;
	std::vector<uintfloat> weights;
	std::vector<uint3> indices;

	std::string materialName;
};

void ProcessMesh(const aiMesh* mesh, const aiScene *scene, std::vector<Mesh>& outMeshes)
{
	Mesh outMesh;
	outMesh.name = mesh->mName.C_Str();
	std::replace(outMesh.name.begin(), outMesh.name.end(), ':', '_');

	for (uint32_t i = 0; i < mesh->mNumVertices; i++)
	{
		float3 vertex;
		vertex.x = mesh->mVertices[i].x;
		vertex.y = mesh->mVertices[i].y;
		vertex.z = mesh->mVertices[i].z;
		outMesh.vertices.push_back(vertex);

		float3 normal;
		normal.x = mesh->mNormals[i].x;
		normal.y = mesh->mNormals[i].y;
		normal.z = mesh->mNormals[i].z;
		outMesh.normals.push_back(normal);

		if (mesh->mTangents)
		{
			float3 tangent;
			tangent.x = mesh->mTangents[i].x;
			tangent.y = mesh->mTangents[i].y;
			tangent.z = mesh->mTangents[i].z;
			outMesh.tangents.push_back(tangent);
		}

		if (mesh->mTextureCoords[0])
		{
			float2 uv;
			uv.x = mesh->mTextureCoords[0][i].x;
			uv.y = mesh->mTextureCoords[0][i].y;
			outMesh.uvs.push_back(uv);
		}

		if (mesh->mBones)
		{
			uintfloat weight;
			weight.x = mesh->mBones[0]->mWeights->mVertexId;
			weight.y = mesh->mBones[0]->mWeights->mWeight;
			outMesh.weights.push_back(weight);
		}
	}

	for (uint32_t i = 0; i < mesh->mNumFaces; i++)
	{
		uint3 index;
		index.x = mesh->mFaces[i].mIndices[0];
		index.y = mesh->mFaces[i].mIndices[1];
		index.z = mesh->mFaces[i].mIndices[2];
		outMesh.indices.push_back(index);
	}

	{
		aiString name;
		scene->mMaterials[mesh->mMaterialIndex]->Get(AI_MATKEY_NAME, name);
		outMesh.materialName = name.C_Str();
	}

	outMeshes.push_back(outMesh);
}

void ProcessNode(aiNode *node, const aiScene *scene, std::vector<Mesh>& meshes)
{
	// process all the node's meshes (if any)
	for (uint32_t i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(mesh, scene, meshes);
	}
	// then do the same for each of its children
	for (uint32_t i = 0; i < node->mNumChildren; ++i)
	{
		ProcessNode(node->mChildren[i], scene, meshes);
	}
}
void ProcessMaterial(const aiScene *scene, std::vector<Material>& materials)
{
	materials.reserve(scene->mNumMaterials);
	for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
	{
		const aiMaterial* material = scene->mMaterials[i];
		Material outMat;

		aiString name;
		material->Get(AI_MATKEY_NAME, name);
		outMat.name = name.C_Str();

		// PARAMETER
		aiColor3D albedo;
		material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, albedo);
		memcpy(&outMat.albedo.x, &albedo, sizeof(albedo));
		material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, outMat.roughness);
		material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, outMat.metallic);

		// TEXTURE
		char tmp[256];
		struct { aiTextureType type; uint32_t index; } aiTextures[] =
		{
			{ aiTextureType_DIFFUSE, 1 }, // albedo, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE aiTextureType_DIFFUSE, 1
			{ aiTextureType_NORMALS, 0 }, // normal
			{ aiTextureType_UNKNOWN, 0 }, // metallic, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE aiTextureType_UNKNOWN, 0
			{ aiTextureType_UNKNOWN, 0 }, // roughness, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE aiTextureType_UNKNOWN, 0
			{ aiTextureType_LIGHTMAP, 0 }, // ao
		};
		for (uint32_t i = 0; i < TextureType::Count; ++i)
		{
			auto& tex = outMat.textures[i];
			aiString path;

			material->GetTexture(aiTextures[i].type, aiTextures[i].index, &path);
			if (path.length != 0)
			{
				int x, y, n;
				sprintf_s(tmp, "%s%s", s_inputDirectory, path.C_Str());
				tex.data = stbi_load(tmp, &x, &y, &n, 4);
				assert(tex.data);
				tex.width = x;
				tex.height = y;
				tex.numComponent = 4;
				tex.size = x * y * 4;

				char filename[256];
				_splitpath_s(tmp, NULL, 0, NULL, 0, filename, 256, NULL, 0);
				tex.name = filename;

				outMat.numTextures++;
			}
		}

		materials.push_back(std::move(outMat));
	}
}

bool ProcessTexture(std::vector<Texture>& textures, const char* path, const char*& error, uint32_t pitch = 1)
{
	Texture tex;
	int x, y, n;
	if (pitch == 1) {
		tex.data = stbi_load(path, &x, &y, &n, 4);
	}
	else if (pitch == 2) {
		tex.data = reinterpret_cast<uint8_t*>(stbi_load_16(path, &x, &y, &n, 4));
	}
	if (!tex.data) {
		error = stbi_failure_reason();
		return false;
	}
	tex.width = x;
	tex.height = y;
	tex.numComponent = 4;
	tex.size = x * y * pitch * 4;

	char filename[256];
	_splitpath_s(path, NULL, 0, NULL, 0, filename, 256, NULL, 0);
	tex.name = filename;

	textures.push_back(tex);
	return true;
}

void Export(const std::vector<Material>& materials, const char* path)
{
	std::fstream fout;
	uint32_t size = static_cast<uint32_t>(materials.size());
	char fileOut[256];
	sprintf_s(fileOut, "%s\\Materials\\", path);
	std::filesystem::create_directories(fileOut);
	sprintf_s(fileOut, "%s\\Textures\\", path);
	std::filesystem::create_directories(fileOut);

	for (uint32_t i = 0; i < size; ++i)
	{
		const Material& material = materials[i];

		sprintf_s(fileOut, "%s\\Materials\\%s.mate", path, material.name.c_str());
		fout.open(fileOut, std::fstream::out);
		fout << material.albedo.x << " " << material.albedo.y << " " << material.albedo.z << "\n";
		fout << material.metallic << "\n";
		fout << material.roughness << "\n";
		fout << material.numTextures << "\n";
		for (uint32_t j = 0; j < TextureType::Count; ++j)
		{
			if (!material.textures[j].name.empty())
			{
				sprintf_s(fileOut, "..\\Textures\\%s.tex", material.textures[j].name.c_str());
				fout << fileOut << "\n";
			}
		}
		fout.close();

		for (uint32_t j = 0; j < TextureType::Count; ++j)
		{
			const auto& tex = material.textures[j];
			if (!tex.name.empty())
			{
				sprintf_s(fileOut, "%s\\Textures\\%s.tex", path, tex.name.c_str());

				fout.open(fileOut, std::fstream::out | std::fstream::binary);
				fout.write(reinterpret_cast<const char*>(&tex.width), sizeof(tex.width));
				fout.write(reinterpret_cast<const char*>(&tex.height), sizeof(tex.height));
				fout.write(reinterpret_cast<const char*>(&tex.numComponent), sizeof(tex.numComponent));
				fout.write(reinterpret_cast<const char*>(&tex.size), sizeof(tex.size));
				fout.write(reinterpret_cast<char*>(tex.data), tex.size);
				fout.close();
			}
		}
	}

}

void Export(const std::vector<Mesh>& meshes, const char* path)
{
	uint32_t size = static_cast<uint32_t>(meshes.size());
	std::fstream fout;
	char fileOut[256];
	sprintf_s(fileOut, "%s\\Models\\", path);
	std::filesystem::create_directories(fileOut);

	for (uint32_t i = 0; i < size; ++i)
	{
		const Mesh& mesh = meshes[i];
		uint64_t num = 0;

		// vertex
		sprintf_s(fileOut, "%s\\Models\\%s.vert", path, mesh.name.c_str());
		fout.open(fileOut, std::fstream::out);
		num = mesh.vertices.size();
		fout << num << std::endl;
		for (auto& vertex : mesh.vertices)
		{
			fout << vertex.x << " " << vertex.y << " " << vertex.z << std::endl;
		}
		fout.close();

		// normal
		sprintf_s(fileOut, "%s\\Models\\%s.norm", path, mesh.name.c_str());
		fout.open(fileOut, std::fstream::out);
		num = mesh.normals.size();
		fout << num << std::endl;
		for (auto& normal : mesh.normals)
		{
			fout << normal.x << " " << normal.y << " " << normal.z << std::endl;
		}
		fout.close();

		// tangent
		if (!mesh.tangents.empty())
		{
			sprintf_s(fileOut, "%s\\Models\\%s.tangent", path, mesh.name.c_str());
			fout.open(fileOut, std::fstream::out);
			num = mesh.tangents.size();
			fout << num << std::endl;
			for (auto& tangent : mesh.tangents)
			{
				fout << tangent.x << " " << tangent.y << " " << tangent.z << std::endl;
			}
			fout.close();
		}

		// texCoord
		if (!mesh.uvs.empty())
		{
			sprintf_s(fileOut, "%s\\Models\\%s.texcoord", path, mesh.name.c_str());
			fout.open(fileOut, std::fstream::out);
			num = mesh.uvs.size();
			fout << num << std::endl;
			for (auto& uv : mesh.uvs)
			{
				fout << uv.x << " " << uv.y << std::endl;
			}
			fout.close();
		}

		// index
		sprintf_s(fileOut, "%s\\Models\\%s.index", path, mesh.name.c_str());
		fout.open(fileOut, std::fstream::out);
		num = mesh.indices.size();
		fout << num << std::endl;
		for (auto& index : mesh.indices)
		{
			fout << index.x << " " << index.y << " " << index.z << std::endl;
		}
		fout.close();

		// material
		sprintf_s(fileOut, "%s\\Models\\%s.mate", path, mesh.name.c_str());
		fout.open(fileOut, std::fstream::out);
		fout << mesh.materialName;
		fout.close();
	}
}

void Export(const std::vector<Mesh>& meshes, const std::vector<Material>& materials, const char* path, const char* sceneName)
{
	std::fstream fout;
	std::string meshNames;
	std::string materialNames;
	char fileOut[256];

	uint32_t numMesh = static_cast<uint32_t>(meshes.size());
	for (uint32_t i = 0; i < numMesh; ++i)
	{
		meshNames.append(meshes[i].name + '\n');
	}

	uint32_t numMat = static_cast<uint32_t>(materials.size());
	for (uint32_t i = 0; i < numMat; ++i)
	{
		materialNames.append(materials[i].name + '\n');
	}

	std::filesystem::create_directories(path);

	sprintf_s(fileOut, "%s\\%s.scene", path, sceneName);
	fout.open(fileOut, std::fstream::out);
	fout << numMat << std::endl << materialNames;
	fout << numMesh << std::endl << meshNames;
	fout.close();

	Export(meshes, path);
	Export(materials, path);
}

void Export(const std::vector<Texture>& textures, const char* path)
{
	std::fstream fout;
	char fileOut[256];
	sprintf_s(fileOut, "%s\\Textures\\", path);
	std::filesystem::create_directories(fileOut);

	for (auto& tex : textures) 
	{
		if (!tex.name.empty())
		{
			sprintf_s(fileOut, "%s\\Textures\\%s.tex", path, tex.name.c_str());

			fout.open(fileOut, std::fstream::out | std::fstream::binary);
			fout.write(reinterpret_cast<const char*>(&tex.width), sizeof(tex.width));
			fout.write(reinterpret_cast<const char*>(&tex.height), sizeof(tex.height));
			fout.write(reinterpret_cast<const char*>(&tex.numComponent), sizeof(tex.numComponent));
			fout.write(reinterpret_cast<const char*>(&tex.size), sizeof(tex.size));
			fout.write(reinterpret_cast<char*>(tex.data), tex.size);
			fout.close();
		}
	}
}

int main(int argc, char *argv[])
{
	if (argc < 5) {
		return 0;
	}

	const char* option = argv[1];
	const char* inputPath = argv[2];
	_splitpath_s(inputPath, NULL, 0, s_inputDirectory, 256, NULL, 0, NULL, 0);
	char outputPath[256];
	sprintf_s(outputPath, "%s\\%s", argv[3], argv[4]); // outdir + scene name
	const char* sceneName = argv[4];

	if (strcmp(option, "model") == 0)
	{
		Assimp::Importer importer;
		const aiScene *scene = nullptr;
		try 
		{
			scene = importer.ReadFile(inputPath, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded | aiProcess_PreTransformVertices);
		}
		catch (DeadlyImportError e)
		{
			printf("Assimp import error: %s\n", e.what());
		}

		std::vector<Mesh> meshes;
		std::vector<Material> materials;

		ProcessNode(scene->mRootNode, scene, meshes);
		ProcessMaterial(scene, materials);

		Export(meshes, materials, outputPath, sceneName);
	}
	if (strcmp(option, "texture") == 0)
	{
		std::vector<Texture> textures;
		uint32_t pitch = 1;
		if (argc > 5) {
			pitch = atoi(argv[5]);
		}

		const char* error = {};
		if (!ProcessTexture(textures, inputPath, error, pitch)) {
			std::cerr << "Parse textures failed because: " << error << '\n';
			return 0;
		}
		Export(textures, outputPath);
	}

	return 0;
}