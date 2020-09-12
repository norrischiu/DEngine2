// Engine
#include <DEGame/Renderer/Renderer.h>
#include <DERendering/Device/RenderDevice.h>
#include <DEGame/Loader/TextureLoader.h>
#include <DEGame/Component/Camera.h>
// Windows
#include <DXProgrammableCapture.h>

namespace DE
{

void Renderer::Init(const Desc& desc)
{
	RenderDevice::Desc rdDesc = {};
	rdDesc.hWnd_ = desc.hWnd;
	rdDesc.windowWidth_ = desc.windowWidth;
	rdDesc.windowHeight_ = desc.windowHeight;
	rdDesc.backBufferCount_ = desc.backBufferCount;
	m_RenderDevice.Init(rdDesc);

	TextureLoader::LoadDefaultTexture(&m_RenderDevice);

	// init imgui setup
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.DisplaySize = ImVec2(static_cast<float>(desc.windowWidth), static_cast<float>(desc.windowHeight));
	
	TextureLoader texLoader(&m_RenderDevice);
	Texture areaLightTexture;
	texLoader.Load(areaLightTexture, "..\\Assets\\SampleScene\\Textures\\AreaLightTexture.tex", DXGI_FORMAT_R8G8B8A8_UNORM);
	Texture equirectangularMap;
	texLoader.Load(equirectangularMap, "..\\Assets\\SampleScene\\Textures\\gym_entrance_4k.tex", DXGI_FORMAT_R32G32B32A32_FLOAT);
	Texture depth;
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depth.Init(m_RenderDevice.m_Device, 1024, 768, 1, 1, DXGI_FORMAT_D32_FLOAT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &clearValue);
	Texture skybox;
	skybox.Init(m_RenderDevice.m_Device, 512, 512, 6, 1, DXGI_FORMAT_R11G11B10_FLOAT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	Texture irradianceMap;
	irradianceMap.Init(m_RenderDevice.m_Device, 64, 64, 6, 1, DXGI_FORMAT_R11G11B10_FLOAT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	Texture prefilteredEnvMap;
	prefilteredEnvMap.Init(m_RenderDevice.m_Device, 128, 128, 6, 5, DXGI_FORMAT_R11G11B10_FLOAT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	Texture BRDFIntegrationMap;
	BRDFIntegrationMap.Init(m_RenderDevice.m_Device, 512, 512, 1, 1, DXGI_FORMAT_R16G16_UNORM, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	Texture LTCInverseMatrixMap;
	texLoader.Load(LTCInverseMatrixMap, "..\\Assets\\SampleScene\\Textures\\LTCInverseMatrixMap.tex", DXGI_FORMAT_R16G16B16A16_UNORM);
	Texture LTCNormMap;
	texLoader.Load(LTCNormMap, "..\\Assets\\SampleScene\\Textures\\LTCNormMap.tex", DXGI_FORMAT_R16G16B16A16_UNORM);
	Texture filteredAreaLightTexture;
	filteredAreaLightTexture.Init(m_RenderDevice.m_Device, 2048, 2048, 1, static_cast<uint32_t>(log2f(2048)) + 1, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	Buffer clusters;
	const uint32_t numCluster = 1024 * 768 / 64 / 64 * 16;
	clusters.Init(m_RenderDevice.m_Device, numCluster * sizeof(float4) * 2, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	Buffer clusterVisibilities;
	clusterVisibilities.Init(m_RenderDevice.m_Device, numCluster * sizeof(uint32_t), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	Buffer clusterLightInfoList;
	clusterLightInfoList.Init(m_RenderDevice.m_Device, sizeof(uint3) * numCluster, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	Buffer lightIndexList;
	lightIndexList.Init(m_RenderDevice.m_Device, sizeof(uint32_t) * 1024 * 256, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	Buffer lightIndexListCounter;
	lightIndexListCounter.Init(m_RenderDevice.m_Device, sizeof(uint32_t), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	
	m_commandList.Init(&m_RenderDevice);
	
	m_Camera.Init(Vector3(0.0f, 2.0f, -4.0f), Vector3(0.0f, 2.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), PI / 2.0f, 1024.0f / 768.0f, 0.1f, 100.0f);
	{
		ZPrePass::Data data;
		data.depth = depth;
		data.clusterVisibilities = clusterVisibilities;
		m_zPrePass.Setup(&m_RenderDevice, data);
	}
	{
		ClusterLightPass::Data data;
		data.resolutionX = desc.windowWidth;
		data.resolutionY = desc.windowHeight;
		data.numSlice = 16;
		data.clusterSize = 64;
		data.clusters = clusters;
		data.clusterLightInfoList = clusterLightInfoList;
		data.lightIndexList = lightIndexList;
		data.lightIndexListCounter = lightIndexListCounter;
		m_clusterLightPass.Setup(&m_RenderDevice, data);
	}
	{
		ForwardPass::Data data;
		data.depth = depth;
		data.irradianceMap = irradianceMap;
		data.prefilteredEnvMap = prefilteredEnvMap;
		data.BRDFIntegrationMap = BRDFIntegrationMap;
		data.LTCInverseMatrixMap = LTCInverseMatrixMap;
		data.LTCNormMap = LTCNormMap;
		data.filteredAreaLightTexture = filteredAreaLightTexture;
		data.clusterLightInfoList = clusterLightInfoList;
		data.lightIndexList = lightIndexList;
		m_forwardPass.Setup(&m_RenderDevice, data);
	}
	{
		PrecomputeDiffuseIBLPass::Data data;
		data.equirectangularMap = equirectangularMap;
		data.cubemap = skybox;
		data.irradianceMap = irradianceMap;
		m_precomputeDiffuseIBLPass.Setup(&m_RenderDevice, data);
	}
	{
		PrecomputeSpecularIBLPass::Data data;
		data.cubemap = skybox;
		data.prefilteredEnvMap = prefilteredEnvMap;
		data.LUT = BRDFIntegrationMap;
		m_precomputeSpecularIBLPass.Setup(&m_RenderDevice, data);
	}
	{
		PrefilterAreaLightTexturePass::Data data;
		data.src = areaLightTexture;
		data.dst = filteredAreaLightTexture;
		m_prefilterAreaLightTexturePass.Setup(&m_RenderDevice, data);
	}
	{
		SkyboxPass::Data data;
		data.cubemap = skybox;
		data.depth = depth;
		m_SkyboxPass.Setup(&m_RenderDevice, data);
	}
	{
		UIPass::Data data;
		m_UIPass.Setup(&m_RenderDevice, data);
	}

	m_Desc = desc;
}

void Renderer::Update(float dt)
{
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
			if (light.debug)
			{
				m_frameData.batcher.Add(MaterialMeshBatcher::Flag::Wireframe, Mesh::Get(light.debugMesh));
			}
		}
	});
	m_scene.ForEach<QuadLight>([&](QuadLight& light) {
		if (light.enable)
		{
			m_frameData.quadLights.push_back(light.Index());
			m_frameData.batcher.Add(MaterialMeshBatcher::Flag::Unlit, Mesh::Get(light.mesh));
		}
	});
	m_frameData.camera.zNear = m_Camera.GetZNear();
	m_frameData.camera.zFar = m_Camera.GetZFar();
	m_frameData.camera.wvp = m_Camera.GetCameraToScreen();
	m_frameData.camera.view = m_Camera.GetV();
	m_frameData.camera.projection = m_Camera.GetP();
	m_frameData.camera.pos = m_Camera.GetPosition();

	const auto& clusteringPassData = m_clusterLightPass.GetData();
	m_frameData.clusteringInfo.clusterSize = clusteringPassData.clusterSize;
	m_frameData.clusteringInfo.numCluster = { clusteringPassData.resolutionX / clusteringPassData.clusterSize, clusteringPassData.resolutionY / clusteringPassData.clusterSize };
	m_frameData.clusteringInfo.resolution = { clusteringPassData.resolutionX, clusteringPassData.resolutionY };
	m_frameData.clusteringInfo.numSlice = clusteringPassData.numSlice;

	// Render
	auto& commandList = m_commandList;
	commandList.Begin();
	if (m_bFirstRun)
	{
		m_precomputeDiffuseIBLPass.Execute(commandList, m_frameData);
		m_precomputeSpecularIBLPass.Execute(commandList, m_frameData);
		m_prefilterAreaLightTexturePass.Execute(commandList, m_frameData);
		m_bFirstRun = false;
	}
	m_zPrePass.Execute(commandList, m_frameData);
	m_clusterLightPass.Execute(commandList, m_frameData);
	m_forwardPass.Execute(commandList, m_frameData);
	m_SkyboxPass.Execute(commandList, m_frameData);
	m_UIPass.Execute(commandList, m_frameData);
	m_RenderDevice.Submit(&commandList, 1);

	// Reset
	m_frameData.batcher.Reset();
	m_frameData.pointLights.clear();
	m_frameData.quadLights.clear();
}

void Renderer::Render()
{
	static ComPtr<IDXGraphicsAnalysis> ga;
	if (!ga && (DXGIGetDebugInterface1(0, IID_PPV_ARGS(&ga)) == S_OK))
	{
		ga->BeginCapture();
		m_RenderDevice.ExecuteAndPresent();
		m_RenderDevice.WaitForIdle();
		ga->EndCapture();
	}
	else
	{
		m_RenderDevice.ExecuteAndPresent();
		m_RenderDevice.WaitForIdle();
	}
}

void Renderer::Destruct()
{
	ImGui::DestroyContext();

	Texture::ReleaseDefault();
	Material::Release();
	Mesh::Release();
}

}