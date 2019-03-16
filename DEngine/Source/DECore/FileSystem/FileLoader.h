#pragma once

#include <DECore/DECore.h>

namespace DE
{
class FileLoader final
{
public:

	static void LoadSync(const char* path, Vector<char>& output);
	static Job* LoadAsync(const char* path, Vector<char>& output);
};
}