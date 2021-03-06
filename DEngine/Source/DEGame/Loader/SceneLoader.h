#pragma once

// Engine
#include <DEGame/DEGame.h>
#include <DEGame/Collection/Scene.h>
// Cpp
#include <string>


namespace DE 
{

class RenderDevice;
class CopyCommandList;

class SceneLoader
{
public:

	SceneLoader(RenderDevice* device, const char* rootPath);
	void Load(const char* sceneName, Scene& scene);

private:

	std::string m_sRootPath;
	RenderDevice* m_pRenderDevice;
};

}