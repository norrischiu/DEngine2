#pragma once

// Engine
#include <DECore/DECore.h>
// Windows
#include <Windows.h>

#define VK_MOUSE_L 0x00
#define VK_MOUSE_R 0x01

namespace DE
{

class Mouse
{
	struct State
	{
		void operator=(const State& other)
		{
			cursorPos.x = other.cursorPos.x;
			cursorPos.y = other.cursorPos.y;
			Buttons[0] = other.Buttons[0];
			Buttons[1] = other.Buttons[1];
			Buttons[2] = other.Buttons[2];
		}
		struct
		{
			long x;
			long y;
		} cursorPos;
		bool Buttons[3] = { false, false, false };
	};

public:

	static void SetMousePos(long x, long y);
	static void SetButton(int virtualKey, bool flag);
	static void Tick();

	static State m_currState;
	static State m_lastState;
};

};