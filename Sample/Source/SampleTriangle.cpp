// Window
#include <d3d12.h>
// Cpp
#include <fstream>
// Engine
#include <DERendering/DERendering.h>
#include "SampleTriangle.h"

using namespace DE;

void SampleTriangle::Setup(D3D12Renderer& renderer, Framegraph& framegraph)
{
	Vector<char> vs;
	Vector<char> ps;
	std::ifstream fs;
	fs.open("../Bin/Assets/Shaders/Position.vs.cso", std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
	assert(fs);
	vs.resize(fs.tellg());
	fs.seekg(0, fs.beg);
	fs.read(vs.data(), vs.size());
	fs.close();

	fs.open("../Bin/Assets/Shaders/Red.ps.cso", std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
	assert(fs);
	ps.resize(fs.tellg());
	fs.seekg(0, fs.beg);
	fs.read(ps.data(), ps.size());
	fs.close();

	struct TrianglePassData final
	{
		VertexBuffer vb;
		GraphicsPipelineState pso;
		RootSignature rootSignature;
		RenderTargetView rtv[2];
		DescriptorHeap heap;
		DescriptorHeap descriptorHeap;
		uint32_t backBufferIndex = 0;
	};

	framegraph.AddPass<TrianglePassData>(
		"triangle",
		[&](TrianglePassData& data) {
			HRESULT hr;

			float vertices[9] = { 0.0, 0.5f, 0.0f, 0.45f, -0.5f, 0.0f, -0.45f, -0.5f, 0.0f };
			D3D12_HEAP_PROPERTIES heapProp = {};
			heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProp.CreationNodeMask = 1;
			heapProp.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC desc = {};
			desc.Width = sizeof(vertices);
			desc.Height = 1;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.Alignment = 0;
			desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			desc.SampleDesc.Count = 1;

			hr = renderer.m_Device.ptr->CreateCommittedResource(
				&heapProp,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&data.vb.ptr));
			assert(hr == S_OK);

			void* address = nullptr;
			D3D12_RANGE range{ 0, sizeof(vertices) };
			data.vb.ptr->Map(0, &range, &address);
			memcpy(address, vertices, sizeof(vertices));
			data.vb.ptr->Unmap(0, &range);

			{
				data.vb.m_VertexBufferView.BufferLocation = data.vb.ptr->GetGPUVirtualAddress();
				data.vb.m_VertexBufferView.SizeInBytes = sizeof(vertices);
				data.vb.m_VertexBufferView.StrideInBytes = sizeof(float) * 3;
			}
			{
				D3D12_ROOT_SIGNATURE_DESC desc = {};
				D3D12_ROOT_PARAMETER rootParameters[1] = {};

				rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_CBV;
				rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_VERTEX;
				rootParameters[0].Descriptor.RegisterSpace = 0;
				rootParameters[0].Descriptor.ShaderRegister = 1;

				desc.NumStaticSamplers = 0;
				desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
				desc.NumParameters = 1;
				desc.pParameters = rootParameters;

				data.rootSignature.Init(renderer.m_Device, desc);
			}
			{
				D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
				desc.pRootSignature = data.rootSignature.ptr;
				D3D12_SHADER_BYTECODE VS;
				VS.pShaderBytecode = vs.data();
				VS.BytecodeLength = vs.size();
				desc.VS = VS;

				D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
				D3D12_INPUT_ELEMENT_DESC inputElementDesc = {};
				inputElementDesc.SemanticName = "POSITION";
				inputElementDesc.SemanticIndex = 0;
				inputElementDesc.InputSlot = 0;
				inputElementDesc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
				inputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
				inputElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
				inputElementDesc.InstanceDataStepRate = 0;
				inputLayoutDesc.pInputElementDescs = &inputElementDesc;
				inputLayoutDesc.NumElements = 1;
				desc.InputLayout = inputLayoutDesc;
				desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

				D3D12_SHADER_BYTECODE PS;
				PS.pShaderBytecode = ps.data();
				PS.BytecodeLength = ps.size();
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

				data.pso.Init(renderer.m_Device, desc);
			}
			{
				data.heap.Init(renderer.m_Device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
				data.descriptorHeap.Init(renderer.m_Device, 2, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
			}
			{
				data.rtv[0].Init(renderer.m_Device, data.descriptorHeap, *renderer.GetBackBuffer(0));
				data.rtv[1].Init(renderer.m_Device, data.descriptorHeap, *renderer.GetBackBuffer(1));
			}
		},
		[=](TrianglePassData& data, CommandList& commandList) {
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = data.rtv[data.backBufferIndex].resource;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			commandList.ptr->ResourceBarrier(1, &barrier);

			commandList.ptr->SetGraphicsRootSignature(data.rootSignature.ptr);
			commandList.ptr->SetPipelineState(data.pso.ptr);
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

			commandList.ptr->OMSetRenderTargets(1, &data.rtv[data.backBufferIndex].descriptor, false, nullptr);
			float clearColor[4] = { 0.5f, 0.5f, 0.5f, 0.1f };
			commandList.ptr->ClearRenderTargetView(data.rtv[data.backBufferIndex].descriptor, clearColor, 0, nullptr);
			commandList.ptr->IASetVertexBuffers(0, 1, &data.vb.m_VertexBufferView);
			commandList.ptr->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList.ptr->DrawInstanced(3, 1, 0, 0);

			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			commandList.ptr->ResourceBarrier(1, &barrier);

			data.backBufferIndex = 1 - data.backBufferIndex;
		});

	framegraph.Compile();
}
