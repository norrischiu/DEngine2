#include <DERendering/DataType/GraphicsDataType.h>
#include <DERendering/DataType/LightType.h>
#include <DERendering/RenderPass/ForwardPass.h>
#include <DERendering/Device/DrawCommandList.h>
#include <DERendering/FrameData/FrameData.h>
#include <DERendering/Device/RenderHelper.h>
#include <DECore/FileSystem/FileLoader.h>

namespace DE
{

void ForwardPass::Setup(RenderDevice *renderDevice, const Data& data)
{
	auto vs = FileLoader::LoadAsync("..\\Assets\\Shaders\\Pbr.vs.cso");
	auto ps = FileLoader::LoadAsync("..\\Assets\\Shaders\\Pbr.ps.cso");
	auto positionOnlyVs = FileLoader::LoadAsync("..\\Assets\\Shaders\\PositionOnly.vs.cso");
	auto noNormalMapPs = FileLoader::LoadAsync("..\\Assets\\Shaders\\Pbr_NoNormalMap.ps.cso");
	auto albedoOnlyPs = FileLoader::LoadAsync("..\\Assets\\Shaders\\AlbedoOnly.ps.cso");

	{
		ConstantDefinition constants[] =
			{
				{0, D3D12_SHADER_VISIBILITY_VERTEX},
				{1, D3D12_SHADER_VISIBILITY_PIXEL},
				{2, D3D12_SHADER_VISIBILITY_PIXEL},
			};
		ReadOnlyResourceDefinition readOnly[] =
			{
				{0, 5, D3D12_SHADER_VISIBILITY_PIXEL}, // material textures
				{5, 2, D3D12_SHADER_VISIBILITY_PIXEL}, // irradiance & prefiltered map
				{7, 1, D3D12_SHADER_VISIBILITY_PIXEL}, // brdf integration map
				{8, 2, D3D12_SHADER_VISIBILITY_PIXEL}, // LTC maps
				{10, 1, D3D12_SHADER_VISIBILITY_PIXEL}, // quadLight texture
				{11, 2, D3D12_SHADER_VISIBILITY_PIXEL}, // clustering buffers
			};
		SamplerDefinition samplers[] = {
			{0, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_COMPARISON_FUNC_NEVER},
			{1, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_COMPARISON_FUNC_NEVER},
		};

		m_rootSignature.Add(constants, ARRAYSIZE(constants));
		m_rootSignature.Add(readOnly, ARRAYSIZE(readOnly));
		m_rootSignature.Add(samplers, ARRAYSIZE(samplers));
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
		inputLayout.Add("NORMAL", 0, 1, DXGI_FORMAT_R32G32B32_FLOAT);
		inputLayout.Add("TANGENT", 0, 2, DXGI_FORMAT_R32G32B32_FLOAT);
		inputLayout.Add("TEXCOORD", 0, 3, DXGI_FORMAT_R32G32_FLOAT);

		desc.InputLayout = inputLayout.desc;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		D3D12_SHADER_BYTECODE PS;
		auto psBlob = ps.WaitGet();
		PS.pShaderBytecode = psBlob.data();
		PS.BytecodeLength = psBlob.size();
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
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
		desc.DepthStencilState = depthStencilDesc;

		m_pso.Init(renderDevice->m_Device, desc);

		{
			D3D12_SHADER_BYTECODE PS;
			auto blob = noNormalMapPs.WaitGet();
			PS.pShaderBytecode = blob.data();
			PS.BytecodeLength = blob.size();
			desc.PS = PS;
			m_noNormalMapPso.Init(renderDevice->m_Device, desc);
		}
		{
			D3D12_SHADER_BYTECODE PS;
			auto blob = albedoOnlyPs.WaitGet();
			PS.pShaderBytecode = blob.data();
			PS.BytecodeLength = blob.size();
			desc.PS = PS;
			m_albedoOnlyPso.Init(renderDevice->m_Device, desc);

			D3D12_RASTERIZER_DESC rasterizerDesc = {};
			rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
			rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
			desc.RasterizerState = rasterizerDesc;
			D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
			depthStencilDesc.DepthEnable = true;
			depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
			desc.DepthStencilState = depthStencilDesc;
			m_wireframePso.Init(renderDevice->m_Device, desc);
		}
	}

	m_data = data;
	m_pDevice = renderDevice;
}

void ForwardPass::Execute(DrawCommandList &commandList, const FrameData &frameData)
{
	// constants
	const uint32_t totalMeshNum = frameData.batcher.GetTotalNum();
	struct PerObject
	{
		Matrix4 world;
		Matrix4 wvp;
	};
	struct PerView
	{
		Matrix4 viewMatrix;
		float3 eyePosWS;
		float pad;
		uint32_t numPointLights;
		uint32_t numQuadLights;
		float2 pad2;
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
	auto perViewConstants = RenderHelper::AllocateConstant<PerView>(m_pDevice, 1);
	auto perObjectConstants = RenderHelper::AllocateConstant<PerObject>(m_pDevice, totalMeshNum);
	auto perMaterialConstants = RenderHelper::AllocateConstant<MaterialParameter>(m_pDevice, totalMeshNum);

	const uint32_t numPointLights = static_cast<uint32_t>(frameData.pointLights.size());
	const uint32_t numQuadLights = static_cast<uint32_t>(frameData.quadLights.size());
	perViewConstants->viewMatrix = frameData.camera.view;
	perViewConstants->eyePosWS = { frameData.camera.pos.GetX(), frameData.camera.pos.GetY(), frameData.camera.pos.GetZ() };
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
	perViewConstants->clusterInfo.numSlice = frameData.clusteringInfo.numSlice;
	perViewConstants->clusterInfo.zNear = frameData.camera.zNear;
	perViewConstants->clusterInfo.zFar = frameData.camera.zFar;
	perViewConstants->clusterInfo.resolution = frameData.clusteringInfo.resolution;
	perViewConstants->clusterInfo.numCluster = frameData.clusteringInfo.numCluster;
	perViewConstants->clusterInfo.clusterSize = frameData.clusteringInfo.clusterSize;

	commandList.ResourceBarrier(*m_pDevice->GetBackBuffer(m_backBufferIndex), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	commandList.SetViewportAndScissorRect(0, 0, 1024, 768, 0.0f, 1.0f);

	float clearColor[] = {0.5f, 0.5f, 0.5f, 0.1f};
	RenderTargetView::Desc rtv{ m_pDevice->GetBackBuffer(m_backBufferIndex), 0, 0};
	commandList.SetRenderTargetDepth(&rtv, 1, &m_data.depth, ClearFlag::Color, clearColor);

	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList.SetSignature(&m_rootSignature);

	commandList.SetConstantResource(1, perViewConstants.GetCurrentResource());

	ReadOnlyResource indirects[] = 
	{ 
		ReadOnlyResource().Texture(m_data.irradianceMap).Dimension(D3D12_SRV_DIMENSION_TEXTURECUBE),
		ReadOnlyResource().Texture(m_data.prefilteredEnvMap).Dimension(D3D12_SRV_DIMENSION_TEXTURECUBE),
	};
	commandList.SetReadOnlyResource(1, indirects, ARRAYSIZE(indirects));
	commandList.SetReadOnlyResource(2, &ReadOnlyResource().Texture(m_data.BRDFIntegrationMap).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D), 1);
	ReadOnlyResource LTCMaps[] = 
	{ 
		ReadOnlyResource().Texture(m_data.LTCInverseMatrixMap).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D),
		ReadOnlyResource().Texture(m_data.LTCNormMap).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D),
	};
	commandList.SetReadOnlyResource(3, LTCMaps, ARRAYSIZE(LTCMaps));
	commandList.SetReadOnlyResource(4, &ReadOnlyResource().Texture(m_data.filteredAreaLightTexture).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D).UseTextureMipRange(), 1);
	ReadOnlyResource clusterBuffers[] =
	{
		ReadOnlyResource().Buffer(m_data.clusterLightInfoList).Dimension(D3D12_SRV_DIMENSION_BUFFER).Stride(sizeof(uint3)),
		ReadOnlyResource().Buffer(m_data.lightIndexList).Dimension(D3D12_SRV_DIMENSION_BUFFER).Stride(sizeof(uint32_t)),
	};
	commandList.SetReadOnlyResource(5, clusterBuffers, ARRAYSIZE(clusterBuffers));

	{
		commandList.SetPipeline(m_noNormalMapPso);
		const auto &meshes = frameData.batcher.Get(MaterialMeshBatcher::Flag::NoNormalMap);
		for (auto &i : meshes)
		{
			const Mesh &mesh = Mesh::Get(i);
			const Material& material = Material::Get(mesh.m_MaterialID);

			ReadOnlyResource materialTextures[] =
			{
				ReadOnlyResource().Texture(material.m_Textures[0].ptr == nullptr ? Texture::WHITE : material.m_Textures[0]).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D).UseTextureMipRange(),
				ReadOnlyResource().Texture(material.m_Textures[1].ptr == nullptr ? Texture::WHITE : material.m_Textures[1]).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D).UseTextureMipRange(),
				ReadOnlyResource().Texture(material.m_Textures[2].ptr == nullptr ? Texture::WHITE : material.m_Textures[2]).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D).UseTextureMipRange(),
				ReadOnlyResource().Texture(material.m_Textures[3].ptr == nullptr ? Texture::WHITE : material.m_Textures[3]).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D).UseTextureMipRange(),
				ReadOnlyResource().Texture(material.m_Textures[4].ptr == nullptr ? Texture::WHITE : material.m_Textures[4]).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D).UseTextureMipRange(),
			};
			commandList.SetReadOnlyResource(0, materialTextures, ARRAYSIZE(materialTextures));

			perObjectConstants->world = Matrix4::Scale(mesh.scale) * Matrix4::Translation(Vector3(mesh.translate.x, mesh.translate.y, mesh.translate.z));
			perObjectConstants->wvp = perObjectConstants->world * frameData.camera.wvp;
			commandList.SetConstantResource(0, perObjectConstants.GetCurrentResource());
			++perObjectConstants;

			perMaterialConstants->params[0] = float4(material.albedo, material.metallic);
			perMaterialConstants->params[1] = float4(material.roughness, material.ao, 0, 0);
			commandList.SetConstantResource(2, perMaterialConstants.GetCurrentResource());
			++perMaterialConstants;

			VertexBuffer vertexBuffers[] = { mesh.m_Vertices, mesh.m_Normals, mesh.m_Tangents, mesh.m_TexCoords };
			commandList.SetVertexBuffers(vertexBuffers, ARRAYSIZE(vertexBuffers));
			commandList.SetIndexBuffer(mesh.m_Indices);
			commandList.DrawIndexedInstanced(mesh.m_iNumIndices, 1, 0, 0, 0);
		}
	}
	{
		commandList.SetPipeline(m_pso);

		const auto &meshes = frameData.batcher.Get(MaterialMeshBatcher::Flag::Textured);
		for (auto &i : meshes)
		{
			const Mesh &mesh = Mesh::Get(i);
			const Material& material = Material::Get(mesh.m_MaterialID);

			ReadOnlyResource materialTextures[] =
			{
				ReadOnlyResource().Texture(material.m_Textures[0].ptr == nullptr ? Texture::WHITE : material.m_Textures[0]).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D).UseTextureMipRange(),
				ReadOnlyResource().Texture(material.m_Textures[1].ptr == nullptr ? Texture::WHITE : material.m_Textures[1]).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D).UseTextureMipRange(),
				ReadOnlyResource().Texture(material.m_Textures[2].ptr == nullptr ? Texture::WHITE : material.m_Textures[2]).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D).UseTextureMipRange(),
				ReadOnlyResource().Texture(material.m_Textures[3].ptr == nullptr ? Texture::WHITE : material.m_Textures[3]).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D).UseTextureMipRange(),
				ReadOnlyResource().Texture(material.m_Textures[4].ptr == nullptr ? Texture::WHITE : material.m_Textures[4]).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D).UseTextureMipRange(),
			};
			commandList.SetReadOnlyResource(0, materialTextures, ARRAYSIZE(materialTextures));

			perObjectConstants->world = Matrix4::Scale(mesh.scale) * Matrix4::Translation(Vector3(mesh.translate.x, mesh.translate.y, mesh.translate.z));
			perObjectConstants->wvp = perObjectConstants->world * frameData.camera.wvp;
			commandList.SetConstantResource(0, perObjectConstants.GetCurrentResource());
			++perObjectConstants;

			perMaterialConstants->params[0] = float4(material.albedo, material.metallic);
			perMaterialConstants->params[1] = float4(material.roughness, material.ao, 0, 0);
			commandList.SetConstantResource(2, perMaterialConstants.GetCurrentResource());
			++perMaterialConstants;

			VertexBuffer vertexBuffers[] = {mesh.m_Vertices, mesh.m_Normals, mesh.m_Tangents, mesh.m_TexCoords};
			commandList.SetVertexBuffers(vertexBuffers, ARRAYSIZE(vertexBuffers));
			commandList.SetIndexBuffer(mesh.m_Indices);
			commandList.DrawIndexedInstanced(mesh.m_iNumIndices, 1, 0, 0, 0);
		}
	}
	{
		commandList.SetPipeline(m_albedoOnlyPso);
		const auto &meshes = frameData.batcher.Get(MaterialMeshBatcher::Flag::Unlit);
		for (auto &i : meshes)
		{
			const Mesh &mesh = Mesh::Get(i);
			const auto &material = Material::Get(mesh.m_MaterialID);

			perObjectConstants->world = Matrix4::Scale(mesh.scale) * Matrix4::Translation(Vector3(mesh.translate.x, mesh.translate.y, mesh.translate.z));
			perObjectConstants->wvp = perObjectConstants->world * frameData.camera.wvp;
			commandList.SetConstantResource(0, perObjectConstants.GetCurrentResource());
			++perObjectConstants;

			perMaterialConstants->params[0] = float4(material.albedo, material.metallic);
			perMaterialConstants->params[1] = float4(material.roughness, material.ao, 0, 0);
			commandList.SetConstantResource(2, perMaterialConstants.GetCurrentResource());
			++perMaterialConstants;

			ReadOnlyResource materialTextures[] =
			{
				ReadOnlyResource().Texture(material.m_Textures[0].ptr == nullptr ? Texture::WHITE : material.m_Textures[0]).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D).UseTextureMipRange(),
			};
			commandList.SetReadOnlyResource(0, materialTextures, ARRAYSIZE(materialTextures));

			VertexBuffer vertexBuffers[] = { mesh.m_Vertices, mesh.m_Normals, mesh.m_Tangents, mesh.m_TexCoords };
			commandList.SetVertexBuffers(vertexBuffers, ARRAYSIZE(vertexBuffers));
			commandList.SetIndexBuffer(mesh.m_Indices);
			commandList.DrawIndexedInstanced(mesh.m_iNumIndices, 1, 0, 0, 0);
		}
	}
	{
		commandList.SetPipeline(m_wireframePso);
		const auto &meshes = frameData.batcher.Get(MaterialMeshBatcher::Flag::Wireframe);
		for (auto &i : meshes)
		{
			const Mesh &mesh = Mesh::Get(i);

			perObjectConstants->world = Matrix4::Scale(mesh.scale) * Matrix4::Translation(Vector3(mesh.translate.x, mesh.translate.y, mesh.translate.z));
			perObjectConstants->wvp = perObjectConstants->world * frameData.camera.wvp;
			commandList.SetConstantResource(0, perObjectConstants.GetCurrentResource());
			++perObjectConstants;

			perMaterialConstants->params[0] = float4(float3{ 1.0f, 0.0f, 0.0f }, 0.0f);
			commandList.SetConstantResource(2, perMaterialConstants.GetCurrentResource());
			++perMaterialConstants;

			commandList.SetReadOnlyResource(0, &ReadOnlyResource().Texture(Texture::WHITE).Dimension(D3D12_SRV_DIMENSION_TEXTURE2D), 1);

			commandList.SetVertexBuffers(&mesh.m_Vertices, 1);
			commandList.SetIndexBuffer(mesh.m_Indices);
			commandList.DrawIndexedInstanced(mesh.m_iNumIndices, 1, 0, 0, 0);
		}
	}

	m_backBufferIndex = 1 - m_backBufferIndex;
}

} // namespace DE