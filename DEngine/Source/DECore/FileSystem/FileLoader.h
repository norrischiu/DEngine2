#pragma once

// Engine
#include <DECore/Container/Vector.h>
#include <DECore/Job/JobFuture.h>

namespace DE
{

struct Job;

class FileLoader final
{
public:
	static void LoadSync(const char* path, Vector<char>& output);
	static JobFuture<Vector<char>> LoadAsync(const char* path);
};
}