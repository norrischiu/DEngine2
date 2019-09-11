#pragma once

// Engine
#include <DECore/DECore.h>
#include <DECore/FileSystem/FileLoader.h>
#include <DECore/Job/JobScheduler.h>
// Cpp
#include <fstream>

namespace DE
{

void FileLoader::LoadSync(const char* path, Vector<char>& output)
{
	std::ifstream fs;
	fs.open(path, std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
	assert(fs);
	output.resize(fs.tellg());
	fs.seekg(0, fs.beg);
	fs.read(output.data(), output.size());
	fs.close();
}

struct LoadFileData
{
	const char* path;
	Vector<char>& output;
};

void LoadFile(void* data)
{
	LoadFileData* pData = reinterpret_cast<LoadFileData*>(data);
	std::ifstream fs;
	fs.open(pData->path, std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
	assert(fs);
	pData->output.resize(fs.tellg());
	fs.seekg(0, fs.beg);
	fs.read(pData->output.data(), pData->output.size());
	fs.close();
}

Job* FileLoader::LoadAsync(const char* path, Vector<char>& output)
{
	Vector<Job::Desc> jobDescs(1);
	LoadFileData* data = new LoadFileData{ path, output };
	Job::Desc desc(&LoadFile, data, nullptr);
	jobDescs[0] = std::move(desc);
	return JobScheduler::Instance()->Run(jobDescs);
}

}