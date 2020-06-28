#include <DERendering/DERendering.h>
#include <DERendering/RenderPass/SkyboxPass.h>
#include <DERendering/Device/DrawCommandList.h>
#include <DERendering/FrameData/FrameData.h>
#include <DECore/FileSystem/FileLoader.h>
#include <DECore/Job/JobScheduler.h>

namespace DE
{

void SkyboxPass::Setup(RenderDevice* renderDevice, const Texture& depth, const Texture& equirectangularMap)
{
	Vector<char> vs;
	Vector<char> ps;
	Job* vsCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\Skybox.vs.cso", vs);
	JobScheduler::Instance()->WaitOnMainThread(vsCounter);
	Job* psCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\Skybox.ps.cso", ps);
	JobScheduler::Instance()->WaitOnMainThread(psCounter);

	{
		ConstantDefinition constant = { 0, D3D12_SHADER_VISIBILITY_VERTEX };
		ReadOnlyResourceDefinition readOnly = { 0, 1, D3D12_SHADER_VISIBILITY_PIXEL };
		SamplerDefinition sampler = { 0, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_TEXTURE_ADDRESS_MODE_WRAP ,D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_COMPARISON_FUNC_NEVER };

		data.rootSignature.Add(&constant, 1);
		data.rootSignature.Add(&readOnly, 1);
		data.rootSignature.Add(&sampler, 1);
		data.rootSignature.Finalize(renderDevice->m_Device, RootSignature::Type::Graphics);
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
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		desc.DepthStencilState = depthStencilDesc;

		data.pso.Init(renderDevice->m_Device, desc);
	}
	{
		data.depth = depth;
		data.cubemap = equirectangularMap;
	}
	{
		data.cbv.Init(renderDevice->m_Device, 256);
	}

	data.pDevice = renderDevice;
}

void SkyboxPass::Execute(DrawCommandList& commandList, const FrameData& frameData)
{
	struct CBuffer
	{
		Matrix4 projection;
		Matrix4 view;
	};
	CBuffer transforms;
	SIMDMatrix4 invProj(frameData.cameraProjection);
	invProj.Invert();	
	memcpy(&transforms.projection, &invProj, sizeof(Matrix4));
	
	SIMDMatrix4 invView(frameData.cameraView);
	invView.Invert();
	memcpy(&transforms.view, &invView, sizeof(Matrix4));

	data.cbv.buffer.Update(&transforms, sizeof(transforms));

	commandList.SetViewportAndScissorRect(0, 0, 1024, 768, 0.0f, 1.0f);

	RenderTargetView::Desc rtv{ data.pDevice->GetBackBuffer(data.backBufferIndex), 0, 0 };
	commandList.SetRenderTargetDepth(&rtv, 1, &data.depth);

	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList.SetSignature(&data.rootSignature);
	commandList.SetPipeline(data.pso);

	commandList.SetConstant(0, data.cbv);
	commandList.SetReadOnlyResource(0, &ReadOnlyResource().Texture(data.cubemap).Dimension(D3D12_SRV_DIMENSION_TEXTURECUBE), 1);
	commandList.DrawInstanced(3, 1, 0, 0);

	data.backBufferIndex = 1 - data.backBufferIndex;
}

} // namespace DE