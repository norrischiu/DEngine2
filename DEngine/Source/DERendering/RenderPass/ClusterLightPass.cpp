#include "ClusterLightPass.h"
#include <DERendering/DataType/LightType.h>
#include <DERendering/DataType/GraphicsDataType.h>
#include <DERendering/Device/DrawCommandList.h>
#include <DERendering/FrameData/FrameData.h>
#include <DERendering/Device/RenderHelper.h>
#include <DECore/FileSystem/FileLoader.h>

namespace DE
{
bool ClusterLightPass::Setup(RenderDevice * renderDevice, const Data & data)
{
	m_lightCullingIndirectBuffer.Init(renderDevice->m_Device, sizeof(uint32_t) * 3, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	auto cs = FileLoader::LoadAsync("..\\Assets\\Shaders\\ConstructCluster.cs.cso");
	auto lightCullingCs = FileLoader::LoadAsync("..\\Assets\\Shaders\\LightCulling.cs.cso");
	auto filteringCs = FileLoader::LoadAsync("..\\Assets\\Shaders\\FilterVisibleCluster.cs.cso");
	auto resetCounterCs = FileLoader::LoadAsync("..\\Assets\\Shaders\\ResetClusteringCounter.cs.cso");
	auto prepareIndirectCs = FileLoader::LoadAsync("..\\Assets\\Shaders\\PrepareIndirectForLightCulling.cs.cso");
	{
		ConstantDefinition constants[] =
		{
			{0, D3D12_SHADER_VISIBILITY_ALL},
			{1, D3D12_SHADER_VISIBILITY_ALL},
		};
		ReadWriteResourceDefinition readWrite = { 0, 3, D3D12_SHADER_VISIBILITY_ALL };
		ReadOnlyResourceDefinition readOnly = {0, 3, D3D12_SHADER_VISIBILITY_ALL };
		m_rootSignature.Add(constants, ARRAYSIZE(constants));
		m_rootSignature.Add(&readWrite, 1);
		m_rootSignature.Add(&readOnly, 1);
		m_rootSignature.Finalize(renderDevice->m_Device, RootSignature::Type::Compute);
	}
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
		desc.pRootSignature = m_rootSignature.ptr;
		auto blob = cs.WaitGet();
		desc.CS.pShaderBytecode = blob.data();
		desc.CS.BytecodeLength = blob.size();

		m_clusterFroxelPso.Init(renderDevice->m_Device, desc);
	}	
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
		desc.pRootSignature = m_rootSignature.ptr;
		auto blob = lightCullingCs.WaitGet();
		desc.CS.pShaderBytecode = blob.data();
		desc.CS.BytecodeLength = blob.size();

		m_lightCullingPso.Init(renderDevice->m_Device, desc);
	}	
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
		desc.pRootSignature = m_rootSignature.ptr;
		auto blob = filteringCs.WaitGet();
		desc.CS.pShaderBytecode = blob.data();
		desc.CS.BytecodeLength = blob.size();

		m_filteringPso.Init(renderDevice->m_Device, desc);
	}	
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
		desc.pRootSignature = m_rootSignature.ptr;
		auto blob = resetCounterCs.WaitGet();
		desc.CS.pShaderBytecode = blob.data();
		desc.CS.BytecodeLength = blob.size();

		m_resetCounterPso.Init(renderDevice->m_Device, desc);
	}	
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
		desc.pRootSignature = m_rootSignature.ptr;
		auto blob = prepareIndirectCs.WaitGet();
		desc.CS.pShaderBytecode = blob.data();
		desc.CS.BytecodeLength = blob.size();

		m_prepareIndirectPso.Init(renderDevice->m_Device, desc);
	}

	m_data = data;
	m_pDevice = renderDevice;

	return true;
}

