#pragma once

#include <d3d12.h>
#include <DERendering/DERendering.h>


namespace DE
{

class DrawCommandList
{
public:

	DrawCommandList() = default;
	~DrawCommandList() = default;

	uint32_t Init(const GraphicsDevice& device);

	void BeginRenderPass();

	void SetSignature(RootSignature& signature);
	void SetViewportAndScissorRect(float x, float y, float width, float height, float zMin, float zMax);
	
	void SetConstant(uint32_t index, const ConstantBufferView& cbv);
	void SetReadOnlyResource(uint32_t index, const ShaderResourceView& srv);
	
	void ResourceBarrier(const RenderTargetView& rtv, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

	void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, uint32_t vertexOffset, uint32_t instanceOffset);

	inline const CommandList& GetCommandList() const
	{
		return m_CommandList;
	}

//private:
	CommandList m_CommandList;
	GraphicsPipelineState* m_pCurrPipeline;
	RootSignature* m_pCurrSignature;
	std::vector<D3D12_RESOURCE_BARRIER> m_Barriers;
};

}