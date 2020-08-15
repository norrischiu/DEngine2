#include <DERendering/RenderPass/SkyboxPass.h>
#include <DERendering/Device/DrawCommandList.h>
#include <DERendering/FrameData/FrameData.h>
#include <DERendering/Device/RenderHelper.h>
#include <DECore/FileSystem/FileLoader.h>

namespace DE
{

void SkyboxPass::Setup(RenderDevice* renderDevice, const Data& data)
{
	auto vs = FileLoader::LoadAsync("..\\Assets\\Shaders\\Skybox.vs.cso");
	auto ps = FileLoader::LoadAsync("..\\Assets\\Shaders\\Skybox.ps.cso");

	{
		ConstantDefinition constant = { 0, D3D12_SHADER_VISIBILITY_VERTEX };
		ReadOnlyResourceDefinition readOnly = { 0, 1, D3D12_SHADER_VISIBILITY_PIXEL };
		SamplerDefinition sampler = { 0, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_TEXTURE_ADDRESS_MODE_WRAP ,D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_COMPARISON_FUNC_NEVER };

		m_rootSignature.Add(&constant, 1);
		m_rootSignature.Add(&readOnly, 1);
		m_rootSignature.Add(&sampler, 1);
		m_rootSignature.Finalize(renderDevice->m_Device, RootSignature::Type::Graphics);
	}
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.pRootSignature = m_rootSignature.ptr;
		D3D12_SHADER_BYTECODE VS;
		auto vsBlob = vs.WaitGet();
		VS.pShaderBytecode = vsBlob.data();
		VS.BytecodeLength = vsBlob.size();
		desc.VS = VS;

		InputLayout inputLayout;
		inputLayout.Add("POSITION", 0, 0, DXGI_FORMAT_R32G32B32_FLOAT);

		desc.InputLayout = inputLayout.desc;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		D3D12_SHADER_BYTECODE PS;
		auto psBlob = ps.WaitGet();
		PS.pShaderBytecode = psBlob.data();
		PS.BytecodeLength = psBlob.size();
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

		m_pso.Init(renderDevice->m_Device, desc);
	}

	m_data = data;
	m_pDevice = renderDevice;
}

void SkyboxPass::Execute(DrawCommandList& commandList, const FrameData& frameData)
{
	struct PerView
	{
		Matrix4 projection;
		Matrix4 view;
	};
	auto perViewConstants = RenderHelper::AllocateConstant<PerView>(m_pDevice, 1);
	SIMDMatrix4 invProj(frameData.cameraProjection);
	invProj.Invert();	
	memcpy(&perViewConstants->projection, &invProj, sizeof(Matrix4));
	SIMDMatrix4 invView(frameData.cameraView);
	invView.Invert();
	memcpy(&perViewConstants->view, &invView, sizeof(Matrix4));

	commandList.SetViewportAndScissorRect(0, 0, 1024, 768, 0.0f, 1.0f);

	RenderTargetView::Desc rtv{ m_pDevice->GetBackBuffer(m_backBufferIndex), 0, 0 };
	commandList.SetRenderTargetDepth(&rtv, 1, &m_data.depth);

	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList.SetSignature(&m_rootSignature);
	commandList.SetPipeline(m_pso);

	commandList.SetConstantResource(0, perViewConstants.GetCurrentResource());
	commandList.SetReadOnlyResource(0, &ReadOnlyResource().Texture(m_data.cubemap).Dimension(D3D12_SRV_DIMENSION_TEXTURECUBE), 1);
	commandList.DrawInstanced(3, 1, 0, 0);

	m_backBufferIndex = 1 - m_backBufferIndex;
}

} // namespace DE