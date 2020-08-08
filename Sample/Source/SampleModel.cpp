// Engine
#include <DECore/FileSystem/FileLoader.h>
#include <DERendering/DataType/GraphicsDataType.h>
#include <DERendering/DataType/LightType.h>
#include <DERendering/Imgui/imgui.h>
#include <DEGame/Renderer/Renderer.h>
#include <DEGame/Loader/SceneLoader.h>
#include <DEGame/Loader/TextureLoader.h>
#include "SampleModel.h"

using namespace DE;

void SampleModel::Setup(Renderer* renderer)
{
	auto& scene = renderer->GetScene();
	SceneLoader sceneLoader(renderer->GetRenderDevice(), "..\\Assets\\");
	sceneLoader.Load("SampleScene", scene);
	
	// Add lights manually
	{
		auto& light = PointLight::Create();
		scene.Add(light);
		light.position = float3{ 0.0f, 5.0f, 0.0f };
		light.color = float3{ 1.0f, 1.0f, 1.0f };
		light.intensity = 1.0f;
		light.enable = true;
	}
	{
		QuadLight& light = QuadLight::Create();
		scene.Add(light);
		light.position = float3{ 0.0f, 3.0f, 5.0f };
		light.width = 4.0f;
		light.height = 4.0f;
		light.color = float3{ 1.0f, 1.0f, 1.0f };
		light.intensity = 1.0f;
		light.enable = true;

		Mesh& mesh = Mesh::Create();
		light.mesh = mesh.Index();
		scene.Add(mesh);
		{
			float posX = light.position.x + light.width / 2.0f;
			float posY = light.position.y + light.height / 2.0f;
			float z = light.position.z;
			float negX = light.position.x - light.width / 2.0f;
			float negY = light.position.y - light.height / 2.0f;
			float vertices[] = {
				negX, posY, z,
				posX, posY, z,
				posX, negY, z,
				negX, negY, z
			};
			mesh.m_Vertices.Init(renderer->GetRenderDevice()->m_Device, sizeof(float3), sizeof(vertices));
			mesh.m_Vertices.Update(vertices, sizeof(vertices));
		}
		{
			float texcoords[] = {
				0.0f, 0.0f, 
				1.0f, 0.0f, 
				1.0f, 1.0f, 
				0.0f, 1.0f 
			};
			mesh.m_TexCoords.Init(renderer->GetRenderDevice()->m_Device, sizeof(float2), sizeof(texcoords));
			mesh.m_TexCoords.Update(texcoords, sizeof(texcoords));
		}
		{
			uint3 indices[] = { {0, 1, 2}, {2, 3, 0} };
			constexpr uint32_t size = 2 * sizeof(uint3);
			mesh.m_Indices.Init(renderer->GetRenderDevice()->m_Device, sizeof(uint32_t), size);
			mesh.m_Indices.Update(indices, size);
			mesh.m_iNumIndices = 6;
		}
		{
			Material& material = Material::Create();
			material.albedo = float3{ light.intensity, light.intensity, light.intensity };
			material.shadingType = ShadingType::AlbedoOnly;
			mesh.m_MaterialID = material.Index();
			Texture areaLightTexture;
			TextureLoader texLoader(renderer->GetRenderDevice());
			texLoader.Load(areaLightTexture, "..\\Assets\\SampleScene\\Textures\\AreaLightTexture.tex", DXGI_FORMAT_R8G8B8A8_UNORM);
			material.m_Textures[0] = areaLightTexture;
		}
	}

	m_pRenderer = renderer;
}

void SampleModel::Update(float dt)
{
	SetupUI(dt);
}

void SampleModel::SetupUI(float dt)
{
	ImGui::Begin("Info");
	ImGui::Text("frame/second: %.4f", 1.0f / dt);
	ImGui::Text("resize: disabled");
	ImGui::End();

	auto& scene = m_pRenderer->GetScene();
	bool active;
	ImGui::Begin("Edit", &active);
	scene.ForEach<Mesh>([&](Mesh& mesh) {
		char name[256];
		sprintf_s(name, "mesh_%i", mesh.Index());
		if (ImGui::CollapsingHeader(name))
		{
			auto &material = Material::Get(mesh.m_MaterialID);
			sprintf_s(name, "mat_%i", mesh.m_MaterialID);
			if (ImGui::TreeNode(name))
			{
				ImGui::SliderFloat3("Albedo", &material.albedo.x, 0.0f, 1.0f);
				ImGui::SliderFloat("Roughness", &material.roughness, 0.0f, 1.0f);
				ImGui::SliderFloat("Metallic", &material.metallic, 0.0f, 1.0f);
				ImGui::SliderFloat("AO", &material.ao, 0.0f, 1.0f);
				ImGui::TreePop();
			}
		}
	});
	scene.ForEach<PointLight>([&](PointLight& light) {
		char name[256];
		sprintf_s(name, "pointLight_%i", light.Index());
		if (ImGui::CollapsingHeader(name))
		{
			ImGui::Checkbox("Enable", &light.enable);
			ImGui::SliderFloat3("Position", &light.position.x, -50.0f, 50.0f);
			ImGui::SliderFloat3("Color", &light.color.x, 0.0f, 1.0f);
			ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 10.0f);
		}
	});
	scene.ForEach<QuadLight>([&](QuadLight& light) {
		char name[256];
		sprintf_s(name, "quadLight_%i", light.Index());
		if (ImGui::CollapsingHeader(name))
		{
			ImGui::Checkbox("Enable", &light.enable);
			ImGui::SliderFloat3("Position", &light.position.x, -50.0f, 50.0f);
			ImGui::SliderFloat("Width", &light.width, 0.0f, 50.0f);
			ImGui::SliderFloat("Height", &light.height, 0.0f, 50.0f);
			ImGui::SliderFloat3("Color", &light.color.x, 0.0f, 1.0f);
			ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 10.0f);

			float posX = light.position.x + light.width / 2.0f;
			float posY = light.position.y + light.height / 2.0f;
			float z = light.position.z;
			float negX = light.position.x - light.width / 2.0f;
			float negY = light.position.y - light.height / 2.0f;
			float vertices[] = {
				negX, posY, z,
				posX, posY, z,
				posX, negY, z,
				negX, negY, z
			};
			Mesh& mesh = Mesh::Get(light.mesh);
			mesh.m_Vertices.Update(vertices, sizeof(vertices));
			Material& material = Material::Get(mesh.m_MaterialID);
			material.albedo = float3{ light.intensity, light.intensity, light.intensity };
		}
	});

	ImGui::End();
}