#pragma once

// Engine
#include <DECore/DECore.h>
// Cpp
#include <stdint.h>
#include <bitset>

#define VK_A 0x41
#define VK_D 0x44
#define VK_W 0x57
#define VK_S 0x53

namespace DE
{

class Keyboard
{
public:
	static constexpr uint32_t KEY_NUM = 163;

	struct State
	{
		std::bitset<KEY_NUM> Keys = {};
	};

	static void SetInputKey(uint64_t virtualKey, bool flag);
	static void Tick();

	static State m_currState;
	static State m_lastState;
};

};