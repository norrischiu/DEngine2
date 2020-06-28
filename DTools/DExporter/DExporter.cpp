#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>
#include <assimp/Exceptional.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#include "picojson.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <filesystem>
#include <unordered_map>

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
	static Texture Null()
	{
		return Texture{};
	};
	std::vector<uint8_t> data = {};
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t numComponent = 0;
	uint32_t numMip = 0;
	std::string name = {};
};

struct Material
{
	std::string name = {};
	// param
	float3 albedo = {};
	float metallic = 0.0f;
	float roughness = 0.0f;
	// texture
	std::vector<Texture> textures = {};
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

bool ProcessTexture(std::vector<Texture>& textures, const char* path, const char*& error, bool generateMip = false, uint32_t pitch = 1)
{
	Texture tex;
	int x, y, n;
	void* raw = nullptr;
	bool isHdr = stbi_is_hdr(path);
	bool is16Bit = stbi_is_16_bit(path);
	if (is16Bit)
	{
		raw = reinterpret_cast<uint8_t*>(stbi_load_16(path, &x, &y, &n, 4));
	}
	else
	{
		if (isHdr)
		{
			raw = stbi_loadf(path, &x, &y, &n, 4);
			pitch = 4;
		}
		else
		{
			raw = stbi_load(path, &x, &y, &n, 4);
		}
	}
	if (!raw)
	{
		error = stbi_failure_reason();
		return false;
	}
	if (generateMip)
	{
		if (x != y)
		{
			error = "Mipmap generation only support on square texture for now\n";
			return false;
		}

		std::size_t baseMipSize = x * y * pitch * 4;
		tex.data.resize(baseMipSize * 4 / 3);
		memcpy(tex.data.data(), raw, baseMipSize);

		uint8_t* src = tex.data.data();
		uint8_t* dst = tex.data.data() + baseMipSize;

		uint32_t numMip = static_cast<uint32_t>(log2f(static_cast<float>(x))) + 1;
		for (uint32_t i = 1; i < numMip; ++i)
		{
			uint32_t lastWidth = x >> (i - 1);
			uint32_t lastHeight = y >> (i - 1);
			std::size_t lastSize = lastWidth * lastHeight * pitch * 4;
			uint32_t width = x >> i;
			uint32_t height = y >> i;
			std::size_t size = width * height * pitch * 4;

			stbir_resize_uint8(src, lastWidth, lastHeight, 0, dst, width, height, 0, 4);

			src += lastSize;
			dst += size;
		}
		tex.numMip = numMip;
	}
	else
	{
		tex.numMip = 1;
		tex.data.resize(x * y * pitch * 4);
		memcpy(tex.data.data(), raw, tex.data.size());
	}

	tex.width = x;
	tex.height = y;
	tex.numComponent = 4;

	char filename[256];
	_splitpath_s(path, NULL, 0, NULL, 0, filename, 256, NULL, 0);
	tex.name = filename;

	textures.push_back(tex);
	return true;
}

void ProcessMaterial(const aiMaterial* material, std::vector<Material>& materials)
{
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
	const char* error = {};
	struct { aiTextureType type; uint32_t index; } aiTextures[] =
	{
		{ aiTextureType_DIFFUSE, 1 }, // albedo, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE aiTextureType_DIFFUSE, 1
		{ aiTextureType_NORMALS, 0 }, // normal
		{ aiTextureType_UNKNOWN, 0 }, // metallic, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE aiTextureType_UNKNOWN, 0
		{ aiTextureType_UNKNOWN, 0 }, // roughness, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE aiTextureType_UNKNOWN, 0
		{ aiTextureType_LIGHTMAP, 0 }, // ao
	};
	for (const auto& pair : aiTextures)
	{
		aiString path;

		material->GetTexture(pair.type, pair.index, &path);
		if (path.length != 0)
		{
			sprintf_s(tmp, "%s%s", s_inputDirectory, path.C_Str());
			ProcessTexture(outMat.textures, tmp, error, true);
			outMat.numTextures++;
		}
		else
		{
			outMat.textures.push_back(Texture::Null());
		}
	}

	// find duplicated name
	size_t count = std::count_if(materials.begin(), materials.end(), [&](const auto& m) {return m.name == outMat.name; });
	if (count > 0)
	{
		outMat.name += std::to_string(count);
	}

	materials.push_back(std::move(outMat));
}

void ProcessMesh(const aiMesh* mesh, const aiScene *scene, std::vector<Mesh>& outMeshes, std::vector<Material>& outMaterials)
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
		ProcessMaterial(scene->mMaterials[mesh->mMaterialIndex], outMaterials);
		outMesh.materialName = outMaterials.back().name;
	}

	outMeshes.push_back(outMesh);
}

