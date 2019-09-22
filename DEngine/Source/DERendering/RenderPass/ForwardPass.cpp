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
		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		data.depth.Init(renderDevice.m_Device, 1024, 768, 1, 1, DXGI_FORMAT_D32_FLOAT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &clearValue);
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

	commandList.ResourceBarrier(*data.pDevice->GetBackBuffer(data.backBufferIndex), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	
	commandList.SetViewportAndScissorRect(0, 0, 1024, 768, 0.0f, 1.0f);

	float clearColor[] = { 0.5f, 0.5f, 0.5f, 0.1f };
	commandList.SetRenderTargetDepth(data.pDevice->GetBackBuffer(data.backBufferIndex), 1, clearColor, &data.depth, 1.0f);

	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList.SetSignature(&data.rootSignature);
	commandList.SetPipeline(data.pso);

	commandList.SetConstant(0, data.vsCbv);
	commandList.SetConstant(1, data.psCbv);

	auto& meshes = frameData.batcher.Get();
	for (auto& i : meshes)
	{
		const Mesh& mesh = Meshes::Get(i);
		uint32_t materialID = mesh.m_MaterialID;

		commandList.SetReadOnlyResource(0, Materials::Get(materialID).m_Textures, 5);
		VertexBuffer vertexBuffers[] = { mesh.m_Vertices, mesh.m_Normals, mesh.m_Tangents, mesh.m_TexCoords };
		commandList.SetVertexBuffers(vertexBuffers, 4);
		commandList.SetIndexBuffer(mesh.m_Indices);
		commandList.DrawIndexedInstanced(mesh.m_iNumIndices, 1, 0, 0, 0);
	}

	commandList.ResourceBarrier(*data.pDevice->GetBackBuffer(data.backBufferIndex), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	data.backBufferIndex = 1 - data.backBufferIndex;
}

} // namespace DE