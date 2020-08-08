#pragma once

class DE::Renderer;

class SampleModel
{
public:
	SampleModel() = default;
	~SampleModel() = default;

	void Setup(DE::Renderer* renderer);
	void Update(float dt);

private:
	void SetupUI(float dt);

	DE::Renderer* m_pRenderer;
};