#include <DERendering/Device/RenderDevice.h>
#include <DERendering/DataType/GraphicsDataType.h>
#include <DERendering/RenderPass/PrefilterAreaLightTexturePass.h>
#include <DERendering/Device/DrawCommandList.h>
#include <DERendering/Device/RenderHelper.h>
#include <DERendering/FrameData/FrameData.h>
#include <DERendering/RenderConstant.h>
#include <DECore/FileSystem/FileLoader.h>

namespace DE
{

bool PrefilterAreaLightTexturePass::Setup(RenderDevice *renderDevice, const Data& data)
{
	auto cs = FileLoader::LoadAsync("..\\Assets\\Shaders\\GaussianBlurHorizontal.cs.cso");
	auto verticalCs = FileLoader::LoadAsync("..\\Assets\\Shaders\\GaussianBlurVertical.cs.cso");

	{
		ConstantDefinition constant = { 0, D3D12_SHADER_VISIBILITY_ALL };
		ReadOnlyResourceDefinition readOnly = { 0, 1, D3D12_SHADER_VISIBILITY_ALL };
		ReadWriteResourceDefinition readWrite = {0, 1, D3D12_SHADER_VISIBILITY_ALL};

		m_rootSignature.Add(&constant, 1);
		m_rootSignature.Add(&readOnly, 1);
		m_rootSignature.Add(&readWrite, 1);
		m_rootSignature.Finalize(renderDevice->m_Device, RootSignature::Type::Compute);
	}
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
		desc.pRootSignature = m_rootSignature.ptr;
		auto blob = cs.WaitGet();
		desc.CS.pShaderBytecode = blob.data();
		desc.CS.BytecodeLength = blob.size();

		m_pso.Init(renderDevice->m_Device, desc);

		{
			auto blob = verticalCs.WaitGet();
			desc.CS.pShaderBytecode = blob.data();
			desc.CS.BytecodeLength = blob.size();
			m_verticalPso.Init(renderDevice->m_Device, desc);
		}
	}

	m_data = data;
	m_pDevice = renderDevice;

	return true;
}

void PrefilterAreaLightTexturePass::Execute(DrawCommandList &commandList, const FrameData &frameData)
{
	// constants
	struct PerMip
	{
		float2 resolution;
		float2 invResolution;
		float standardDeviation;
	};
	auto perMipConstants = RenderHelper::AllocateConstant<PerMip>(m_pDevice, m_data.src.m_Desc.MipLevels);

	commandList.SetSignature(&m_rootSignature);

	for (uint32_t i = 0; i < m_data.src.m_Desc.MipLevels; ++i)
	{
		uint32_t width = m_data.src.m_Desc.Width >> i;
		uint32_t height = m_data.src.m_Desc.Height >> i;

		perMipConstants->resolution = float2{ static_cast<float>(width), static_cast<float>(height) };
		perMipConstants->invResolution = float2{ 1.0f / width, 1.0f / height };
		perMipConstants->standardDeviation = 5.0f;
		commandList.SetConstantResource(0, perMipConstants.GetCurrentResource());
		++perMipConstants;

		// horizontal
		commandList.UnorderedAccessBarrier(m_data.dst);
		commandList.SetReadOnlyResource(0, &ReadOnlyResource().Texture(m_data.src).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D).MipRange(i, i), 1);
		commandList.SetReadWriteResource(0, &ReadWriteResource().Texture(m_data.dst).Dimension(D3D12_UAV_DIMENSION_TEXTURE2D).Mip(i), 1);
		commandList.SetPipeline(m_pso);
		commandList.Dispatch(max(1, width / 256), height, 1);

		// vertical
		commandList.UnorderedAccessBarrier(m_data.dst);
		commandList.SetReadWriteResource(0, &ReadWriteResource().Texture(m_data.dst).Dimension(D3D12_UAV_DIMENSION_TEXTURE2D).Mip(i), 1);
		commandList.SetPipeline(m_verticalPso);
		commandList.Dispatch(width, max(1, height / 256), 1);
	}

	commandList.ResourceBarrier(m_data.dst, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

} // namespace DE