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
	auto cs = FileLoader::LoadAsync("..\\Assets\\Shaders\\ConstructCluster.cs.cso");
	auto lightCullingCs = FileLoader::LoadAsync("..\\Assets\\Shaders\\LightCulling.cs.cso");
	{
		ConstantDefinition constants[] =
		{
			{0, D3D12_SHADER_VISIBILITY_ALL},
			{1, D3D12_SHADER_VISIBILITY_ALL},
		};		
		ReadWriteResourceDefinition readWrite = { 0, 3, D3D12_SHADER_VISIBILITY_ALL };
		ReadOnlyResourceDefinition readOnly = {0, 1, D3D12_SHADER_VISIBILITY_ALL };
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
		uint32_t clusterSize;
		float zNear;
		float zFar;
		uint32_t numSlice;
		uint2 resolution;
		uint2 numCluster;
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
	}
	
	clusteringInfoConstant->perspectiveMatrixInverse = frameData.camera.projection.Inverse();
	clusteringInfoConstant->clusterSize = m_data.clusterSize;
	clusteringInfoConstant->zNear = frameData.camera.zNear;
	clusteringInfoConstant->zFar = frameData.camera.zFar;
	clusteringInfoConstant->numSlice = m_data.numSlice;
	clusteringInfoConstant->resolution = { m_data.resolutionX, m_data.resolutionY };
	clusteringInfoConstant->numCluster = { m_data.resolutionX / m_data.clusterSize,  m_data.resolutionY / m_data.clusterSize };

	const uint32_t numCluster = m_data.resolutionX * m_data.resolutionY / m_data.clusterSize / m_data.clusterSize * m_data.numSlice;

	commandList.SetSignature(&m_rootSignature);
	commandList.SetPipeline(m_clusterFroxelPso);

	commandList.ResourceBarrier(m_data.clusters, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList.SetConstantResource(0, clusteringInfoConstant.GetCurrentResource());
	commandList.SetConstantResource(1, perViewConstants.GetCurrentResource());
	commandList.SetReadWriteResource(0, &ReadWriteResource().Buffer(m_data.clusters).Dimension(D3D12_UAV_DIMENSION_BUFFER).Stride(sizeof(float4) * 2).NumElement(numCluster),1);

	commandList.Dispatch(ceil(m_data.resolutionX / m_data.clusterSize), ceil(m_data.resolutionY / m_data.clusterSize), ceil(m_data.numSlice));

	commandList.ResourceBarrier(m_data.clusters, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	commandList.SetPipeline(m_lightCullingPso);
	ReadWriteResource readWriteResources[] =
	{
		ReadWriteResource().Buffer(m_data.lightIndexList).Dimension(D3D12_UAV_DIMENSION_BUFFER).Stride(sizeof(uint32_t)),
		ReadWriteResource().Buffer(m_data.lightIndexListCounter).Dimension(D3D12_UAV_DIMENSION_BUFFER).Stride(sizeof(uint32_t)).NumElement(1),
		ReadWriteResource().Buffer(m_data.clusterLightInfoList).Dimension(D3D12_UAV_DIMENSION_BUFFER).Stride(sizeof(uint3)).NumElement(numCluster),
	};
	commandList.SetReadOnlyResource(0, &ReadOnlyResource().Buffer(m_data.clusters).Dimension(D3D12_SRV_DIMENSION_BUFFER).Stride(sizeof(float4) * 2).NumElement(numCluster), 1);
	commandList.SetReadWriteResource(0, readWriteResources, ARRAYSIZE(readWriteResources));
	
	commandList.Dispatch(ceil(m_data.resolutionX / m_data.clusterSize), ceil(m_data.resolutionY / m_data.clusterSize), ceil(m_data.numSlice));
}

}
