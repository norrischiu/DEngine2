#include <DERendering/Device/RenderDevice.h>
#include <DERendering/DataType/GraphicsDataType.h>
#include <DERendering/RenderPass/PrecomputeDiffuseIBLPass.h>
#include <DERendering/Device/DrawCommandList.h>
#include <DERendering/Device/RenderHelper.h>
#include <DERendering/FrameData/FrameData.h>
#include <DERendering/RenderConstant.h>
#include <DECore/FileSystem/FileLoader.h>

namespace DE
{

bool PrecomputeDiffuseIBLPass::Setup(RenderDevice* renderDevice, const Data& data)
{
	auto vs = FileLoader::LoadAsync("..\\Assets\\Shaders\\PositionAsDirection.vs.cso");
	auto ps = FileLoader::LoadAsync("..\\Assets\\Shaders\\SampleEquirectangular.ps.cso");
	auto convolutePs = FileLoader::LoadAsync("..\\Assets\\Shaders\\ConvoluteIrradianceMap.ps.cso");

	{
		ConstantDefinition constant = { 0, D3D12_SHADER_VISIBILITY_VERTEX };
		ReadOnlyResourceDefinition readOnly = { 0, 1, D3D12_SHADER_VISIBILITY_PIXEL };
		SamplerDefinition sampler = { 0, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_TEXTURE_ADDRESS_MODE_CLAMP ,D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_COMPARISON_FUNC_NEVER };

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
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		desc.RasterizerState = rasterizerDesc;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = data.cubemap.m_Desc.Format;
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
			desc.RTVFormats[0] = data.irradianceMap.m_Desc.Format;

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

void PrecomputeDiffuseIBLPass::Execute(DrawCommandList& commandList, const FrameData& frameData)
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
	auto perFaceConstants = RenderHelper::AllocateConstant<PerFace>(m_pDevice, 6);

	commandList.SetViewportAndScissorRect(0, 0, static_cast<float>(m_data.cubemap.m_Desc.Width), static_cast<float>(m_data.cubemap.m_Desc.Height), 1.0f, 10.0f);
	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList.SetSignature(&m_rootSignature);
	commandList.SetPipeline(m_pso);

	commandList.SetReadOnlyResource(0, &ReadOnlyResource().Texture(m_data.equirectangularMap).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D), 1);
	commandList.SetVertexBuffers(&m_cubeVertices, 1);

	for (uint32_t i = 0; i < 6; ++i)
	{
		perFaceConstants->transform = views[i] * perspective;
		commandList.SetConstantResource(0, perFaceConstants.GetCurrentResource());
		++perFaceConstants;

		RenderTargetView::Desc rtv{ &m_data.cubemap, 0, i };
		commandList.SetRenderTargetDepth(&rtv, 1, nullptr);
		commandList.DrawInstanced(36, 1, 0, 0);
	}

	commandList.ResourceBarrier(m_data.cubemap, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// convolute
	commandList.SetViewportAndScissorRect(0, 0, static_cast<float>(m_data.irradianceMap.m_Desc.Width), static_cast<float>(m_data.irradianceMap.m_Desc.Height), 1.0f, 10.0f);
	commandList.SetPipeline(m_convolutePso);
	commandList.SetReadOnlyResource(0, &ReadOnlyResource().Texture(m_data.cubemap).Dimension(D3D12_SRV_DIMENSION_TEXTURECUBE), 1);
	commandList.SetVertexBuffers(&m_cubeVertices, 1);  // TODO: cache this
	perFaceConstants.Reset();

	for (uint32_t i = 0; i < 6; ++i)
	{
		commandList.SetConstantResource(0, perFaceConstants.GetCurrentResource());
		++perFaceConstants;

		RenderTargetView::Desc rtv{ &m_data.irradianceMap, 0, i };
		commandList.SetRenderTargetDepth(&rtv, 1, nullptr);
		commandList.DrawInstanced(36, 1, 0, 0);
	}
}

}