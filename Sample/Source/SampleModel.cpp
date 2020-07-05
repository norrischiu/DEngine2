// Window
#include <d3d12.h>
// Cpp
#include <fstream>
// Engine
#include <DECore/FileSystem/FileLoader.h>
#include <DERendering/DERendering.h>
#include <DERendering/Device/RenderDevice.h>
#include <DERendering/Device/CopyCommandList.h>
#include <DERendering/DataType/GraphicsDataType.h>
#include <DERendering/DataType/LightType.h>
#include <DERendering/Imgui/imgui.h>
#include <DEGame/Loader/SceneLoader.h>
#include <DEGame/Loader/TextureLoader.h>
#include <DEGame/Component/Camera.h>
#include "SampleModel.h"

using namespace DE;

void SampleModel::Setup(RenderDevice *renderDevice)
{
	TextureLoader texLoader(renderDevice);
	texLoader.LoadDefaultTexture();
	SceneLoader sceneLoader(renderDevice, "..\\Assets\\");
	sceneLoader.Load("SampleScene", m_scene);
	{
		auto& light = PointLight::Create();
		m_scene.Add(light);
		light.position = float3{ 0.0f, 5.0f, 0.0f };
		light.color = float3{ 1.0f, 1.0f, 1.0f };
		light.intensity = 1.0f;
		light.enable = true;
	}
	Texture areaLightTexture;
	texLoader.Load(areaLightTexture, "..\\Assets\\SampleScene\\Textures\\AreaLightTexture.tex", DXGI_FORMAT_R8G8B8A8_UNORM);
	{
		QuadLight& light = QuadLight::Create();
		m_scene.Add(light);
		light.position = float3{ 0.0f, 3.0f, 5.0f };
		light.width = 4.0f;
		light.height = 4.0f;
		light.color = float3{ 1.0f, 1.0f, 1.0f };
		light.intensity = 1.0f;
		light.enable = true;

		Mesh& mesh = Mesh::Create();
		light.mesh = mesh.Index();
		m_scene.Add(mesh);
		{
			float posX = light.position.x + light.width / 2.0f;
			float posY = light.position.y + light.height / 2.0f;
			float z = light.position.z;
			float negX = light.position.x - light.width / 2.0f;
			float negY = light.position.y - light.height / 2.0f;
			float vertices[] = {
				negX, posY, z,
				posX, posY, z,
				posX, negY, z,
				negX, negY, z
			};
			mesh.m_Vertices.Init(renderDevice->m_Device, sizeof(float3), sizeof(vertices));
			mesh.m_Vertices.Update(vertices, sizeof(vertices));
		}
		{
			float texcoords[] = {
				0.0f, 0.0f, 
				1.0f, 0.0f, 
				1.0f, 1.0f, 
				0.0f, 1.0f 
			};
			mesh.m_TexCoords.Init(renderDevice->m_Device, sizeof(float2), sizeof(texcoords));
			mesh.m_TexCoords.Update(texcoords, sizeof(texcoords));
		}
		{
			uint3 indices[] = { {0, 1, 2}, {2, 3, 0} };
			constexpr uint32_t size = 2 * sizeof(uint3);
			mesh.m_Indices.Init(renderDevice->m_Device, sizeof(uint32_t), size);
			mesh.m_Indices.Update(indices, size);
			mesh.m_iNumIndices = 6;
		}
		{
			Material& material = Material::Create();
			material.albedo = float3{ light.intensity, light.intensity, light.intensity };
			material.shadingType = ShadingType::AlbedoOnly;
			material.m_Textures[0] = areaLightTexture;
			mesh.m_MaterialID = material.Index();
		}
	}

	Texture equirectangularMap;
	texLoader.Load(equirectangularMap, "..\\Assets\\SampleScene\\Textures\\gym_entrance_4k.tex", DXGI_FORMAT_R32G32B32A32_FLOAT);
	Texture depth;
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depth.Init(renderDevice->m_Device, 1024, 768, 1, 1, DXGI_FORMAT_D32_FLOAT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &clearValue);
	Texture skybox;
	skybox.Init(renderDevice->m_Device, 512, 512, 6, 1, DXGI_FORMAT_R11G11B10_FLOAT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	Texture irradianceMap;
	irradianceMap.Init(renderDevice->m_Device, 64, 64, 6, 1, DXGI_FORMAT_R11G11B10_FLOAT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	Texture prefilteredEnvMap;
	prefilteredEnvMap.Init(renderDevice->m_Device, 128, 128, 6, 5, DXGI_FORMAT_R11G11B10_FLOAT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	Texture BRDFIntegrationMap;
	BRDFIntegrationMap.Init(renderDevice->m_Device, 512, 512, 1, 1, DXGI_FORMAT_R16G16_UNORM, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	Texture LTCInverseMatrixMap;
	texLoader.Load(LTCInverseMatrixMap, "..\\Assets\\SampleScene\\Textures\\LTCInverseMatrixMap.tex", DXGI_FORMAT_R16G16B16A16_UNORM);
	Texture LTCNormMap;
	texLoader.Load(LTCNormMap, "..\\Assets\\SampleScene\\Textures\\LTCNormMap.tex", DXGI_FORMAT_R16G16B16A16_UNORM);
	Texture filteredAreaLightTexture;
	filteredAreaLightTexture.Init(renderDevice->m_Device, 2048, 2048, 1, log2f(2048) + 1, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	m_Camera.Init(Vector3(0.0f, 0.0f, -3.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), PI / 2.0f, 1024.0f / 768.0f, 0.1f, 100.0f);
	m_commandList.Init(renderDevice);

	{
		ForwardPass::Data data;
		data.depth = depth;
		data.irradianceMap = irradianceMap;
		data.prefilteredEnvMap = prefilteredEnvMap;
		data.BRDFIntegrationMap = BRDFIntegrationMap;
		data.LTCInverseMatrixMap = LTCInverseMatrixMap;
		data.LTCNormMap = LTCNormMap;
		data.filteredAreaLightTexture = filteredAreaLightTexture;
		m_forwardPass.Setup(renderDevice, data);
	}
	m_precomputeDiffuseIBLPass.Setup(renderDevice, equirectangularMap, skybox, irradianceMap);
	m_precomputeSpecularIBLPass.Setup(renderDevice, skybox, prefilteredEnvMap, BRDFIntegrationMap);
	{
		PrefilterAreaLightTexturePass::Data data;
		data.src = areaLightTexture;
		data.dst = filteredAreaLightTexture;
		m_prefilterAreaLightTexturePass.Setup(renderDevice, data);
	}
	m_SkyboxPass.Setup(renderDevice, depth, skybox);
	m_UIPass.Setup(renderDevice);
}

void SampleModel::Update(RenderDevice &renderDevice, float dt)
{
	SetupUI();

	m_Camera.ParseInput(dt);

	// Prepare frame data
	m_scene.ForEach<Mesh>([&](Mesh& mesh) {
		const auto type = Material::Get(mesh.m_MaterialID).shadingType;
		if (type == ShadingType::NoNormalMap)
		{
			m_frameData.batcher.Add(MaterialMeshBatcher::Flag::NoNormalMap, mesh);
		}
		else if (type == ShadingType::Textured)
		{
			m_frameData.batcher.Add(MaterialMeshBatcher::Flag::Textured, mesh);
		}
		else if (type == ShadingType::AlbedoOnly) 
		{
			m_frameData.batcher.Add(MaterialMeshBatcher::Flag::Unlit, mesh);
		}
	});
	m_scene.ForEach<PointLight>([&](PointLight& light) {
		if (light.enable)
		{
			m_frameData.pointLights.push_back(light.Index());
		}
	});
	m_scene.ForEach<QuadLight>([&](QuadLight& light) {
		if (light.enable)
		{
			m_frameData.quadLights.push_back(light.Index());
		}
	});
	m_frameData.cameraWVP = m_Camera.GetCameraToScreen();
	m_frameData.cameraView = m_Camera.GetV();
	m_frameData.cameraProjection = m_Camera.GetP();
	m_frameData.cameraPos = m_Camera.GetPosition();

	// Render
	m_commandList.Begin();
	if (m_bFirstRun)
	{
		m_precomputeDiffuseIBLPass.Execute(m_commandList, m_frameData);
		m_precomputeSpecularIBLPass.Execute(m_commandList, m_frameData);
		m_prefilterAreaLightTexturePass.Execute(m_commandList, m_frameData);
		m_bFirstRun = false;
	}
	m_forwardPass.Execute(m_commandList, m_frameData);
	m_SkyboxPass.Execute(m_commandList, m_frameData);
	m_UIPass.Execute(m_commandList, m_frameData);
	renderDevice.Submit(&m_commandList, 1);

	// Reset
	renderDevice.Reset();
	m_frameData.batcher.Reset();
	m_frameData.pointLights.clear();
	m_frameData.quadLights.clear();
}

void SampleModel::SetupUI()
{
	bool active;
	ImGui::Begin("Edit", &active);
	m_scene.ForEach<Mesh>([&](Mesh& mesh) {
		char name[256];
		sprintf_s(name, "mesh_%i", mesh.Index());
		if (ImGui::CollapsingHeader(name))
		{
			auto &material = Material::Get(mesh.m_MaterialID);
			sprintf_s(name, "mat_%i", mesh.m_MaterialID);
			if (ImGui::TreeNode(name))
			{
				ImGui::SliderFloat3("Albedo", &material.albedo.x, 0.0f, 1.0f);
				ImGui::SliderFloat("Roughness", &material.roughness, 0.0f, 1.0f);
				ImGui::SliderFloat("Metallic", &material.metallic, 0.0f, 1.0f);
				ImGui::SliderFloat("AO", &material.ao, 0.0f, 1.0f);
				ImGui::TreePop();
			}
		}
	});
	m_scene.ForEach<PointLight>([&](PointLight& light) {
		char name[256];
		sprintf_s(name, "pointLight_%i", light.Index());
		if (ImGui::CollapsingHeader(name))
		{
			ImGui::Checkbox("Enable", &light.enable);
			ImGui::SliderFloat3("Position", &light.position.x, -50.0f, 50.0f);
			ImGui::SliderFloat3("Color", &light.color.x, 0.0f, 1.0f);
			ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 10.0f);
		}
	});
	m_scene.ForEach<QuadLight>([&](QuadLight& light) {
		char name[256];
		sprintf_s(name, "quadLight_%i", light.Index());
		if (ImGui::CollapsingHeader(name))
		{
			ImGui::Checkbox("Enable", &light.enable);
			ImGui::SliderFloat3("Position", &light.position.x, -50.0f, 50.0f);
			ImGui::SliderFloat("Width", &light.width, 0.0f, 50.0f);
			ImGui::SliderFloat("Height", &light.height, 0.0f, 50.0f);
			ImGui::SliderFloat3("Color", &light.color.x, 0.0f, 1.0f);
			ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 10.0f);

			float posX = light.position.x + light.width / 2.0f;
			float posY = light.position.y + light.height / 2.0f;
			float z = light.position.z;
			float negX = light.position.x - light.width / 2.0f;
			float negY = light.position.y - light.height / 2.0f;
			float vertices[] = {
				negX, posY, z,
				posX, posY, z,
				posX, negY, z,
				negX, negY, z
			};
			Mesh& mesh = Mesh::Get(light.mesh);
			mesh.m_Vertices.Update(vertices, sizeof(vertices));
			Material& material = Material::Get(mesh.m_MaterialID);
			material.albedo = float3{ light.intensity, light.intensity, light.intensity };
		}
	});
	ImGui::End();
}

SampleModel::~SampleModel()
{
	Texture::ReleaseDefault();
	Material::Release();
	Mesh::Release();
}
