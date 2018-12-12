#pragma once

namespace DE
{
class D3D12Renderer;
class Framegraph;
}

class SampleTriangle
{
public:
	SampleTriangle() = delete;

	static void Setup(DE::D3D12Renderer& renderer, DE::Framegraph& framegraph);
};