#include <DERendering/DERendering.h>
#include <DERendering/DataType/GraphicsDataType.h>
#include <DERendering/DataType/LightType.h>
#include <DERendering/RenderPass/ForwardPass.h>
#include <DERendering/Device/DrawCommandList.h>
#include <DERendering/FrameData/FrameData.h>
#include <DECore/Job/JobScheduler.h>
#include <DECore/FileSystem/FileLoader.h>
#include <DECore/Math/simdmath.h>

namespace DE
{

void ForwardPass::Setup(RenderDevice *renderDevice, const Data& data)
{
	Vector<char> vs;
	Vector<char> ps;
	Vector<char> texturedPs;
	Job *vsCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\Pbr.vs.cso", vs);
	JobScheduler::Instance()->WaitOnMainThread(vsCounter);
	Job *psCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\Pbr.ps.cso", ps);
	JobScheduler::Instance()->WaitOnMainThread(psCounter);
	Job *texturedPsCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\PbrTextured.ps.cso", texturedPs);
	JobScheduler::Instance()->WaitOnMainThread(texturedPsCounter);

	{
		ConstantDefinition constants[3] =
			{
				{0, D3D12_SHADER_VISIBILITY_VERTEX},
				{1, D3D12_SHADER_VISIBILITY_PIXEL},
				{2, D3D12_SHADER_VISIBILITY_PIXEL},
			};
		ReadOnlyResourceDefinition readOnly[3] =
			{
				{0, 5, D3D12_SHADER_VISIBILITY_PIXEL},
				{5, 2, D3D12_SHADER_VISIBILITY_PIXEL},
				{7, 1, D3D12_SHADER_VISIBILITY_PIXEL},
			};
		SamplerDefinition samplers[2] = {
			{0, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_COMPARISON_FUNC_NEVER},
			{1, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_COMPARISON_FUNC_NEVER},
		};

		m_rootSignature.Add(constants, 3);
		m_rootSignature.Add(readOnly, 3);
		m_rootSignature.Add(samplers, 2);
		m_rootSignature.Finalize(renderDevice->m_Device);
	}
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.pRootSignature = m_rootSignature.ptr;
		D3D12_SHADER_BYTECODE VS;
		VS.pShaderBytecode = vs.data();
		VS.BytecodeLength = vs.size();
		desc.VS = VS;

		InputLayout inputLayout;
		inputLayout.Add("POSITION", 0, 0, DXGI_FORMAT_R32G32B32_FLOAT);
		inputLayout.Add("NORMAL", 0, 1, DXGI_FORMAT_R32G32B32_FLOAT);
		inputLayout.Add("TANGENT", 0, 2, DXGI_FORMAT_R32G32B32_FLOAT);
		inputLayout.Add("TEXCOORD", 0, 3, DXGI_FORMAT_R32G32_FLOAT);

		desc.InputLayout = inputLayout.desc;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		D3D12_SHADER_BYTECODE PS;
		PS.pShaderBytecode = ps.data();
		PS.BytecodeLength = ps.size();
		desc.PS = PS;
		D3D12_RASTERIZER_DESC rasterizerDesc = {};
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		desc.RasterizerState = rasterizerDesc;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		DXGI_SAMPLE_DESC sampleDesc = {};
		sampleDesc.Count = 1;
		desc.SampleMask = 0xffffff;
		desc.SampleDesc = sampleDesc;
		D3D12_BLEND_DESC blendDesc = {};
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		desc.BlendState = blendDesc;
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		desc.DepthStencilState = depthStencilDesc;

		m_pso.Init(renderDevice->m_Device, desc);

		{
			D3D12_SHADER_BYTECODE PS;
			PS.pShaderBytecode = texturedPs.data();
			PS.BytecodeLength = texturedPs.size();
			desc.PS = PS;

			m_texturedPso.Init(renderDevice->m_Device, desc);
		}
	}
	{
		m_vsCbv.Init(renderDevice->m_Device, 256);
		m_psCbv.Init(renderDevice->m_Device, 256);
	}
	{
		m_materialCbv.Init(renderDevice->m_Device, 256 * 32);
	}

	m_data = data;
	m_pDevice = renderDevice;
}

