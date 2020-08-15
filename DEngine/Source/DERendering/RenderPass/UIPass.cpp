#include <DERendering/RenderPass/UIPass.h>
#include <DERendering/Device/DrawCommandList.h>
#include <DERendering/Device/CopyCommandList.h>
#include <DERendering/Device/RenderHelper.h>
#include <DERendering/FrameData/FrameData.h>
#include <DERendering/Imgui/imgui.h>
#include <DECore/FileSystem/FileLoader.h>

namespace DE
{

void UIPass::Setup(RenderDevice* renderDevice, const Data& data)
{
	auto vs = FileLoader::LoadAsync("..\\Assets\\Shaders\\UI.vs.cso");
	auto ps = FileLoader::LoadAsync("..\\Assets\\Shaders\\UI.ps.cso");

	{
		ConstantDefinition constant = { 0, D3D12_SHADER_VISIBILITY_VERTEX };
		ReadOnlyResourceDefinition readOnly = { 0, 1, D3D12_SHADER_VISIBILITY_PIXEL };
		SamplerDefinition sampler = { 0, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_COMPARISON_FUNC_NEVER, 0 };

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
		inputLayout.Add("POSITION", 0, 0, DXGI_FORMAT_R32G32_FLOAT);
		inputLayout.Add("TEXCOORD", 0, 0, DXGI_FORMAT_R32G32_FLOAT);
		inputLayout.Add("COLOR", 0, 0, DXGI_FORMAT_R8G8B8A8_UNORM);

		desc.InputLayout = inputLayout.desc;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		D3D12_SHADER_BYTECODE PS;
		auto psBlob = ps.WaitGet();
		PS.pShaderBytecode = psBlob.data();
		PS.BytecodeLength = psBlob.size();
		desc.PS = PS;
		D3D12_RASTERIZER_DESC rasterizerDesc = {};
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		rasterizerDesc.DepthClipEnable = true;
		desc.RasterizerState = rasterizerDesc;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		DXGI_SAMPLE_DESC sampleDesc = {};
		sampleDesc.Count = 1;
		desc.SampleMask = UINT_MAX;
		desc.SampleDesc = sampleDesc;
		D3D12_BLEND_DESC blendDesc = {};
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		desc.BlendState = blendDesc;
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
        depthStencilDesc.DepthEnable = false;
        depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        depthStencilDesc.StencilEnable = false;
        depthStencilDesc.FrontFace.StencilFailOp = depthStencilDesc.FrontFace.StencilDepthFailOp = depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        depthStencilDesc.BackFace = depthStencilDesc.FrontFace;		
		desc.DepthStencilState = depthStencilDesc;

		m_pso.Init(renderDevice->m_Device, desc);
	}
	{
		m_vertexBuffer.Init(renderDevice->m_Device, sizeof(ImDrawVert), 1024 * 1024);
		m_indexBuffer.Init(renderDevice->m_Device, sizeof(ImDrawIdx), 1024 * 1024);
	}
	{
		ImGuiIO& io = ImGui::GetIO();

		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		m_font.Init(renderDevice->m_Device, width, height, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
		CopyCommandList commandList(renderDevice);
		commandList.Start();
		UINT uploadPitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);

		UploadTextureDesc desc;
		desc.width = width;
		desc.height = height;
		desc.rowPitch = uploadPitch;
		desc.pitch = 1;
		commandList.UploadTexture(pixels, desc, DXGI_FORMAT_R8G8B8A8_UNORM, m_font);

		renderDevice->Submit(&commandList, 1);
		renderDevice->Execute();
		renderDevice->WaitForIdle();
	}

	m_data = data;
	m_pDevice = renderDevice;
}

void UIPass::Execute(DrawCommandList& commandList, const FrameData& frameData)
{
	ImGui::Render();
	auto* imguiData = ImGui::GetDrawData();

	if (!imguiData)
	{
		return;
	}
	if (imguiData->DisplaySize.x <= 0.0f || imguiData->DisplaySize.y <= 0.0f)
	{
		return;
	}

	commandList.SetViewportAndScissorRect(0.0f, 0.0f, imguiData->DisplaySize.x, imguiData->DisplaySize.y, 0.0f, 1.0f);

	struct PerView
	{
		Matrix4 projection;
	};
	auto perViewConstants = RenderHelper::AllocateConstant<PerView>(m_pDevice, 1);

	float L = imguiData->DisplayPos.x;
	float R = imguiData->DisplayPos.x + imguiData->DisplaySize.x;
	float T = imguiData->DisplayPos.y;
	float B = imguiData->DisplayPos.y + imguiData->DisplaySize.y;
	float mvp[4][4] =
	{
		{ 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
		{ 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
		{ 0.0f,         0.0f,           0.5f,       0.0f },
		{ (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
	};
	memcpy(&perViewConstants->projection, mvp, sizeof(mvp));

	RenderTargetView::Desc rtv{ m_pDevice->GetBackBuffer(m_backBufferIndex), 0, 0 };
	commandList.SetRenderTargetDepth(&rtv, 1, nullptr);

	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList.SetSignature(&m_rootSignature);
	commandList.SetPipeline(m_pso);

	commandList.SetVertexBuffers(&m_vertexBuffer, 1);
	commandList.SetIndexBuffer(m_indexBuffer);
	commandList.SetConstantResource(0, perViewConstants.GetCurrentResource());
	commandList.SetReadOnlyResource(0, &ReadOnlyResource().Texture(m_font).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D), 1);

	// Upload vertex/index data into a single contiguous GPU buffer
	void* vtx_resource = m_vertexBuffer.GetMappedPtr();
	void* idx_resource = m_indexBuffer.GetMappedPtr();

	ImDrawVert* vtx_dst = (ImDrawVert*)vtx_resource;
	ImDrawIdx* idx_dst = (ImDrawIdx*)idx_resource;
	for (int n = 0; n < imguiData->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = imguiData->CmdLists[n];
		memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtx_dst += cmd_list->VtxBuffer.Size;
		idx_dst += cmd_list->IdxBuffer.Size;
	}
	
	int global_vtx_offset = 0;
	int global_idx_offset = 0;
	ImVec2 clip_off = imguiData->DisplayPos;
	for (int n = 0; n < imguiData->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = imguiData->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			commandList.SetScissorRect((LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y));
			commandList.DrawIndexedInstanced(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
		}
		global_idx_offset += cmd_list->IdxBuffer.Size;
		global_vtx_offset += cmd_list->VtxBuffer.Size;
	}

	commandList.ResourceBarrier(*m_pDevice->GetBackBuffer(m_backBufferIndex), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	m_backBufferIndex = 1 - m_backBufferIndex;
}

} // namespace DE