#include <DERendering/DERendering.h>
#include <DERendering/Device/RenderDevice.h>
#include <DERendering/DataType/GraphicsDataType.h>
#include <DERendering/RenderPass/PrecomputeDiffuseIBLPass.h>
#include <DERendering/Device/DrawCommandList.h>
#include <DERendering/FrameData/FrameData.h>
#include <DERendering/RenderConstant.h>
#include <DECore/FileSystem/FileLoader.h>
#include <DECore/Job/JobScheduler.h>

namespace DE
{

bool PrecomputeDiffuseIBLPass::Setup(RenderDevice* renderDevice, const Texture& equirectangularMap, Texture& cubemap, Texture& irradianceMap)
{
	Vector<char> vs;
	Vector<char> ps;
	Vector<char> convolutePs;
	Job* vsCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\Position.vs.cso", vs);
	JobScheduler::Instance()->WaitOnMainThread(vsCounter);
	Job* psCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\SampleEquirectangular.ps.cso", ps);
	JobScheduler::Instance()->WaitOnMainThread(psCounter);
	Job* convolutePsCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\ConvoluteIrradianceMap.ps.cso", convolutePs);
	JobScheduler::Instance()->WaitOnMainThread(convolutePsCounter);

	{
		ConstantDefinition constant = { 0, D3D12_SHADER_VISIBILITY_VERTEX };
		ReadOnlyResourceDefinition readOnly = { 0, 1, D3D12_SHADER_VISIBILITY_PIXEL };
		SamplerDefinition sampler = { 0, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_TEXTURE_ADDRESS_MODE_WRAP ,D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_COMPARISON_FUNC_NEVER };

		data.rootSignature.Add(&constant, 1);
		data.rootSignature.Add(&readOnly, 1);
		data.rootSignature.Add(&sampler, 1);
		data.rootSignature.Finalize(renderDevice->m_Device);
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
		desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
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
		data.src = equirectangularMap;
		data.dst = cubemap;
		data.irradianceMap = irradianceMap;
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

		struct CBuffer
		{
			Matrix4 transform;
		};
		CBuffer cbuf;
		for (uint32_t i = 0; i < 6; ++i) {
			cbuf.transform = views[i] * perspective;
			data.cbvs[i].Init(renderDevice->m_Device, sizeof(CBuffer));
			data.cbvs[i].buffer.Update(&cbuf, sizeof(CBuffer));
		}
	}

	data.pDevice = renderDevice;

	return true;
}

void PrecomputeDiffuseIBLPass::Execute(DrawCommandList& commandList, const FrameData& frameData)
{
	commandList.SetViewportAndScissorRect(0, 0, data.dst.m_Desc.Width, data.dst.m_Desc.Height, 1.0f, 10.0f);
	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList.SetSignature(&data.rootSignature);
	commandList.SetPipeline(data.pso);

	commandList.SetReadOnlyResource(0, &data.src, 1);
	commandList.SetVertexBuffers(&data.cubeVertices, 1);

	for (uint32_t i = 0; i < 6; ++i)
	{
		commandList.SetConstant(0, data.cbvs[i]);

		RenderTargetView::Desc rtv{ &data.dst, 0, i };
		commandList.SetRenderTargetDepth(&rtv, 1, nullptr);
		commandList.DrawInstanced(36, 1, 0, 0);
	}

	commandList.ResourceBarrier(data.dst, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// convolute
	commandList.SetViewportAndScissorRect(0, 0, data.irradianceMap.m_Desc.Width, data.irradianceMap.m_Desc.Height, 1.0f, 10.0f);
	commandList.SetPipeline(data.convolutePso);
	commandList.SetReadOnlyResource(0, &data.dst, 1, D3D12_SRV_DIMENSION_TEXTURECUBE);
	commandList.SetVertexBuffers(&data.cubeVertices, 1);  // TODO: cache this

	for (uint32_t i = 0; i < 6; ++i)
	{
		commandList.SetConstant(0, data.cbvs[i]);

		RenderTargetView::Desc rtv{ &data.irradianceMap, 0, i };
		commandList.SetRenderTargetDepth(&rtv, 1, nullptr);
		commandList.DrawInstanced(36, 1, 0, 0);
	}
}

}