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
		uint32_t lightIndex = PointLight::Create();
		PointLight& light = PointLight::Get(lightIndex);
		m_scene.AddLight(lightIndex);
		light.position = Vector3(0.0f, 5.0f, 0.0f);
		light.color = Vector3(1.0f, 1.0f, 1.0f);
		light.intensity = 1.0f;
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
	Texture LTCNormMap;

	m_Camera.Init(Vector3(0.0f, 0.0f, -3.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), PI / 2.0f, 1024.0f / 768.0f, 1.0f, 100.0f);
	m_commandList.Init(renderDevice);

	{
		ForwardPass::Data data;
		data.depth = depth;
		data.irradianceMap = irradianceMap;
		data.prefilteredEnvMap = prefilteredEnvMap;
		data.BRDFIntegrationMap = BRDFIntegrationMap;
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
	m_scene.ForEachMesh([&](uint32_t index) {
		const auto& mesh = Mesh::Get(index);
		const auto type = Material::Get(mesh.m_MaterialID).shadingType;
		if (type == ShadingType::None)
		{
			m_frameData.batcher.Add(MaterialMeshBatcher::Flag::None, index);
		}
		else if (type == ShadingType::Textured)
		{
			m_frameData.batcher.Add(MaterialMeshBatcher::Flag::Textured, index);
		}
	});
	m_scene.ForEachLight([&](uint32_t index) {
		m_frameData.pointLights.push_back(index);
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
}

void SampleModel::SetupUI()
{
	bool active;
	ImGui::Begin("Edit", &active);
	m_scene.ForEachMesh([&](uint32_t index) {
		auto &mesh = Mesh::Get(index);
		char name[256];
		sprintf_s(name, "mesh_%i", index);
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
	ImGui::End();
}
