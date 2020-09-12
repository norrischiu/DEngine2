#pragma once

// Engine
#include <DERendering/DataType/GraphicsNativeType.h>
#include <DERendering/DataType/GraphicsResourceType.h>

namespace DE
{

class RenderDevice;
class DrawCommandList;
class FrameData;

class ClusterLightPass
{
public:
	struct Data final
	{
		uint32_t resolutionX;
		uint32_t resolutionY;
		uint32_t clusterSize = 64;
		uint32_t numSlice = 16;
		Buffer clusters;
		Buffer lightIndexList;
		Buffer lightIndexListCounter;
		Buffer clusterLightInfoList;
		Texture depth;
		Buffer visibleClusterIndices;
		Buffer visibleClusterCount;
		Buffer clusterVisibilities;
	};

	ClusterLightPass() = default;

	bool Setup(RenderDevice* renderDevice, const Data& data);
	void Execute(DrawCommandList& commandList, const FrameData& frameData);

	const Data& GetData() const
	{
		return m_data;
	}

private:
	Data m_data;
	RenderDevice* m_pDevice;

	ComputePipelineState m_clusterFroxelPso;
	ComputePipelineState m_lightCullingPso;
	ComputePipelineState m_filteringPso;
	ComputePipelineState m_resetCounterPso;
	ComputePipelineState m_prepareIndirectPso;
	RootSignature m_rootSignature;

	Buffer m_lightCullingIndirectBuffer;
};

} // namespace DE