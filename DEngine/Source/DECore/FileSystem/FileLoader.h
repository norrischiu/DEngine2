#pragma once

// Engine
#include <DECore/Container/Vector.h>

namespace DE
{

struct Job;

class FileLoader final
{
public:

	static void LoadSync(const char* path, Vector<char>& output);
	static Job* LoadAsync(const char* path, Vector<char>& output);
};
}