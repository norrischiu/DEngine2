// Window
#include <d3d12.h>
// Cpp
#include <fstream>
// Engine
#include <DECore/FileSystem/FileLoader.h>
#include <DERendering/DERendering.h>
#include <DERendering/Device/CopyCommandList.h>
#include <DEGame/Loader/SceneLoader.h>
#include <DEGame/Loader/TextureLoader.h>
#include <DEGame/Component/Camera.h>
#include "SampleModel.h"

using namespace DE;

void SampleModel::Setup(RenderDevice& renderDevice)
{
	SceneLoader sceneLoader;
	sceneLoader.Init(renderDevice);
	sceneLoader.SetRootPath("..\\Assets\\");
	sceneLoader.Load("ShaderBall");

	TextureLoader texLoader;
	texLoader.Init(renderDevice);

	m_Camera.Init(Vector3(2.0f, 2.0f, -3.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), PI / 2.0f, 1024.0f / 768.0f, 1.0f, 100.0f);
	m_commandList.Init(renderDevice.m_Device);
	m_forwardPass.Setup(renderDevice);

#if 0
	Vector<char> iblPrecomputeVs;
	Vector<char> iblPrecomputePs;
	Job* iblPrecomputeVsCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\Position.vs.cso", iblPrecomputeVs);
	JobScheduler::Instance()->WaitOnMainThread(iblPrecomputeVsCounter);
	Job* iblPrecomputePsCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\SampleEquirectangular.ps.cso", iblPrecomputePs);
	JobScheduler::Instance()->WaitOnMainThread(iblPrecomputePsCounter);


	struct PerViewCBuffer final
	{
		Matrix4 wvp;
	};
	static const float cubeVertices[] = {
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		1.0f,-1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f,-1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f,-1.0f, 1.0f
	};
	struct IBLPrecomputePassData final
	{
		Texture equirectangularHdr;
		Texture cubemapHdr;
		VertexBuffer cubeVertices;
		ConstantBufferView cbvs[6];
		RenderTargetView rtvs[6]; // cubemap face
		ShaderResourceView srv;
		GraphicsPipelineState pso;
		RootSignature rootSignature;
		DescriptorHeap cpuDescriptorHeap;
		DescriptorHeap cbvSrvDescriptorHeap;
		DescriptorHeap rtvDescriptorHeap;
		RenderDevice* pDevice;

		bool bFirstRun = true;
	};

	framegraph.AddPass<IBLPrecomputePassData>(
		"IBLPrecompute",
		[&](IBLPrecomputePassData& data)
	{
		HRESULT hr;
		{
			data.cubeVertices.Init(renderDevice.m_Device, sizeof(float3), sizeof(cubeVertices));
			data.cubeVertices.Update(cubeVertices, sizeof(cubeVertices));
		}
		{
			data.cpuDescriptorHeap.Init(renderDevice.m_Device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, false);
			data.cbvSrvDescriptorHeap.Init(renderDevice.m_Device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
			data.rtvDescriptorHeap.Init(renderDevice.m_Device, 6, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
		}
		{
			texLoader.Load(data.equirectangularHdr, "..\\Assets\\ShaderBall\\Textures\\gym_entrance_1k.tex");
			data.srv.Init(renderDevice.m_Device, data.cpuDescriptorHeap, data.equirectangularHdr);
		}
		{
			data.cubemapHdr.Init(renderDevice.m_Device, 512, 512, 6, 1, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
			for (uint32_t i = 0; i < 6; ++i) {
				data.rtvs[i].InitFromTexture2DArray(renderDevice.m_Device, data.rtvDescriptorHeap, data.cubemapHdr, i, 1);
			}
		}
		{
			Matrix4 perspective = Matrix4::PerspectiveProjection(PI / 2.0f, 1.0f, 0.1f, 10.0f);
			Matrix4 views[6] = {
				Matrix4::LookAtMatrix(Vector3::Zero, Vector3::UnitX, Vector3::UnitY),
				Matrix4::LookAtMatrix(Vector3::Zero, Vector3::NegativeUnitX, Vector3::UnitY),
				Matrix4::LookAtMatrix(Vector3::Zero, Vector3::UnitY, Vector3::UnitZ),
				Matrix4::LookAtMatrix(Vector3::Zero, Vector3::NegativeUnitY, Vector3::NegativeUnitZ),
				Matrix4::LookAtMatrix(Vector3::Zero, Vector3::UnitZ, Vector3::UnitY),
				Matrix4::LookAtMatrix(Vector3::Zero, Vector3::NegativeUnitZ, Vector3::UnitY),
			};

			PerViewCBuffer transform;
			for (uint32_t i = 0; i < 6; ++i) {
				transform.wvp = perspective * views[i];
				data.cbvs[i].Init(renderDevice.m_Device, sizeof(PerViewCBuffer));
				data.cbvs[i].buffer.Update(&transform, sizeof(PerViewCBuffer));
			}
		}
		{
			D3D12_ROOT_SIGNATURE_DESC desc = {};
			D3D12_ROOT_PARAMETER rootParameters[2] = {};

			rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_VERTEX;
			rootParameters[0].Descriptor.RegisterSpace = 0;
			rootParameters[0].Descriptor.ShaderRegister = 0;

			rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL;
			D3D12_DESCRIPTOR_RANGE range = {};
			range.NumDescriptors = 1;
			range.BaseShaderRegister = 0;
			range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			range.RegisterSpace = 0;
			range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
			rootParameters[1].DescriptorTable.pDescriptorRanges = &range;

			D3D12_STATIC_SAMPLER_DESC sampler = {};
			sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL;
			sampler.ShaderRegister = 0;
			sampler.RegisterSpace = 0;
			sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			sampler.MipLODBias = 0.0f;
			sampler.MaxAnisotropy = 1;
			sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			sampler.MinLOD = 0;
			sampler.MaxLOD = D3D12_FLOAT32_MAX;

			desc.NumStaticSamplers = 1;
			desc.pStaticSamplers = &sampler;
			desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
			desc.NumParameters = 2;
			desc.pParameters = rootParameters;

			data.rootSignature.Init(renderDevice.m_Device, desc);
		}
		{
			D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
			desc.pRootSignature = data.rootSignature.ptr;
			D3D12_SHADER_BYTECODE VS;
			VS.pShaderBytecode = iblPrecomputeVs.data();
			VS.BytecodeLength = iblPrecomputeVs.size();
			desc.VS = VS;

			InputLayout inputLayout;
			inputLayout.Add("POSITION", 0, 0, DXGI_FORMAT_R32G32B32_FLOAT);

			desc.InputLayout = inputLayout.desc;
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

			D3D12_SHADER_BYTECODE PS;
			PS.pShaderBytecode = iblPrecomputePs.data();
			PS.BytecodeLength = iblPrecomputePs.size();
			desc.PS = PS;
			D3D12_RASTERIZER_DESC rasterizerDesc = {};
			rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
			rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
			desc.RasterizerState = rasterizerDesc;
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			desc.NumRenderTargets = 1;
			desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			DXGI_SAMPLE_DESC sampleDesc = {};
			sampleDesc.Count = 1;
			desc.SampleMask = 0xffffff;
			desc.SampleDesc = sampleDesc;
			D3D12_BLEND_DESC blendDesc = {};
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			desc.BlendState = blendDesc;
			D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
			desc.DepthStencilState = depthStencilDesc;

			data.pso.Init(renderDevice.m_Device, desc);
		}

		data.pDevice = &renderDevice;
	},
		[=](IBLPrecomputePassData& data, CommandList& commandList)
	{
		if (!data.bFirstRun) {
			return;
		}

		D3D12_VIEWPORT viewport;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = 512;
		viewport.Height = 512;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		D3D12_RECT scissorRect;
		scissorRect.left = 0;
		scissorRect.top = 0;
		scissorRect.right = 512;
		scissorRect.bottom = 512;
		commandList.ptr->RSSetViewports(1, &viewport);
		commandList.ptr->RSSetScissorRects(1, &scissorRect);

		commandList.ptr->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList.ptr->SetGraphicsRootSignature(data.rootSignature.ptr);
		commandList.ptr->SetPipelineState(data.pso.ptr);
		commandList.ptr->SetDescriptorHeaps(1, &data.cbvSrvDescriptorHeap.ptr);

		D3D12_GPU_DESCRIPTOR_HANDLE descriptorTable;
		descriptorTable = data.cbvSrvDescriptorHeap.ptr->GetGPUDescriptorHandleForHeapStart();
		data.pDevice->m_Device.ptr->CopyDescriptorsSimple(1, data.cbvSrvDescriptorHeap.current, data.srv.descriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		commandList.ptr->SetGraphicsRootDescriptorTable(1, descriptorTable);

		commandList.ptr->IASetVertexBuffers(0, 1, &data.cubeVertices.view);

		for (uint32_t i = 0; i < 6; ++i)
		{
			// update cbuffer
			commandList.ptr->SetGraphicsRootConstantBufferView(0, data.cbvs[i].buffer.ptr->GetGPUVirtualAddress());

			commandList.ptr->OMSetRenderTargets(1, &data.rtvs[i].descriptor, false, nullptr);
			commandList.ptr->DrawInstanced(36, 1, 0, 0);
		}

		data.bFirstRun = false;
	});
#endif

#if 0
	Vector<char> skyboxVs;
	Vector<char> skyboxPs;
	Job* skyboxVsCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\Skybox.vs.cso", vs);
	JobScheduler::Instance()->WaitOnMainThread(skyboxVsCounter);
	Job* skyboxPsCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\Skybox.ps.cso", ps);
	JobScheduler::Instance()->WaitOnMainThread(skyboxPsCounter);

	struct SkyboxPassData final
	{
		VertexBuffer cubeVertices;
		ConstantBufferView cbv;
		RenderTargetView rtv;
		ShaderResourceView srv;
		GraphicsPipelineState pso;
		RootSignature rootSignature;
		DescriptorHeap cpuDescriptorHeap;
		DescriptorHeap cbvSrvDescriptorHeap;
		DescriptorHeap rtvDescriptorHeap;
		RenderDevice* pDevice;
	};

	framegraph.AddPass<SkyboxPassData>(
		"skybox",
		[&](SkyboxPassData& data)
	{
		HRESULT hr;
		{
			data.cubeVertices.Init(renderDevice.m_Device, sizeof(float3), sizeof(cubeVertices));
			data.cubeVertices.Update(cubeVertices, sizeof(cubeVertices));
		}
		{
			D3D12_ROOT_SIGNATURE_DESC desc = {};
			D3D12_ROOT_PARAMETER rootParameters[2] = {};

			rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_VERTEX;
			rootParameters[0].Descriptor.RegisterSpace = 0;
			rootParameters[0].Descriptor.ShaderRegister = 0;

			rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL;
			D3D12_DESCRIPTOR_RANGE range = {};
			range.NumDescriptors = 5;
			range.BaseShaderRegister = 0;
			range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			range.RegisterSpace = 0;
			range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
			rootParameters[1].DescriptorTable.pDescriptorRanges = &range;

			D3D12_STATIC_SAMPLER_DESC sampler = {};
			sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL;
			sampler.ShaderRegister = 0;
			sampler.RegisterSpace = 0;
			sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			sampler.MipLODBias = 0.0f;
			sampler.MaxAnisotropy = 1;
			sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			sampler.MinLOD = 0;
			sampler.MaxLOD = D3D12_FLOAT32_MAX;

			desc.NumStaticSamplers = 1;
			desc.pStaticSamplers = &sampler;
			desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
			desc.NumParameters = 2;
			desc.pParameters = rootParameters;

			data.rootSignature.Init(renderDevice.m_Device, desc);
		}
		{
			D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
			desc.pRootSignature = data.rootSignature.ptr;
			D3D12_SHADER_BYTECODE VS;
			VS.pShaderBytecode = skyboxVs.data();
			VS.BytecodeLength = skyboxVs.size();
			desc.VS = VS;

			InputLayout inputLayout;
			inputLayout.Add("POSITION", 0, 0, DXGI_FORMAT_R32G32B32_FLOAT);

			desc.InputLayout = inputLayout.desc;
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

			D3D12_SHADER_BYTECODE PS;
			PS.pShaderBytecode = skyboxPs.data();
			PS.BytecodeLength = skyboxPs.size();
			desc.PS = PS;
			D3D12_RASTERIZER_DESC rasterizerDesc = {};
			rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
			rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
			desc.RasterizerState = rasterizerDesc;
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			desc.NumRenderTargets = 1;
			desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			DXGI_SAMPLE_DESC sampleDesc = {};
			sampleDesc.Count = 1;
			desc.SampleMask = 0xffffff;
			desc.SampleDesc = sampleDesc;
			D3D12_BLEND_DESC blendDesc = {};
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			desc.BlendState = blendDesc;
			D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
			depthStencilDesc.DepthEnable = true;
			depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
			depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			desc.DepthStencilState = depthStencilDesc;

			data.pso.Init(renderDevice.m_Device, desc);
		}
		{
			data.cpuDescriptorHeap.Init(renderDevice.m_Device, 65536, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, false);
			data.cbvSrvDescriptorHeap.Init(renderDevice.m_Device, 65536, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
		}
		{
			data.cbv.Init(renderDevice.m_Device, 256);
		}

		data.pDevice = &renderDevice;
	},
		[=](SkyboxPassData& data, CommandList& commandList)
	{
		PerViewCBuffer transform;
		transform.wvp = m_Camera.GetPV();
		data.cbv.buffer.Update(&transform, sizeof(transform));

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = data.rtv[data.backBufferIndex].resource;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		commandList.ptr->ResourceBarrier(1, &barrier);

		D3D12_VIEWPORT viewport;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = 1024;
		viewport.Height = 768;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		D3D12_RECT scissorRect;
		scissorRect.left = 0;
		scissorRect.top = 0;
		scissorRect.right = 1024;
		scissorRect.bottom = 768;
		commandList.ptr->RSSetViewports(1, &viewport);
		commandList.ptr->RSSetScissorRects(1, &scissorRect);

		commandList.ptr->OMSetRenderTargets(1, &data.rtv[data.backBufferIndex].descriptor, false, &data.dsv.descriptor);
		float clearColor[4] = { 0.5f, 0.5f, 0.5f, 0.1f };
		commandList.ptr->ClearRenderTargetView(data.rtv[data.backBufferIndex].descriptor, clearColor, 0, nullptr);
		commandList.ptr->ClearDepthStencilView(data.dsv.descriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		commandList.ptr->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList.ptr->SetGraphicsRootSignature(data.rootSignature.ptr);
		commandList.ptr->SetPipelineState(data.pso.ptr);
		commandList.ptr->SetDescriptorHeaps(1, &data.cbvSrvDescriptorHeap.ptr);
		commandList.ptr->SetGraphicsRootConstantBufferView(0, data.cbv.buffer.ptr->GetGPUVirtualAddress());

		D3D12_GPU_DESCRIPTOR_HANDLE descriptorTable;
		descriptorTable = data.cbvSrvDescriptorHeap.ptr->GetGPUDescriptorHandleForHeapStart();
		commandList.ptr->SetGraphicsRootDescriptorTable(1, descriptorTable);
		commandList.ptr->IASetVertexBuffers(0, 1, &data.cubeVertices.view);

		commandList.ptr->DrawIndexedInstanced(36, 1, 0, 0, 0);

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		commandList.ptr->ResourceBarrier(1, &barrier);

		data.backBufferIndex = 1 - data.backBufferIndex;

		data.cbvSrvDescriptorHeap.current = data.cbvSrvDescriptorHeap.ptr->GetCPUDescriptorHandleForHeapStart();
	});
#endif

#if 0
	framegraph.Compile();
#endif
}

void SampleModel::Update(RenderDevice& renderDevice, float dt)
{
	m_Camera.ParseInput(dt);

	m_commandList.BeginRenderPass();
	m_forwardPass.Execute(m_commandList, m_Camera.GetPV());
	renderDevice.Submit(&m_commandList, 1);
	
}
