// Window
#include <d3d12.h>
// Cpp
#include <fstream>
// Engine
#include <DECore/FileSystem/FileLoader.h>
#include <DERendering/DERendering.h>
#include <DEGame/Loader/SceneLoader.h>
#include "SampleModel.h"

using namespace DE;

void SampleModel::Setup(RenderDevice& renderDevice, Framegraph& framegraph)
{
	SceneLoader loader;
	loader.Init(renderDevice);
	loader.SetRootPath("..\\Assets\\");
	loader.Load("ShaderBall");

	Vector<char> vs;
	Vector<char> ps;
	Job* vsCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\Pbr.vs.cso", vs);
	JobScheduler::Instance()->WaitOnMainThread(vsCounter);
	Job* psCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\Pbr.ps.cso", ps);
	JobScheduler::Instance()->WaitOnMainThread(psCounter);

	struct PerLightCBuffer
	{
		Vector3 lightPos;
		Vector3 eyePos;
	};

	struct PerObjectCBuffer
	{
		Matrix4 world;
		Matrix4 wvp;
	};

	struct TrianglePassData final
	{
		VertexBuffer position[10];
		VertexBuffer normal[10];
		VertexBuffer tangent[10];
		VertexBuffer texcoord[10];
		IndexBuffer ibs[10];
		GraphicsPipelineState pso;
		RootSignature rootSignature;
		RenderTargetView rtv[2];
		Texture depth;
		DepthStencilView dsv;
		ShaderResourceView srv[50];
		DescriptorHeap cpuDescriptorHeap;
		DescriptorHeap cbvSrvDescriptorHeap;
		DescriptorHeap rtvDescriptorHeap;
		DescriptorHeap dsvDescriptorHeap;
		uint32_t backBufferIndex = 0;

		ConstantBufferView vsCbv;
		ConstantBufferView psCbv;
		PerObjectCBuffer transforms;
		PerLightCBuffer light;

		RenderDevice* pDevice;
	};

	framegraph.AddPass<TrianglePassData>(
		"triangle",
		[&](TrianglePassData& data) {
		HRESULT hr;

		for (uint32_t i = 0; i < 6; ++i)
		{
			// vertex
			std::size_t num = Meshes::Get(i).m_Vertices.size();
			std::size_t size = num * sizeof(float3);
			auto& vertices = Meshes::Get(i).m_Vertices;

			data.position[i].Init(renderDevice.m_Device, sizeof(float3), size);

			void* address = nullptr;
			D3D12_RANGE range{ 0, size };
			data.position[i].ptr->Map(0, &range, &address);
			memcpy(address, vertices.data(), size);
			data.position[i].ptr->Unmap(0, &range);

			// normal
			num = Meshes::Get(i).m_Normals.size();
			size = num * sizeof(float3);
			auto& normals = Meshes::Get(i).m_Normals;

			data.normal[i].Init(renderDevice.m_Device, sizeof(float3), size);

			range = { 0, size };
			data.normal[i].ptr->Map(0, &range, &address);
			memcpy(address, normals.data(), size);
			data.normal[i].ptr->Unmap(0, &range);

			// tangent
			num = Meshes::Get(i).m_Tangents.size();
			size = num * sizeof(float3);
			auto& tangents = Meshes::Get(i).m_Tangents;

			data.tangent[i].Init(renderDevice.m_Device, sizeof(float3), size);

			range = D3D12_RANGE{ 0, size };
			data.tangent[i].ptr->Map(0, &range, &address);
			memcpy(address, tangents.data(), size);
			data.tangent[i].ptr->Unmap(0, &range);

			// texcoord
			num = Meshes::Get(i).m_TexCoords.size();
			size = num * sizeof(float2);
			auto& texcoords = Meshes::Get(i).m_TexCoords;

			data.texcoord[i].Init(renderDevice.m_Device, sizeof(float2), size);

			range = D3D12_RANGE{ 0, size };
			data.texcoord[i].ptr->Map(0, &range, &address);
			memcpy(address, texcoords.data(), size);
			data.texcoord[i].ptr->Unmap(0, &range);

			// index
			num = Meshes::Get(i).m_Indices.size();
			size = num * sizeof(uint3);
			auto& indices = Meshes::Get(i).m_Indices;

			data.ibs[i].Init(renderDevice.m_Device, sizeof(uint3), size);

			range = D3D12_RANGE{ 0, size };
			data.ibs[i].ptr->Map(0, &range, &address);
			memcpy(address, indices.data(), size);
			data.ibs[i].ptr->Unmap(0, &range);
		}
		{
			D3D12_ROOT_SIGNATURE_DESC desc = {};
			D3D12_ROOT_PARAMETER rootParameters[3] = {};

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

			rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL;
			rootParameters[2].Descriptor.RegisterSpace = 0;
			rootParameters[2].Descriptor.ShaderRegister = 1;

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
			desc.NumParameters = 3;
			desc.pParameters = rootParameters;

			data.rootSignature.Init(renderDevice.m_Device, desc);
		}
		{
			D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
			desc.pRootSignature = data.rootSignature.ptr;
			D3D12_SHADER_BYTECODE VS;
			VS.pShaderBytecode = vs.data();
			VS.BytecodeLength = vs.size();
			desc.VS = VS;

			InputLayout inputLayout;
			inputLayout.Add("POSITION", 0, 0, DXGI_FORMAT_R32G32B32_FLOAT);
			inputLayout.Add("NORMAL", 0, 1, DXGI_FORMAT_R32G32B32_FLOAT);
			inputLayout.Add("TANGENT", 0, 2, DXGI_FORMAT_R32G32B32_FLOAT);
			inputLayout.Add("TEXCOORD", 0, 3, DXGI_FORMAT_R32G32_FLOAT);

			desc.InputLayout = inputLayout.desc;
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

			D3D12_SHADER_BYTECODE PS;
			PS.pShaderBytecode = ps.data();
			PS.BytecodeLength = ps.size();
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
			depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			desc.DepthStencilState = depthStencilDesc;

			data.pso.Init(renderDevice.m_Device, desc);
		}
		{
			data.cpuDescriptorHeap.Init(renderDevice.m_Device, 65536, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, false);
			data.cbvSrvDescriptorHeap.Init(renderDevice.m_Device, 65536, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
			data.rtvDescriptorHeap.Init(renderDevice.m_Device, 2, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
			data.dsvDescriptorHeap.Init(renderDevice.m_Device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false);
		}
		{
			data.rtv[0].Init(renderDevice.m_Device, data.rtvDescriptorHeap, *renderDevice.GetBackBuffer(0));
			data.rtv[1].Init(renderDevice.m_Device, data.rtvDescriptorHeap, *renderDevice.GetBackBuffer(1));
		}
		{
			D3D12_CLEAR_VALUE clearValue = {};
			clearValue.DepthStencil.Depth = 1.0f;
			clearValue.Format = DXGI_FORMAT_D32_FLOAT;
			data.depth.Init(renderDevice.m_Device, 1024, 768, 1, 1, DXGI_FORMAT_D32_FLOAT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &clearValue);
			data.dsv.Init(renderDevice.m_Device, data.dsvDescriptorHeap, data.depth);
		}
		{
			data.vsCbv.Init(renderDevice.m_Device, 256);

			data.transforms.world = Matrix4::Identity;
			data.transforms.wvp = Matrix4::PerspectiveProjection(PI / 2.0f, 1024.0f / 768.0f, 1.0f, 100.0f) * Matrix4::LookAtMatrix(Vector3(2.0f, 2.0f, -3.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
			void* address = nullptr;
			D3D12_RANGE range{ 0, sizeof(PerObjectCBuffer) };
			data.vsCbv.buffer.ptr->Map(0, &range, &address);
			memcpy(address, &data.transforms, sizeof(PerObjectCBuffer));
			data.vsCbv.buffer.ptr->Unmap(0, &range);
		}
		{
			data.psCbv.Init(renderDevice.m_Device, 256);

			data.light.lightPos = Vector3(1.0f, 1.0f, 0.0f);
			data.light.eyePos = Vector3(2.0f, 2.0f, -3.0f);
			void* address = nullptr;
			D3D12_RANGE range{ 0, sizeof(PerLightCBuffer) };
			data.psCbv.buffer.ptr->Map(0, &range, &address);
			memcpy(address, &data.light, sizeof(PerLightCBuffer));
			data.psCbv.buffer.ptr->Unmap(0, &range);
		}
		{
			for (uint32_t i = 0; i < 6; ++i)
			{
				auto& material = Materials::Get(Meshes::Get(i).m_MaterialID);
				for (uint32_t j = 0; j < 5; ++j)
				{
					if (material.m_Textures[j].ptr != nullptr)
					{
						data.srv[i * 5 + j].Init(renderDevice.m_Device, data.cpuDescriptorHeap, material.m_Textures[j]);
					}
				}
			}
		}

		data.pDevice = &renderDevice;
	},
		[=](TrianglePassData& data, CommandList& commandList) {
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
		commandList.ptr->SetGraphicsRootConstantBufferView(0, data.vsCbv.buffer.ptr->GetGPUVirtualAddress());
		commandList.ptr->SetGraphicsRootConstantBufferView(2, data.psCbv.buffer.ptr->GetGPUVirtualAddress());

		D3D12_GPU_DESCRIPTOR_HANDLE descriptorTable;
		descriptorTable = data.cbvSrvDescriptorHeap.ptr->GetGPUDescriptorHandleForHeapStart();
		for (uint32_t i = 0; i < 6; ++i)
		{
			// copy descriptor
			uint32_t materialID = Meshes::Get(i).m_MaterialID;
			data.pDevice->m_Device.ptr->CopyDescriptorsSimple(5, data.cbvSrvDescriptorHeap.current, data.srv[materialID * 5].descriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			commandList.ptr->SetGraphicsRootDescriptorTable(1, descriptorTable);
			commandList.ptr->IASetVertexBuffers(0, 1, &data.position[i].view);
			commandList.ptr->IASetVertexBuffers(1, 1, &data.normal[i].view);
			commandList.ptr->IASetVertexBuffers(2, 1, &data.tangent[i].view);
			commandList.ptr->IASetVertexBuffers(3, 1, &data.texcoord[i].view);
			commandList.ptr->IASetIndexBuffer(&data.ibs[i].view);
			commandList.ptr->DrawIndexedInstanced(Meshes::Get(i).m_Indices.size() * 3, 1, 0, 0, 0);

			data.cbvSrvDescriptorHeap.current.ptr += data.pDevice->m_Device.ptr->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
			descriptorTable.ptr += data.pDevice->m_Device.ptr->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
		}

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		commandList.ptr->ResourceBarrier(1, &barrier);

		data.backBufferIndex = 1 - data.backBufferIndex;

		data.cbvSrvDescriptorHeap.current = data.cbvSrvDescriptorHeap.ptr->GetCPUDescriptorHandleForHeapStart();
	});

	framegraph.Compile();
}
