#include <DERendering/DERendering.h>
#include <DERendering/RenderPass/ForwardPass.h>
#include <DERendering/Device/DrawCommandList.h>
#include <DERendering/FrameData/FrameData.h>
#include <DECore/FileSystem/FileLoader.h>

namespace DE
{

void ForwardPass::Setup(RenderDevice& renderDevice)
{
	Vector<char> vs;
	Vector<char> ps;
	Job* vsCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\Pbr.vs.cso", vs);
	JobScheduler::Instance()->WaitOnMainThread(vsCounter);
	Job* psCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\Pbr.ps.cso", ps);
	JobScheduler::Instance()->WaitOnMainThread(psCounter);

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
		ConstantDefinition constants[2] = { {0, D3D12_SHADER_VISIBILITY_VERTEX}, {1, D3D12_SHADER_VISIBILITY_PIXEL} };
		ReadOnlyResourceDefinition readOnly = { 0, 5, D3D12_SHADER_VISIBILITY_PIXEL };
		SamplerDefinition sampler = {0, D3D12_TEXTURE_ADDRESS_MODE_WRAP ,D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_COMPARISON_FUNC_NEVER };

		data.rootSignature.Add(constants, 2);
		data.rootSignature.Add(&readOnly, 1);
		data.rootSignature.Add(sampler);
		data.rootSignature.Finalize(renderDevice.m_Device);
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
	}
	{
		data.psCbv.Init(renderDevice.m_Device, 256);
		PerLightCBuffer light;
		light.lightPos = Vector3(1.0f, 1.0f, 0.0f);
		light.eyePos = Vector3(2.0f, 2.0f, -3.0f);
		data.psCbv.buffer.Update(&light, sizeof(light));
	}

	data.pDevice = &renderDevice;
}

void ForwardPass::Execute(DrawCommandList& commandList, const FrameData& frameData)
{
	PerObjectCBuffer transforms;
	transforms.world = Matrix4::Identity;
	transforms.wvp = frameData.cameraWVP;
	data.vsCbv.buffer.Update(&transforms, sizeof(transforms));

	commandList.ResourceBarrier(data.rtv[data.backBufferIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList.SetViewportAndScissorRect(0, 0, 1024, 768, 0.0f, 1.0f);

	commandList.m_CommandList.ptr->OMSetRenderTargets(1, &data.rtv[data.backBufferIndex].descriptor, false, &data.dsv.descriptor);
	float clearColor[4] = { 0.5f, 0.5f, 0.5f, 0.1f };
	commandList.m_CommandList.ptr->ClearRenderTargetView(data.rtv[data.backBufferIndex].descriptor, clearColor, 0, nullptr);
	commandList.m_CommandList.ptr->ClearDepthStencilView(data.dsv.descriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	commandList.m_CommandList.ptr->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList.SetSignature(data.rootSignature);
	commandList.m_CommandList.ptr->SetPipelineState(data.pso.ptr);

	commandList.SetConstant(0, data.vsCbv);
	commandList.SetConstant(1, data.psCbv);

	auto& meshes = frameData.batcher.Get();
	for (auto& i : meshes)
	{
		const Mesh& mesh = Meshes::Get(i);
		uint32_t materialID = mesh.m_MaterialID;

		commandList.SetReadOnlyResource(0, Materials::Get(materialID).m_Textures, 5);
		D3D12_VERTEX_BUFFER_VIEW vertexBuffers[] = { data.position[i].view, data.normal[i].view, data.tangent[i].view, data.texcoord[i].view };
		commandList.m_CommandList.ptr->IASetVertexBuffers(0, 4, vertexBuffers);
		commandList.m_CommandList.ptr->IASetIndexBuffer(&data.ibs[i].view);
		commandList.m_CommandList.ptr->DrawIndexedInstanced(Meshes::Get(i).m_Indices.size() * 3, 1, 0, 0, 0);
	}

	commandList.ResourceBarrier(data.rtv[data.backBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	data.backBufferIndex = 1 - data.backBufferIndex;
}

} // namespace DE