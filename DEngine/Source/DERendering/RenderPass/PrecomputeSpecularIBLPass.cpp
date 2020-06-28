#include <DERendering/DERendering.h>
#include <DERendering/Device/RenderDevice.h>
#include <DERendering/DataType/GraphicsDataType.h>
#include <DERendering/RenderPass/PrecomputeSpecularIBLPass.h>
#include <DERendering/Device/DrawCommandList.h>
#include <DERendering/FrameData/FrameData.h>
#include <DERendering/RenderConstant.h>
#include <DECore/FileSystem/FileLoader.h>
#include <DECore/Job/JobScheduler.h>

namespace DE
{

bool PrecomputeSpecularIBLPass::Setup(RenderDevice *renderDevice, const Texture &cubemap, Texture &prefilteredEnvMap, Texture &LUT)
{
	Vector<char> vs;
	Vector<char> ps;
	Vector<char> fullscreenVs;
	Vector<char> convolutePs;
	Job *vsCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\PositionAsDirection.vs.cso", vs);
	JobScheduler::Instance()->WaitOnMainThread(vsCounter);
	Job *psCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\PrefilterEnvironmentMap.ps.cso", ps);
	JobScheduler::Instance()->WaitOnMainThread(psCounter);
	Job *fullscreenVsCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\Fullscreen.vs.cso", fullscreenVs);
	JobScheduler::Instance()->WaitOnMainThread(fullscreenVsCounter);
	Job *convolutePsCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\ConvoluteBRDFIntegrationMap.ps.cso", convolutePs);
	JobScheduler::Instance()->WaitOnMainThread(convolutePsCounter);

	{
		ConstantDefinition constants[2] = {
			{0, D3D12_SHADER_VISIBILITY_VERTEX},
			{1, D3D12_SHADER_VISIBILITY_PIXEL}};
		ReadOnlyResourceDefinition readOnly = {0, 1, D3D12_SHADER_VISIBILITY_PIXEL};
		SamplerDefinition sampler = {0, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_COMPARISON_FUNC_NEVER};

		data.rootSignature.Add(constants, 2);
		data.rootSignature.Add(&readOnly, 1);
		data.rootSignature.Add(&sampler, 1);
		data.rootSignature.Finalize(renderDevice->m_Device, RootSignature::Type::Graphics);
	}
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.pRootSignature = data.rootSignature.ptr;
		D3D12_SHADER_BYTECODE VS;
		VS.pShaderBytecode = vs.data();
		VS.BytecodeLength = vs.size();
		desc.VS = VS;

		InputLayout inputLayout;
		inputLayout.Add("POSITION", 0, 0, DXGI_FORMAT_R32G32B32_FLOAT);

		desc.InputLayout = inputLayout.desc;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		D3D12_SHADER_BYTECODE PS;
		PS.pShaderBytecode = ps.data();
		PS.BytecodeLength = ps.size();
		desc.PS = PS;
		D3D12_RASTERIZER_DESC rasterizerDesc = {};
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		desc.RasterizerState = rasterizerDesc;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = prefilteredEnvMap.m_Desc.Format;
		DXGI_SAMPLE_DESC sampleDesc = {};
		sampleDesc.Count = 1;
		desc.SampleMask = 0xffffff;
		desc.SampleDesc = sampleDesc;
		D3D12_BLEND_DESC blendDesc = {};
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		desc.BlendState = blendDesc;
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
		desc.DepthStencilState = depthStencilDesc;

		data.pso.Init(renderDevice->m_Device, desc);

		{
			desc.RTVFormats[0] = LUT.m_Desc.Format;

			D3D12_SHADER_BYTECODE VS;
			VS.pShaderBytecode = fullscreenVs.data();
			VS.BytecodeLength = fullscreenVs.size();
			desc.VS = VS;

			InputLayout inputLayout;
			desc.InputLayout = inputLayout.desc;
			// desc.InputLayout = InputLayout::Null().desc;
			D3D12_SHADER_BYTECODE PS;
			PS.pShaderBytecode = convolutePs.data();
			PS.BytecodeLength = convolutePs.size();
			desc.PS = PS;
			data.convolutePso.Init(renderDevice->m_Device, desc);
		}
	}
	{
		data.cubeVertices.Init(renderDevice->m_Device, sizeof(float3), sizeof(RenderConstant::CubeVertices));
		data.cubeVertices.Update(RenderConstant::CubeVertices, sizeof(RenderConstant::CubeVertices));
	}
	{
		data.src = cubemap;
		data.dst = prefilteredEnvMap;
		data.LUT = LUT;
	}
	{
		Matrix4 perspective = Matrix4::PerspectiveProjection(PI / 2.0f, 1.0f, 1.0f, 10.0f);
		Matrix4 views[6] = {
			Matrix4::LookAtMatrix(Vector3::Zero, Vector3::UnitX, Vector3::UnitY),
			Matrix4::LookAtMatrix(Vector3::Zero, Vector3::NegativeUnitX, Vector3::UnitY),
			Matrix4::LookAtMatrix(Vector3::Zero, Vector3::UnitY, Vector3::NegativeUnitZ),
			Matrix4::LookAtMatrix(Vector3::Zero, Vector3::NegativeUnitY, Vector3::UnitZ),
			Matrix4::LookAtMatrix(Vector3::Zero, Vector3::UnitZ, Vector3::UnitY),
			Matrix4::LookAtMatrix(Vector3::Zero, Vector3::NegativeUnitZ, Vector3::UnitY),
		};

		struct
		{
			Matrix4 transform;
		} cbuf;
		for (uint32_t i = 0; i < 6; ++i)
		{
			cbuf.transform = views[i] * perspective;
			data.cbvs[i].Init(renderDevice->m_Device, sizeof(cbuf));
			data.cbvs[i].buffer.Update(&cbuf, sizeof(cbuf));
		}
	}
	{
		struct
		{
			float2 resolution;
			float roughness;
		} cbuf;
		for (uint32_t i = 0; i < 5; ++i)
		{
			data.prefilterData[i].Init(renderDevice->m_Device, sizeof(cbuf));
			cbuf.roughness = (float)i / data.numRoughness;
			cbuf.resolution = float2{ static_cast<float>(data.dst.m_Desc.Width), static_cast<float>(data.dst.m_Desc.Height) };
			data.prefilterData[i].buffer.Update(&cbuf, sizeof(cbuf));
		}
	}
	{
		struct CBuffer
		{
			float2 size;
		} cbuf;
		cbuf.size = float2{512, 512};
		data.LUTsize.Init(renderDevice->m_Device, sizeof(CBuffer));
		data.LUTsize.buffer.Update(&cbuf, sizeof(cbuf));
	}