void ProcessNode(aiNode *node, const aiScene *scene, std::vector<Mesh>& meshes, std::vector<Material>& materials)
{
	// process all the node's meshes (if any)
	for (uint32_t i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(mesh, scene, meshes, materials);
	}
	// then do the same for each of its children
	for (uint32_t i = 0; i < node->mNumChildren; ++i)
	{
		ProcessNode(node->mChildren[i], scene, meshes, materials);
	}
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
			fout.write(reinterpret_cast<const char*>(&tex.numMip), sizeof(tex.numMip));
			std::size_t size = tex.data.size();
			fout.write(reinterpret_cast<const char*>(&size), sizeof(size));
			fout.write(reinterpret_cast<const char*>(tex.data.data()), tex.data.size());
			fout.close();
		}
	}
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
		for (const auto& tex : material.textures)
		{
			if (!tex.name.empty())
			{
				sprintf_s(fileOut, "..\\Textures\\%s.tex", tex.name.c_str());
				fout << fileOut << "\n";
			}
			else
			{
				fout << "null" << "\n";
			}
		}
		fout.close();

		Export(material.textures, path);
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

int main(int argc, char *argv[])
{
	std::unordered_map<std::string, std::string> args;
	for (int32_t i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		if (arg[0] != '-')
		{
			printf("Command ill-formed\n");
			return 0;
		}
		args[argv[i]] = argv[i + 1];
		i++;
	}

	const std::string& mode = args["-mode"];
	const std::string& inputPath = args["-input"];
	_splitpath_s(inputPath.c_str(), NULL, 0, s_inputDirectory, 256, NULL, 0, NULL, 0);
	const std::string& outputPath = args["-outputDir"] + "\\" + args["-scene"];
	const std::string& sceneName = args["-scene"];

	if (mode == "scene")
	{
		std::fstream fin;
		fin.open(inputPath, std::fstream::in);

		if (fin.fail())
		{
			std::cerr << "Cannot open file: " << inputPath << '\n';
			return 0;
		}

		picojson::value json;
		std::string err;
		picojson::parse(json, std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>(), &err);
		if (!err.empty() || !json.is<picojson::object>())
		{
			std::cerr << err << std::endl;
			return 0;
		}

		std::vector<Mesh> meshes;
		std::vector<Material> materials;
		std::vector<Texture> textures;

		auto obj = json.get<picojson::object>();
		const std::string& outputPath = obj["outputDir"].get<std::string>() + "\\" + obj["name"].get<std::string>();
		const std::string& sceneName = obj["name"].get<std::string>();

		for (const auto& model : obj["models"].get<std::vector<picojson::value>>())
		{
			auto obj = model.get<picojson::object>();
			auto path = obj["path"].get<std::string>();
			_splitpath_s(path.c_str(), NULL, 0, s_inputDirectory, 256, NULL, 0, NULL, 0);

			Assimp::Importer importer;
			const aiScene *scene = nullptr;
			try
			{
				scene = importer.ReadFile(path, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded | aiProcess_PreTransformVertices);
			}
			catch (DeadlyImportError e)
			{
				std::cerr << "Assimp import error: " << e.what() << '\n';
				return 0;
			}

			ProcessNode(scene->mRootNode, scene, meshes, materials);
		}

		for (const auto& texture : obj["textures"].get<std::vector<picojson::value>>())
		{
			auto obj = texture.get<picojson::object>();

			uint32_t pitch = 1;
			if (!obj["pitch"].is<picojson::null>()) {
				pitch = static_cast<uint32_t>(obj["pitch"].get<double>());
			}
			bool genMip = false;
			if (!obj["genMip"].is<picojson::null>()) {
				genMip = obj["genMip"].get<bool>();
			}

			const char* error = {};
			if (!ProcessTexture(textures, obj["path"].get<std::string>().c_str(), error, genMip, pitch)) {
				std::cerr << "Parse textures failed because: " << error << '\n';
				return 0;
			}
		}

		Export(meshes, materials, outputPath.c_str(), sceneName.c_str());
		Export(textures, outputPath.c_str());
	}
	else if (mode == "model")
	{
		Assimp::Importer importer;
		const aiScene *scene = nullptr;
		try
		{
			scene = importer.ReadFile(inputPath, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded | aiProcess_PreTransformVertices);
		}
		catch (DeadlyImportError e)
		{
			std::cerr << "Assimp import error: " << e.what() << '\n';
			return 0;
		}

		std::vector<Mesh> meshes;
		std::vector<Material> materials;

		ProcessNode(scene->mRootNode, scene, meshes, materials);

		Export(meshes, materials, outputPath.c_str(), sceneName.c_str());
	}
	else if (mode == "texture")
	{
		std::vector<Texture> textures;
		uint32_t pitch = 1;
		if (args.find("-pitch") != args.end()) {
			pitch = std::stoi(args["-pitch"]);
		}
		bool genMip = false;
		if (args.find("-genMip") != args.end()) {
			genMip = std::stoi(args["-genMip"]);
		}

		const char* error = {};
		if (!ProcessTexture(textures, inputPath.c_str(), error, genMip, pitch)) {
			std::cerr << "Parse textures failed because: " << error << '\n';
			return 0;
		}
		Export(textures, outputPath.c_str());
	}
	else
	{
		std::cerr << "Mode is not defined or supported" << '\n';
		return 0;
	}

	return 0;
}