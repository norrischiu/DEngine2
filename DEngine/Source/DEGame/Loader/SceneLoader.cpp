#include <DEGame/DEGame.h>
#include <DERendering/DERendering.h>
#include <DERendering/Device/RenderDevice.h>
#include <DERendering/Device/CopyCommandList.h>
#include <DERendering/DataType/GraphicsDataType.h>
#include <DECore/Container/Vector.h>
#include <DECore/Container/HashMap.h>
#include <DECore/Job/JobScheduler.h>

#include "SceneLoader.h"

#include <fstream>

namespace DE
{

void SceneLoader::Init(RenderDevice *device)
{
	m_pRenderDevice = device;
}

void SceneLoader::SetRootPath(const char *path)
{
	m_sRootPath = path;
}

struct LoadToMaterialsData
{
	char path[256];
	char materialName[64];
	Material *pMaterial;
	RenderDevice *pDevice;
	CopyCommandList *pCopyCommandList;
};

void LoadToMaterials(void *data)
{
	char tmp[256] = {};
	LoadToMaterialsData *pData = reinterpret_cast<LoadToMaterialsData *>(data);
	CopyCommandList &pCommandList = *pData->pCopyCommandList;
	Material &mat = *pData->pMaterial;
	sprintf(tmp, "%s\\%s.mate", pData->path, pData->materialName);
	pCommandList.Init(pData->pDevice->m_Device);

	std::ifstream fin;
	fin.open(tmp, std::fstream::in);
	assert(!fin.fail());

	fin >> mat.albedo.x >> mat.albedo.y >> mat.albedo.z;
	fin >> mat.metallic;
	fin >> mat.roughness;
	mat.ao = 1.0f;

	uint32_t numTexture = 0;
	fin >> numTexture;

	for (uint32_t i = 0; i < numTexture; ++i)
	{
		std::string texturePath;
		fin >> texturePath;
		// load the file content and upload to ID3D12Resource
		{
			std::ifstream fin;
			std::size_t size = 0;
			uint32_t width = 0, height = 0, numComponent = 0;

			sprintf_s(tmp, "%s\\%s", pData->path, texturePath.c_str());
			fin.open(tmp, std::ifstream::in | std::ifstream::binary);

			fin.read(reinterpret_cast<char *>(&width), sizeof(width));
			fin.read(reinterpret_cast<char *>(&height), sizeof(height));
			fin.read(reinterpret_cast<char *>(&numComponent), sizeof(numComponent));
			fin.read(reinterpret_cast<char *>(&size), sizeof(size));
			char *data = new char[size];
			fin.read(data, size);

			DXGI_FORMAT format = {};
			switch (numComponent)
			{
			case 4:
				format = DXGI_FORMAT_R8G8B8A8_UNORM;
				break;
			default:
				assert(false);
			}
			mat.m_Textures[i].Init(pData->pDevice->m_Device, width, height, 1, 1, format);
			uint32_t rowPitch = Align(width * sizeof(char) * numComponent, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
			pCommandList.UploadTexture(reinterpret_cast<uint8_t *>(data), width, height, rowPitch, 1, format, mat.m_Textures[i]);

			delete data;
		}
	}
	fin.close();

	if (numTexture == 5)
	{
		mat.shadingType = ShadingType::Textured;
	}
}

struct LoadToMeshesData
{
	char path[256];
	Mesh *pMesh;
	HashMap<uint32_t> *pMatToID;
	RenderDevice *pDevice;
};

void LoadToMeshes(void *data)
{
	char tmp[256] = {};
	LoadToMeshesData *pData = reinterpret_cast<LoadToMeshesData *>(data);
	std::ifstream fin;
	Mesh &mesh = *pData->pMesh;
	HashMap<uint32_t> &matToID = *pData->pMatToID;
	uint32_t num;

	// vertices
	{
		sprintf(tmp, "%s.vert", pData->path);
		fin.open(tmp, std::fstream::in);
		assert(!fin.fail());

		fin >> num;
		Vector<float3> vertices(num);
		for (uint32_t n = 0; n < num; ++n)
		{
			fin >> vertices[n].x >> vertices[n].y >> vertices[n].z;
		}
		fin.close();

		std::size_t size = num * sizeof(float3);
		mesh.m_Vertices.Init(pData->pDevice->m_Device, sizeof(float3), size);
		mesh.m_Vertices.Update(vertices.data(), vertices.size() * sizeof(float3));
	}

	// normals
	{
		sprintf(tmp, "%s.norm", pData->path);
		fin.open(tmp, std::fstream::in);
		assert(!fin.fail());

		fin >> num;
		Vector<float3> normals(num);
		for (uint32_t n = 0; n < num; ++n)
		{
			fin >> normals[n].x >> normals[n].y >> normals[n].z;
		}
		fin.close();

		std::size_t size = num * sizeof(float3);
		mesh.m_Normals.Init(pData->pDevice->m_Device, sizeof(float3), size);
		mesh.m_Normals.Update(normals.data(), normals.size() * sizeof(float3));
	}

	// tangents
	{
		sprintf(tmp, "%s.tangent", pData->path);
		fin.open(tmp, std::fstream::in);
		if (!fin.fail())
		{
			fin >> num;
			if (num > 0)
			{
				Vector<float3> tangents(num);
				for (uint32_t n = 0; n < num; ++n)
				{
					fin >> tangents[n].x >> tangents[n].y >> tangents[n].z;
				}

				std::size_t size = num * sizeof(float3);
				mesh.m_Tangents.Init(pData->pDevice->m_Device, sizeof(float3), size);
				mesh.m_Tangents.Update(tangents.data(), tangents.size() * sizeof(float3));
			}
			fin.close();
		}
	}

	// texCoords
	{
		sprintf(tmp, "%s.texcoord", pData->path);
		fin.open(tmp, std::fstream::in);
		if (!fin.fail())
		{
			fin >> num;
			if (num > 0)
			{
				Vector<float2> texCoords(num);
				for (uint32_t n = 0; n < num; ++n)
				{
					fin >> texCoords[n].x >> texCoords[n].y;
				}

				std::size_t size = num * sizeof(float2);
				mesh.m_TexCoords.Init(pData->pDevice->m_Device, sizeof(float2), size);
				mesh.m_TexCoords.Update(texCoords.data(), texCoords.size() * sizeof(float2));
			}
			fin.close();
		}
	}

	// index
	{
		sprintf(tmp, "%s.index", pData->path);
		fin.open(tmp, std::fstream::in);
		assert(!fin.fail());

		fin >> num;
		Vector<uint3> indices(num);
		for (uint32_t n = 0; n < num; ++n)
		{
			fin >> indices[n].x >> indices[n].y >> indices[n].z;
		}
		fin.close();

		std::size_t size = num * sizeof(uint3);
		mesh.m_Indices.Init(pData->pDevice->m_Device, sizeof(uint32_t), size);
		mesh.m_Indices.Update(indices.data(), indices.size() * sizeof(uint3));

		mesh.m_iNumIndices = num * 3;
	}

	// material
	sprintf(tmp, "%s.mate", pData->path);
	fin.open(tmp, std::fstream::in);
	assert(!fin.fail());

	std::string materialName;
	fin >> materialName;
	mesh.m_MaterialID = matToID[materialName.c_str()];
	fin.close();
}

void SceneLoader::Load(const char *sceneName, Scene &scene)
{
	char path[256];
	std::fstream fin;
	Vector<uint32_t> meshes;

	sprintf(path, "%s\\%s\\%s.scene", m_sRootPath.c_str(), sceneName, sceneName);
	fin.open(path, std::fstream::in);
	HashMap<uint32_t> materialToID(512);

	// material
	uint32_t numMat = 0;
	fin >> numMat;
	Vector<Job::Desc> matJobDescs;
	matJobDescs.reserve(numMat);
	Vector<CopyCommandList> commandLists(numMat);
	for (uint32_t i = 0; i < numMat; ++i)
	{
		std::string name;
		fin >> name;

		if (!materialToID.Contain(name.c_str()))
		{
			LoadToMaterialsData *data = new LoadToMaterialsData();
			sprintf(data->path, "%s\\%s\\Materials\\", m_sRootPath.c_str(), sceneName);
			strcpy(data->materialName, name.c_str());
			const uint32_t index = Material::Create();
			data->pMaterial = &Material::Get(index);
			data->pDevice = m_pRenderDevice;
			data->pCopyCommandList = &commandLists[i];
			Job::Desc desc(&LoadToMaterials, data, nullptr);
			matJobDescs.push_back(std::move(desc));

			materialToID.Add(name.c_str(), i);
		}
	}
	auto *loadMatCounter = JobScheduler::Instance()->Run(matJobDescs);
	JobScheduler::Instance()->WaitOnMainThread(loadMatCounter);

	// model
	uint32_t numModel = 0;
	fin >> numModel;
	Vector<Job::Desc> jobDescs(numModel);
	meshes.reserve(numModel);
	for (uint32_t i = 0; i < numModel; ++i)
	{
		std::string name;
		fin >> name;

		char fileName[256];
		sprintf(fileName, "%s\\%s\\Models\\%s", m_sRootPath.c_str(), sceneName, name.c_str());

		LoadToMeshesData *data = new LoadToMeshesData();
		strcpy(data->path, fileName);
		const uint32_t index = Mesh::Create();
		data->pMesh = &Mesh::Get(index);
		data->pDevice = m_pRenderDevice;
		data->pMatToID = &materialToID;
		Job::Desc desc(&LoadToMeshes, data, nullptr);
		jobDescs[i] = std::move(desc);

		meshes.push_back(i);
	}
	fin.close();

	auto *loadMeshCounter = JobScheduler::Instance()->Run(jobDescs);
	JobScheduler::Instance()->WaitOnMainThread(loadMeshCounter);

	m_pRenderDevice->Submit(commandLists.data(), 1);
	m_pRenderDevice->Execute();
	m_pRenderDevice->WaitForIdle();

	scene.SetMeshes(std::move(meshes));
}

} // namespace DE
