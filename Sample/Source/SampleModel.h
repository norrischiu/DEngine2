#pragma once

namespace DE
{
class RenderDevice;
class Framegraph;
}

class SampleModel
{
public:
	SampleModel() = delete;

	static void Setup(DE::RenderDevice& renderer, DE::Framegraph& framegraph);
};