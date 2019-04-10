#pragma once

#include <DEGame/Component/Camera.h>

namespace DE
{
class RenderDevice;
class Framegraph;
}

class SampleModel
{
public:
	SampleModel() = default;
	~SampleModel() = default;

	void Setup(DE::RenderDevice& renderer, DE::Framegraph& framegraph);
	void Update(float dt);

private:
	DE::Camera m_Camera;
};