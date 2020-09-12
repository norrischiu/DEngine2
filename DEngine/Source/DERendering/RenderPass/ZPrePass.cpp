#include <DERendering/DataType/GraphicsDataType.h>
#include <DERendering/DataType/LightType.h>
#include <DERendering/RenderPass/ZPrePass.h>
#include <DERendering/Device/DrawCommandList.h>
#include <DERendering/FrameData/FrameData.h>
#include <DERendering/Device/RenderHelper.h>
#include <DECore/FileSystem/FileLoader.h>

namespace DE
{

void ZPrePass::Setup(RenderDevice *renderDevice, const Data& data)
{
	auto vs = FileLoader::LoadAsync("..\\Assets\\Shaders\\PositionOnly.vs.cso");
	auto ps = FileLoader::LoadAsync("..\\Assets\\Shaders\\MarkCluster.ps.cso");

	{
		ConstantDefinition constants[] =
		{
			{0, D3D12_SHADER_VISIBILITY_VERTEX},
			{0, D3D12_SHADER_VISIBILITY_PIXEL},
		};
		ReadWriteResourceDefinition readWrite[] =
		{
			{0, 1, D3D12_SHADER_VISIBILITY_PIXEL},
		};

		m_rootSignature.Add(constants, ARRAYSIZE(constants));
		m_rootSignature.Add(readWrite, ARRAYSIZE(readWrite));
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
		D3D12_SHADER_BYTECODE PS;
		auto psBlob = ps.WaitGet();
		PS.pShaderBytecode = psBlob.data();
		PS.BytecodeLength = psBlob.size();
		desc.PS = PS;

		InputLayout inputLayout;
		inputLayout.Add("POSITION", 0, 0, DXGI_FORMAT_R32G32B32_FLOAT);
		desc.InputLayout = inputLayout.desc;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		D3D12_RASTERIZER_DESC rasterizerDesc = {};
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		desc.RasterizerState = rasterizerDesc;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		DXGI_SAMPLE_DESC sampleDesc = {};
		sampleDesc.Count = 1;
		desc.SampleMask = 0xffffff;
		desc.SampleDesc = sampleDesc;
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		desc.DepthStencilState = depthStencilDesc;

		m_pso.Init(renderDevice->m_Device, desc);
	}

	m_data = data;
	m_pDevice = renderDevice;
}

void ZPrePass::Execute(DrawCommandList &commandList, const FrameData &frameData)
{
	// constants
	struct ClusteringInfo
	{
		float zNear;
		float zFar;
		uint32_t clusterSize;
		uint32_t numSlice;
		uint2 numCluster;
		float2 resolution;
	};
	const uint32_t totalMeshNum = frameData.batcher.GetTotalNum();
	struct PerObject
	{
		Matrix4 world;
		Matrix4 wvp;
	};
	auto clusteringInfoConstant = RenderHelper::AllocateConstant<ClusteringInfo>(m_pDevice, 1);
	auto perObjectConstants = RenderHelper::AllocateConstant<PerObject>(m_pDevice, totalMeshNum);

	clusteringInfoConstant->numSlice = frameData.clusteringInfo.numSlice;
	clusteringInfoConstant->zNear = frameData.camera.zNear;
	clusteringInfoConstant->zFar = frameData.camera.zFar;
	clusteringInfoConstant->resolution = frameData.clusteringInfo.resolution;
	clusteringInfoConstant->numCluster = frameData.clusteringInfo.numCluster;
	clusteringInfoConstant->clusterSize = frameData.clusteringInfo.clusterSize;

	commandList.SetViewportAndScissorRect(0, 0, 1024, 768, 0.0f, 1.0f);

	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList.SetSignature(&m_rootSignature);
	commandList.SetRenderTargetDepth(nullptr, 0, &m_data.depth, ClearFlag::Depth, nullptr, 1.0f);

	{
		commandList.SetPipeline(m_pso);
		commandList.SetReadWriteResource(0, &ReadWriteResource().Buffer(m_data.clusterVisibilities).Dimension(D3D12_UAV_DIMENSION_BUFFER).Stride(sizeof(uint32_t)), 1);
		commandList.SetConstantResource(1, clusteringInfoConstant.GetCurrentResource());

		auto flags = { MaterialMeshBatcher::Flag::NoNormalMap , MaterialMeshBatcher::Flag::Unlit , MaterialMeshBatcher::Flag::Textured};
		for (const auto& flag : flags)
		{
			const auto &meshes = frameData.batcher.Get(flag);
			for (auto& i : meshes)
			{
				const Mesh &mesh = Mesh::Get(i);
				perObjectConstants->world = Matrix4::Scale(mesh.scale) * Matrix4::Translation(Vector3(mesh.translate.x, mesh.translate.y, mesh.translate.z));
				perObjectConstants->wvp = perObjectConstants->world * frameData.camera.wvp;
				commandList.SetConstantResource(0, perObjectConstants.GetCurrentResource());
				++perObjectConstants;

				VertexBuffer vertexBuffers[] = { mesh.m_Vertices };
				commandList.SetVertexBuffers(vertexBuffers, ARRAYSIZE(vertexBuffers));
				commandList.SetIndexBuffer(mesh.m_Indices);
				commandList.DrawIndexedInstanced(mesh.m_iNumIndices, 1, 0, 0, 0);
			}
		}
	}
}

} // namespace DE