	data.pDevice = renderDevice;

	return true;
}

void PrecomputeSpecularIBLPass::Execute(DrawCommandList &commandList, const FrameData &frameData)
{
	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// convonlute prefilter environmental map
	commandList.SetSignature(&data.rootSignature);
	commandList.SetPipeline(data.pso);
	commandList.SetReadOnlyResource(0, &ReadOnlyResource().Texture(data.src).Dimension(D3D12_SRV_DIMENSION_TEXTURECUBE), 1);
	commandList.SetVertexBuffers(&data.cubeVertices, 1);

	for (uint32_t i = 0; i < data.numRoughness; ++i)
	{
		commandList.SetConstant(1, data.prefilterData[i]);

		uint32_t width = data.dst.m_Desc.Width >> i;
		uint32_t height = data.dst.m_Desc.Height >> i;
		commandList.SetViewportAndScissorRect(0, 0, width, height, 1.0f, 10.0f);

		for (uint32_t j = 0; j < 6; ++j)
		{
			commandList.SetConstant(0, data.cbvs[j]);

			RenderTargetView::Desc rtv{&data.dst, i, j};
			commandList.SetRenderTargetDepth(&rtv, 1, nullptr);
			commandList.DrawInstanced(36, 1, 0, 0);
		}
	}

	commandList.ResourceBarrier(data.dst, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// convonlute BRDF integration map
	commandList.SetViewportAndScissorRect(0, 0, data.LUT.m_Desc.Width, data.LUT.m_Desc.Height, 1.0f, 10.0f);
	commandList.SetPipeline(data.convolutePso);
	commandList.SetConstant(1, data.LUTsize);

	RenderTargetView::Desc rtv{&data.LUT, 0, 0};
	commandList.SetRenderTargetDepth(&rtv, 1, nullptr);
	commandList.DrawInstanced(3, 1, 0, 0);

	commandList.ResourceBarrier(data.LUT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

} // namespace DE