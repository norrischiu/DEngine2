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

	TextureLoader texLoader;
	texLoader.Init(renderDevice);
	Texture equirectangularMap;
	texLoader.Load(equirectangularMap, "..\\Assets\\ShaderBall\\Textures\\gym_entrance_1k.tex");
	Texture skybox;
	skybox.Init(renderDevice->m_Device, 512, 512, 6, 1, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	Texture irradianceMap;
	irradianceMap.Init(renderDevice->m_Device, 32, 32, 6, 1, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

	m_Camera.Init(Vector3(0.0f, 0.0f, -3.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), PI / 2.0f, 1024.0f / 768.0f, 1.0f, 100.0f);
	m_commandList.Init(renderDevice);

	m_forwardPass.Setup(renderDevice, irradianceMap);
	m_precomputeCubemapPass.Setup(renderDevice, equirectangularMap, skybox, irradianceMap);
	m_SkyboxPass.Setup(renderDevice, *m_forwardPass.GetDepth(), skybox);
	m_UIPass.Setup(renderDevice);
}

void SampleModel::Update(RenderDevice &renderDevice, float dt)
{
	SetupUI();

	m_Camera.ParseInput(dt);

	// Prepare frame data
	m_scene.ForEachMesh([&](uint32_t index) {
		const auto& mesh = Meshes::Get(index);
		const auto type = Materials::Get(mesh.m_MaterialID).shadingType;
		if (type == ShadingType::None)
		{
			m_frameData.batcher.Add(MaterialMeshBatcher::Flag::None, index);
		}
		else if (type == ShadingType::Textured)
		{
			m_frameData.batcher.Add(MaterialMeshBatcher::Flag::Textured, index);
		}
	});
	m_frameData.cameraWVP = m_Camera.GetCameraToScreen();
	m_frameData.cameraView = m_Camera.GetV();
	m_frameData.cameraProjection = m_Camera.GetP();

	// Render
	m_commandList.Begin();
	if (m_bFirstRun)
	{
		m_precomputeCubemapPass.Execute(m_commandList, m_frameData);
		m_bFirstRun = false;
	}
	m_forwardPass.Execute(m_commandList, m_frameData);
	m_SkyboxPass.Execute(m_commandList, m_frameData);
	m_UIPass.Execute(m_commandList, m_frameData);
	renderDevice.Submit(&m_commandList, 1);

	// Reset
	renderDevice.Reset();
	m_frameData.batcher.Reset();
}

void SampleModel::SetupUI()
{
	bool active;
	ImGui::Begin("Edit", &active);
	m_scene.ForEachMesh([&](uint32_t index) {
		auto &mesh = Meshes::Get(index);
		char name[256];
		sprintf_s(name, "mesh_%i", index);
		if (ImGui::CollapsingHeader(name))
		{
			auto &material = Materials::Get(mesh.m_MaterialID);
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