void ClusterLightPass::Execute(DrawCommandList & commandList, const FrameData & frameData)
{
	// constants
	struct ClusteringInfo
	{
		Matrix4 perspectiveMatrixInverse;
		struct
		{
			float zNear;
			float zFar;
			uint32_t clusterSize;
			uint32_t numSlice;
			uint2 numCluster;
			float2 resolution;
		} clusterInfo;
	};
	auto clusteringInfoConstant = RenderHelper::AllocateConstant<ClusteringInfo>(m_pDevice, 1);
	struct PerView
	{
		Matrix4 viewMatrix;
		Vector4 eyePosWS;
		uint32_t numPointLights;
		uint32_t numQuadLights;
		uint32_t padding[2];
		struct
		{
			float3 position;
			float falloffRadius;
			float3 color;
			float intensity;
		} pointLights[8];
		struct
		{
			float4 vertices[4];
			float3 color;
			float intensity;
			float falloffRadius;
			float3 centerWS;
		} quadLights[8];
	};
	auto perViewConstants = RenderHelper::AllocateConstant<PerView>(m_pDevice, 1);

	perViewConstants->viewMatrix = frameData.camera.view;
	const uint32_t numPointLights = static_cast<uint32_t>(frameData.pointLights.size());
	const uint32_t numQuadLights = static_cast<uint32_t>(frameData.quadLights.size());
	perViewConstants->eyePosWS = frameData.camera.pos;
	perViewConstants->numPointLights = numPointLights;
	for (uint32_t i = 0; i < numPointLights; ++i)
	{
		const auto& light = PointLight::Get(frameData.pointLights[i]);
		perViewConstants->pointLights[i].position = light.position;
		perViewConstants->pointLights[i].falloffRadius = light.falloffRadius;
		perViewConstants->pointLights[i].color = light.color;
		perViewConstants->pointLights[i].intensity = light.intensity;
	}
	perViewConstants->numQuadLights = numQuadLights;
	for (uint32_t i = 0; i < numQuadLights; ++i)
	{
		const auto& light = QuadLight::Get(frameData.quadLights[i]);
		float posX = light.position.x + light.width / 2.0f;
		float posY = light.position.y + light.height / 2.0f;
		float z = light.position.z;
		float negX = light.position.x - light.width / 2.0f;
		float negY = light.position.y - light.height / 2.0f;
		perViewConstants->quadLights[i].vertices[0] = float4(negX, posY, z, 1.0f);
		perViewConstants->quadLights[i].vertices[1] = float4(posX, posY, z, 1.0f);
		perViewConstants->quadLights[i].vertices[2] = float4(posX, negY, z, 1.0f);
		perViewConstants->quadLights[i].vertices[3] = float4(negX, negY, z, 1.0f);
		perViewConstants->quadLights[i].color = light.color;
		perViewConstants->quadLights[i].intensity = light.intensity;
		perViewConstants->quadLights[i].falloffRadius = light.falloffRadius;
		perViewConstants->quadLights[i].centerWS = light.position;
	}
	
	clusteringInfoConstant->perspectiveMatrixInverse = frameData.camera.projection.Inverse();
	clusteringInfoConstant->clusterInfo.clusterSize = m_data.clusterSize;
	clusteringInfoConstant->clusterInfo.zNear = frameData.camera.zNear;
	clusteringInfoConstant->clusterInfo.zFar = frameData.camera.zFar;
	clusteringInfoConstant->clusterInfo.numSlice = m_data.numSlice;
	clusteringInfoConstant->clusterInfo.resolution = { static_cast<float>(m_data.resolutionX), static_cast<float>(m_data.resolutionY) };
	clusteringInfoConstant->clusterInfo.numCluster = { m_data.resolutionX / m_data.clusterSize,  m_data.resolutionY / m_data.clusterSize };

	const uint32_t numCluster = m_data.resolutionX * m_data.resolutionY / m_data.clusterSize / m_data.clusterSize * m_data.numSlice;

	commandList.SetSignature(&m_rootSignature);
	uint3 dispatchSize = { 
		static_cast<uint32_t>(ceil(m_data.resolutionX / m_data.clusterSize / 8.0f)), 
		static_cast<uint32_t>(ceil(m_data.resolutionY / m_data.clusterSize / 4.0f)),
		static_cast<uint32_t>(ceil(m_data.numSlice / 8.0f)) 
	};
	
	// construct clusters
	{
		commandList.SetPipeline(m_clusterFroxelPso);
		commandList.ResourceBarrier(m_data.clusters, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList.SetConstantResource(0, clusteringInfoConstant.GetCurrentResource());
		commandList.SetConstantResource(1, perViewConstants.GetCurrentResource());
		commandList.SetReadWriteResource(0, &ReadWriteResource().Buffer(m_data.clusters).Dimension(D3D12_UAV_DIMENSION_BUFFER).Stride(sizeof(float4) * 2).NumElement(numCluster), 1);
		commandList.Dispatch(dispatchSize.x, dispatchSize.y, dispatchSize.z);
		commandList.ResourceBarrier(m_data.clusters, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}

	// reset counters
	{
		commandList.SetPipeline(m_resetCounterPso);
		ReadWriteResource readWriteResources[] =
		{
			ReadWriteResource().Buffer(m_data.visibleClusterCount).Dimension(D3D12_UAV_DIMENSION_BUFFER).Stride(sizeof(uint32_t)).NumElement(1),
			ReadWriteResource().Buffer(m_data.lightIndexListCounter).Dimension(D3D12_UAV_DIMENSION_BUFFER).Stride(sizeof(uint32_t)).NumElement(1),
		};
		commandList.SetReadWriteResource(0, readWriteResources, ARRAYSIZE(readWriteResources));
		commandList.Dispatch(1, 1, 1);
	}

	// filter cluster using depth
	{
		commandList.SetPipeline(m_filteringPso);
		commandList.ResourceBarrier(m_data.depth, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ);
		commandList.SetReadOnlyResource(0, &ReadOnlyResource().Texture(m_data.depth).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D), 1);
		ReadWriteResource readWriteResources[] =
		{
			ReadWriteResource().Buffer(m_data.visibleClusterIndices).Dimension(D3D12_UAV_DIMENSION_BUFFER).Stride(sizeof(uint32_t)),
			ReadWriteResource().Buffer(m_data.visibleClusterCount).Dimension(D3D12_UAV_DIMENSION_BUFFER).Stride(sizeof(uint32_t)).NumElement(1),
			ReadWriteResource().Buffer(m_data.clusterVisibilities).Dimension(D3D12_UAV_DIMENSION_BUFFER).Stride(sizeof(uint32_t)),
		};
		commandList.UnorderedAccessBarrier(m_data.visibleClusterCount);
		commandList.SetReadWriteResource(0, readWriteResources, ARRAYSIZE(readWriteResources));
		commandList.Dispatch(dispatchSize.x, dispatchSize.y, dispatchSize.z);
		commandList.ResourceBarrier(m_data.depth, D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}

	// prepare indirect dispatch argument
	{
		commandList.SetPipeline(m_prepareIndirectPso);
		ReadWriteResource readWriteResources[] =
		{
			ReadWriteResource().Buffer(m_data.visibleClusterCount).Dimension(D3D12_UAV_DIMENSION_BUFFER).Stride(sizeof(uint32_t)).NumElement(1),
			ReadWriteResource().Buffer(m_lightCullingIndirectBuffer).Dimension(D3D12_UAV_DIMENSION_BUFFER).Stride(sizeof(uint32_t)).NumElement(3),
		};
		commandList.UnorderedAccessBarrier(m_data.visibleClusterCount);
		commandList.ResourceBarrier(m_lightCullingIndirectBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList.SetReadWriteResource(0, readWriteResources, ARRAYSIZE(readWriteResources));
		commandList.Dispatch(1, 1, 1);
	}

	// cull light
	{
		commandList.SetPipeline(m_lightCullingPso);
		ReadWriteResource readWriteResources[] =
		{
			ReadWriteResource().Buffer(m_data.lightIndexList).Dimension(D3D12_UAV_DIMENSION_BUFFER).Stride(sizeof(uint32_t)),
			ReadWriteResource().Buffer(m_data.lightIndexListCounter).Dimension(D3D12_UAV_DIMENSION_BUFFER).Stride(sizeof(uint32_t)).NumElement(1),
			ReadWriteResource().Buffer(m_data.clusterLightInfoList).Dimension(D3D12_UAV_DIMENSION_BUFFER).Stride(sizeof(uint3)).NumElement(numCluster),
		};
		ReadOnlyResource readOnlyResources[] =
		{
			ReadOnlyResource().Buffer(m_data.clusters).Dimension(D3D12_SRV_DIMENSION_BUFFER).Stride(sizeof(float4) * 2).NumElement(numCluster),
			ReadOnlyResource().Buffer(m_data.visibleClusterIndices).Dimension(D3D12_SRV_DIMENSION_BUFFER).Stride(sizeof(uint32_t)),
			ReadOnlyResource().Buffer(m_data.visibleClusterCount).Dimension(D3D12_SRV_DIMENSION_BUFFER).Stride(sizeof(uint32_t)).NumElement(1)
		};
		commandList.SetReadOnlyResource(0, readOnlyResources, ARRAYSIZE(readOnlyResources));
		commandList.SetReadWriteResource(0, readWriteResources, ARRAYSIZE(readWriteResources));
		commandList.ResourceBarrier(m_lightCullingIndirectBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		commandList.DispatchIndirect(m_lightCullingIndirectBuffer);
	}
}

}