void ForwardPass::Execute(DrawCommandList &commandList, const FrameData &frameData)
{
	// update cbuffer
	struct
	{
		Matrix4 world;
		Matrix4 wvp;
	} perObjectCBuffer;
	perObjectCBuffer.world = Matrix4::Identity;
	perObjectCBuffer.wvp = frameData.cameraWVP;
	m_vsCbv.buffer.Update(&perObjectCBuffer, sizeof(perObjectCBuffer));

	struct
	{
		float3 eyePosWS;
		uint32_t numPointLights;
		struct Light
		{
			Vector4 position;
			float3 color;
			float intensity;
		} pointlights[8];
	} perViewCBuffer;
	memcpy(&perViewCBuffer.eyePosWS, frameData.cameraPos.Raw(), sizeof(float3));
	perViewCBuffer.numPointLights = frameData.pointLights.size();
	for (uint32_t i = 0; i < perViewCBuffer.numPointLights; ++i)
	{
		const auto& pointLight = PointLight::Get(frameData.pointLights[i]);
		perViewCBuffer.pointlights[i].position = pointLight.position;
		memcpy(&perViewCBuffer.pointlights[i].color, pointLight.color.Raw(), sizeof(float3));
		perViewCBuffer.pointlights[i].intensity = pointLight.intensity;
	}
	m_psCbv.buffer.Update(&perViewCBuffer, sizeof(perViewCBuffer));

	commandList.ResourceBarrier(*m_pDevice->GetBackBuffer(m_backBufferIndex), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	commandList.SetViewportAndScissorRect(0, 0, 1024, 768, 0.0f, 1.0f);

	float clearColor[] = {0.5f, 0.5f, 0.5f, 0.1f};
	RenderTargetView::Desc rtv{ m_pDevice->GetBackBuffer(m_backBufferIndex), 0, 0};
	commandList.SetRenderTargetDepth(&rtv, 1, &m_data.depth, ClearFlag::Color | ClearFlag::Depth, clearColor, 1.0f);

	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList.SetSignature(&m_rootSignature);

	commandList.SetConstant(0, m_vsCbv);

	commandList.SetConstant(1, m_psCbv);

	Texture indirects[] = { m_data.irradianceMap, m_data.prefilteredEnvMap };
	commandList.SetReadOnlyResource(1, indirects, 2, D3D12_SRV_DIMENSION_TEXTURECUBE);
	commandList.SetReadOnlyResource(2, &m_data.BRDFIntegrationMap, 1);

	{
		commandList.SetPipeline(m_pso);
		const auto &meshes = frameData.batcher.Get(MaterialMeshBatcher::Flag::None);
		uint32_t count = 0;
		for (auto &i : meshes)
		{
			const Mesh &mesh = Mesh::Get(i);
			const auto &material = Material::Get(mesh.m_MaterialID);

			size_t offset = 256 * count;
			m_materialCbv.buffer.Update(&material.albedo, sizeof(MaterialParameter), offset);
			commandList.SetConstant(2, m_materialCbv, offset);

			VertexBuffer vertexBuffers[] = {mesh.m_Vertices, mesh.m_Normals, mesh.m_Tangents, mesh.m_TexCoords};
			commandList.SetVertexBuffers(vertexBuffers, 4);
			commandList.SetIndexBuffer(mesh.m_Indices);
			commandList.DrawIndexedInstanced(mesh.m_iNumIndices, 1, 0, 0, 0);

			count++;
		}
	}
	{
		commandList.SetPipeline(m_texturedPso);

		const auto &meshes = frameData.batcher.Get(MaterialMeshBatcher::Flag::Textured);
		for (auto &i : meshes)
		{
			const Mesh &mesh = Mesh::Get(i);
			uint32_t materialID = mesh.m_MaterialID;

			commandList.SetReadOnlyResource(0, Material::Get(materialID).m_Textures, 5);
			VertexBuffer vertexBuffers[] = {mesh.m_Vertices, mesh.m_Normals, mesh.m_Tangents, mesh.m_TexCoords};
			commandList.SetVertexBuffers(vertexBuffers, 4);
			commandList.SetIndexBuffer(mesh.m_Indices);
			commandList.DrawIndexedInstanced(mesh.m_iNumIndices, 1, 0, 0, 0);
		}
	}

	m_backBufferIndex = 1 - m_backBufferIndex;
}

} // namespace DE