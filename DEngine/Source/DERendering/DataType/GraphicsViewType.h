#pragma once

// Window
#include <d3d12.h>
// Engine
#include <DERendering/DataType/GraphicsResourceType.h>

namespace DE
{

struct UnorderedAccessView
{
	ID3D12Resource* resource;
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc;
	D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
};

struct ShaderResourceView
{
	ID3D12Resource* resource;
	D3D12_SHADER_RESOURCE_VIEW_DESC desc;
	D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
};

struct RenderTargetView final
{
	void Init(const GraphicsDevice& device, DescriptorHeap& heap, const Texture& texture)
	{
		D3D12_RENDER_TARGET_VIEW_DESC desc;
		desc.Format = texture.m_Desc.Format;
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;
		desc.Texture2D.PlaneSlice = 0;

		descriptor = heap.m_top;
		heap.m_top.ptr += device.ptr->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		device.ptr->CreateRenderTargetView(texture.ptr, &desc, descriptor);

		resource = texture.ptr;
	}

	ID3D12Resource* resource;
	D3D12_RENDER_TARGET_VIEW_DESC desc;
	D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
};

struct DepthStencilView
{
	ID3D12Resource* resource;
	D3D12_DEPTH_STENCIL_VIEW_DESC desc;
	D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
};

}