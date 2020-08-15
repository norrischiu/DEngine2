#include <DERendering/Device/RenderDevice.h>
#include <DERendering/DataType/GraphicsDataType.h>
#include <DERendering/RenderPass/PrecomputeSpecularIBLPass.h>
#include <DERendering/Device/DrawCommandList.h>
#include <DERendering/Device/RenderHelper.h>
#include <DERendering/FrameData/FrameData.h>
#include <DERendering/RenderConstant.h>
#include <DECore/FileSystem/FileLoader.h>

namespace DE
{

bool PrecomputeSpecularIBLPass::Setup(RenderDevice *renderDevice, const Data& data)
{
	auto vs = FileLoader::LoadAsync("..\\Assets\\Shaders\\PositionAsDirection.vs.cso");
	auto ps = FileLoader::LoadAsync("..\\Assets\\Shaders\\PrefilterEnvironmentMap.ps.cso");
	auto fullscreenVs = FileLoader::LoadAsync("..\\Assets\\Shaders\\Fullscreen.vs.cso");
	auto convolutePs = FileLoader::LoadAsync("..\\Assets\\Shaders\\ConvoluteBRDFIntegrationMap.ps.cso");

	{
		ConstantDefinition constants[2] = {
			{0, D3D12_SHADER_VISIBILITY_VERTEX},
			{1, D3D12_SHADER_VISIBILITY_PIXEL}};
		ReadOnlyResourceDefinition readOnly = {0, 1, D3D12_SHADER_VISIBILITY_PIXEL};
		SamplerDefinition sampler = {0, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_COMPARISON_FUNC_NEVER};

		m_rootSignature.Add(constants, 2);
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
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		desc.RasterizerState = rasterizerDesc;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = data.prefilteredEnvMap.m_Desc.Format;
		DXGI_SAMPLE_DESC sampleDesc = {};
		sampleDesc.Count = 1;
		desc.SampleMask = 0xffffff;
		desc.SampleDesc = sampleDesc;
		D3D12_BLEND_DESC blendDesc = {};
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		desc.BlendState = blendDesc;
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
		desc.DepthStencilState = depthStencilDesc;

		m_pso.Init(renderDevice->m_Device, desc);

		{
			desc.RTVFormats[0] = data.LUT.m_Desc.Format;

			D3D12_SHADER_BYTECODE VS;
			auto vsBlob = fullscreenVs.WaitGet();
			VS.pShaderBytecode = vsBlob.data();
			VS.BytecodeLength = vsBlob.size();

			desc.VS = VS;

			InputLayout inputLayout;
			desc.InputLayout = inputLayout.desc;
			// desc.InputLayout = InputLayout::Null().desc;
			D3D12_SHADER_BYTECODE PS;
			auto psBlob = convolutePs.WaitGet();
			PS.pShaderBytecode = psBlob.data();
			PS.BytecodeLength = psBlob.size();
			desc.PS = PS;
			m_convolutePso.Init(renderDevice->m_Device, desc);
		}
	}
	{
		m_cubeVertices.Init(renderDevice->m_Device, sizeof(float3), sizeof(RenderConstant::CubeVertices));
		m_cubeVertices.Update(RenderConstant::CubeVertices, sizeof(RenderConstant::CubeVertices));
	}

	m_data = data;
	m_pDevice = renderDevice;

	return true;
}

void PrecomputeSpecularIBLPass::Execute(DrawCommandList &commandList, const FrameData &frameData)
{
	// constants
	Matrix4 perspective = Matrix4::PerspectiveProjection(PI / 2.0f, 1.0f, 1.0f, 10.0f);
	Matrix4 views[6] = {
		Matrix4::LookAtMatrix(Vector3::Zero, Vector3::UnitX, Vector3::UnitY),
		Matrix4::LookAtMatrix(Vector3::Zero, Vector3::NegativeUnitX, Vector3::UnitY),
		Matrix4::LookAtMatrix(Vector3::Zero, Vector3::UnitY, Vector3::NegativeUnitZ),
		Matrix4::LookAtMatrix(Vector3::Zero, Vector3::NegativeUnitY, Vector3::UnitZ),
		Matrix4::LookAtMatrix(Vector3::Zero, Vector3::UnitZ, Vector3::UnitY),
		Matrix4::LookAtMatrix(Vector3::Zero, Vector3::NegativeUnitZ, Vector3::UnitY),
	};
	struct PerFace
	{
		Matrix4 transform;
	};
	struct PerRoughness
	{
		float2 resolution;
		float roughness;
	};
	struct LUTSize
	{
		float2 size;
	};
	auto perFaceConstants = RenderHelper::AllocateConstant<PerFace>(m_pDevice, 6);
	auto perRougnhessConstants = RenderHelper::AllocateConstant<PerRoughness>(m_pDevice, 6);
	auto LUTSizeConstant = RenderHelper::AllocateConstant<LUTSize>(m_pDevice, 1);

	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// convonlute prefilter environmental map
	commandList.SetSignature(&m_rootSignature);
	commandList.SetPipeline(m_pso);
	commandList.SetReadOnlyResource(0, &ReadOnlyResource().Texture(m_data.cubemap).Dimension(D3D12_SRV_DIMENSION_TEXTURECUBE), 1);
	commandList.SetVertexBuffers(&m_cubeVertices, 1);

	const uint32_t dstWidth = m_data.prefilteredEnvMap.m_Desc.Width;
	const uint32_t dstHeight = m_data.prefilteredEnvMap.m_Desc.Height;
	for (uint32_t i = 0; i < m_data.numRoughness; ++i)
	{
		perRougnhessConstants->roughness = (float)i / m_data.numRoughness;
		perRougnhessConstants->resolution = float2{ static_cast<float>(dstWidth), static_cast<float>(dstHeight) };
		commandList.SetConstantResource(1, perRougnhessConstants.GetCurrentResource());
		++perRougnhessConstants;

		uint32_t width = dstWidth >> i;
		uint32_t height = dstHeight >> i;
		commandList.SetViewportAndScissorRect(0, 0, width, height, 1.0f, 10.0f);

		for (uint32_t j = 0; j < 6; ++j)
		{
			perFaceConstants->transform = views[j] * perspective;
			commandList.SetConstantResource(0, perFaceConstants.GetCurrentResource());
			++perFaceConstants;

			RenderTargetView::Desc rtv{&m_data.prefilteredEnvMap, i, j};
			commandList.SetRenderTargetDepth(&rtv, 1, nullptr);
			commandList.DrawInstanced(36, 1, 0, 0);
		}
		perFaceConstants.Reset();
	}

	commandList.ResourceBarrier(m_data.prefilteredEnvMap, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// convonlute BRDF integration map
	commandList.SetViewportAndScissorRect(0, 0, m_data.LUT.m_Desc.Width, m_data.LUT.m_Desc.Height, 1.0f, 10.0f);
	commandList.SetPipeline(m_convolutePso);

	LUTSizeConstant->size = float2{ 512, 512 };
	commandList.SetConstantResource(1, LUTSizeConstant.GetCurrentResource());

	RenderTargetView::Desc rtv{&m_data.LUT, 0, 0};
	commandList.SetRenderTargetDepth(&rtv, 1, nullptr);
	commandList.DrawInstanced(3, 1, 0, 0);

	commandList.ResourceBarrier(m_data.LUT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

} // namespace DE