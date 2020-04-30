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
	Vector<char> positionOnlyVs;
	Vector<char> albedoOnlyPs;
	Job *vsCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\Pbr.vs.cso", vs);
	JobScheduler::Instance()->WaitOnMainThread(vsCounter);
	Job *psCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\Pbr.ps.cso", ps);
	JobScheduler::Instance()->WaitOnMainThread(psCounter);
	Job *texturedPsCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\PbrTextured.ps.cso", texturedPs);
	JobScheduler::Instance()->WaitOnMainThread(texturedPsCounter);
	Job *positionOnlyVsCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\PositionOnly.vs.cso", positionOnlyVs);
	JobScheduler::Instance()->WaitOnMainThread(positionOnlyVsCounter);
	Job *albedoOnlyPsCounter = FileLoader::LoadAsync("..\\Assets\\Shaders\\AlbedoOnly.ps.cso", albedoOnlyPs);
	JobScheduler::Instance()->WaitOnMainThread(albedoOnlyPsCounter);

	{
		ConstantDefinition constants[] =
			{
				{0, D3D12_SHADER_VISIBILITY_VERTEX},
				{1, D3D12_SHADER_VISIBILITY_PIXEL},
				{2, D3D12_SHADER_VISIBILITY_PIXEL},
			};
		ReadOnlyResourceDefinition readOnly[] =
			{
				{0, 5, D3D12_SHADER_VISIBILITY_PIXEL},
				{5, 2, D3D12_SHADER_VISIBILITY_PIXEL},
				{7, 1, D3D12_SHADER_VISIBILITY_PIXEL},
				{8, 2, D3D12_SHADER_VISIBILITY_PIXEL},
			};
		SamplerDefinition samplers[] = {
			{0, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_COMPARISON_FUNC_NEVER},
			{1, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_COMPARISON_FUNC_NEVER},
		};

		m_rootSignature.Add(constants, ARRAYSIZE(constants));
		m_rootSignature.Add(readOnly, ARRAYSIZE(readOnly));
		m_rootSignature.Add(samplers, ARRAYSIZE(samplers));
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
		{
			D3D12_SHADER_BYTECODE VS;
			VS.pShaderBytecode = positionOnlyVs.data();
			VS.BytecodeLength = positionOnlyVs.size();
			desc.VS = VS;

			D3D12_SHADER_BYTECODE PS;
			PS.pShaderBytecode = albedoOnlyPs.data();
			PS.BytecodeLength = albedoOnlyPs.size();
			desc.PS = PS;

			InputLayout inputLayout;
			inputLayout.Add("POSITION", 0, 0, DXGI_FORMAT_R32G32B32_FLOAT);
			desc.InputLayout = inputLayout.desc;

			m_albedoOnlyPso.Init(renderDevice->m_Device, desc);
		}
	}
	{
		m_vsCbv.Init(renderDevice->m_Device, 256);
		m_psCbv.Init(renderDevice->m_Device, 256 * 32);
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
		Vector4 eyePosWS;
		uint32_t numPointLights;
		uint32_t numQuadLights;
		struct
		{
			Vector4 position;
			float3 color;
			float intensity;
		} pointLights[8];
		struct
		{
			float4 vertices[4];
			float3 color;
			float intensity;
		} quadLights[8];
	} perViewCBuffer = {};
	perViewCBuffer.eyePosWS = frameData.cameraPos;
	perViewCBuffer.numPointLights = static_cast<uint32_t>(frameData.pointLights.size());
	for (uint32_t i = 0; i < perViewCBuffer.numPointLights; ++i)
	{
		const auto& light = PointLight::Get(frameData.pointLights[i]);
		perViewCBuffer.pointLights[i].position = light.position;
		memcpy(&perViewCBuffer.pointLights[i].color, light.color.Raw(), sizeof(float3));
		perViewCBuffer.pointLights[i].intensity = light.intensity;
	}	
	perViewCBuffer.numQuadLights = static_cast<uint32_t>(frameData.quadLights.size());
	for (uint32_t i = 0; i < perViewCBuffer.numQuadLights; ++i)
	{
		const auto& light = QuadLight::Get(frameData.quadLights[i]);
		perViewCBuffer.quadLights[i].vertices[0] = float4(light.position + light.vertices[0], 1.0f);
		perViewCBuffer.quadLights[i].vertices[1] = float4(light.position + light.vertices[1], 1.0f);
		perViewCBuffer.quadLights[i].vertices[2] = float4(light.position + light.vertices[2], 1.0f);
		perViewCBuffer.quadLights[i].vertices[3] = float4(light.position + light.vertices[3], 1.0f);
		perViewCBuffer.quadLights[i].color = light.color;
		perViewCBuffer.quadLights[i].intensity = light.intensity;
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
	commandList.SetReadOnlyResource(1, indirects, ARRAYSIZE(indirects), D3D12_SRV_DIMENSION_TEXTURECUBE);
	commandList.SetReadOnlyResource(2, &m_data.BRDFIntegrationMap, 1);
	Texture LTCMaps[] = { m_data.LTCInverseMatrixMap, m_data.LTCNormMap};
	commandList.SetReadOnlyResource(3, LTCMaps, ARRAYSIZE(LTCMaps));


	uint32_t count = 0;
	{
		commandList.SetPipeline(m_pso);
		const auto &meshes = frameData.batcher.Get(MaterialMeshBatcher::Flag::None);
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
			commandList.SetVertexBuffers(vertexBuffers, ARRAYSIZE(vertexBuffers));
			commandList.SetIndexBuffer(mesh.m_Indices);
			commandList.DrawIndexedInstanced(mesh.m_iNumIndices, 1, 0, 0, 0);
		}
	}
	{
		commandList.SetPipeline(m_albedoOnlyPso);
		const auto &meshes = frameData.batcher.Get(MaterialMeshBatcher::Flag::AlbedoOnly);
		for (auto &i : meshes)
		{
			const Mesh &mesh = Mesh::Get(i);
			const auto &material = Material::Get(mesh.m_MaterialID);

			size_t offset = 256 * count;
			m_materialCbv.buffer.Update(&material.albedo, sizeof(MaterialParameter), offset);
			commandList.SetConstant(2, m_materialCbv, offset);

			VertexBuffer vertexBuffers[] = { mesh.m_Vertices };
			commandList.SetVertexBuffers(vertexBuffers, ARRAYSIZE(vertexBuffers));
			commandList.SetIndexBuffer(mesh.m_Indices);
			commandList.DrawIndexedInstanced(mesh.m_iNumIndices, 1, 0, 0, 0);
		}
	}

	m_backBufferIndex = 1 - m_backBufferIndex;
}

} // namespace DE