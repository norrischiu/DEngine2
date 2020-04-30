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
	SceneLoader sceneLoader;
	sceneLoader.Init(renderDevice);
	sceneLoader.SetRootPath("..\\Assets\\");
	sceneLoader.Load("SampleScene", m_scene);
	{
		auto& light = PointLight::Create();
		m_scene.Add(light);
		light.position = float3{ 0.0f, 5.0f, 0.0f };
		light.color = float3{ 1.0f, 1.0f, 1.0f };
		light.intensity = 1.0f;
		light.enable = true;
	}
	{
		QuadLight& light = QuadLight::Create();
		m_scene.Add(light);
		light.position = float3{ 0.0f, 0.0f, 5.0f };
		light.width = 4.0f;
		light.height = 4.0f;
		light.color = float3{ 1.0f, 1.0f, 1.0f };
		light.intensity = 1.0f;
		light.enable = true;

		Mesh& mesh = Mesh::Create();
		light.mesh = mesh.Index();
		m_scene.Add(mesh);
		{
			float x = light.position.x + light.width / 2.0f;
			float y = light.position.y + light.height / 2.0f;
			float z = light.position.z;
			float vertices[] = {
				-x, y, z,
				x, y, z,
				x, -y, z,
				-x, -y, z
			};
			mesh.m_Vertices.Init(renderDevice->m_Device, sizeof(float3), sizeof(vertices));
			mesh.m_Vertices.Update(vertices, sizeof(vertices));
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
			material.albedo = float3{ 1.0f, 1.0f, 1.0f };
			material.shadingType = ShadingType::AlbedoOnly;
			mesh.m_MaterialID = material.Index();
		}
	}

	TextureLoader texLoader;
	texLoader.Init(renderDevice);
	Texture equirectangularMap;
	texLoader.Load(equirectangularMap, "..\\Assets\\SampleScene\\Textures\\gym_entrance_1k.tex");
	Texture depth;
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depth.Init(renderDevice->m_Device, 1024, 768, 1, 1, DXGI_FORMAT_D32_FLOAT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &clearValue);
	Texture skybox;
	skybox.Init(renderDevice->m_Device, 512, 512, 6, 1, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	Texture irradianceMap;
	irradianceMap.Init(renderDevice->m_Device, 32, 32, 6, 1, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	Texture prefilteredEnvMap;
	prefilteredEnvMap.Init(renderDevice->m_Device, 128, 128, 6, 5, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	Texture BRDFIntegrationMap;
	BRDFIntegrationMap.Init(renderDevice->m_Device, 512, 512, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	Texture LTCInverseMatrixMap;
	texLoader.Load(LTCInverseMatrixMap, "..\\Assets\\SampleScene\\Textures\\LTCInverseMatrixMap.tex", DXGI_FORMAT_R16G16B16A16_UNORM);
	Texture LTCNormMap;
	texLoader.Load(LTCNormMap, "..\\Assets\\SampleScene\\Textures\\LTCNormMap.tex", DXGI_FORMAT_R16G16B16A16_UNORM);

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
		m_forwardPass.Setup(renderDevice, data);
	}
	m_precomputeDiffuseIBLPass.Setup(renderDevice, equirectangularMap, skybox, irradianceMap);
	m_precomputeSpecularIBLPass.Setup(renderDevice, skybox, prefilteredEnvMap, BRDFIntegrationMap);
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
		if (type == ShadingType::None)
		{
			m_frameData.batcher.Add(MaterialMeshBatcher::Flag::None, mesh);
		}
		else if (type == ShadingType::Textured)
		{
			m_frameData.batcher.Add(MaterialMeshBatcher::Flag::Textured, mesh);
		}
		else if (type == ShadingType::AlbedoOnly) 
		{
			m_frameData.batcher.Add(MaterialMeshBatcher::Flag::AlbedoOnly, mesh);
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

			float x = light.position.x + light.width / 2.0f;
			float y = light.position.y + light.height / 2.0f;
			float z = light.position.z;
			float vertices[] = {
				-x, y, z,
				x, y, z,
				x, -y, z,
				-x, -y, z
			};
			Mesh& mesh = Mesh::Get(light.mesh);
			mesh.m_Vertices.Update(vertices, sizeof(vertices));
		}
	});
	ImGui::End();
}
