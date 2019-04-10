#pragma once

// Engine
#include <DECore/DECore.h>
// Windows
#include <Windows.h>

namespace DE
{

class Mouse
{
	struct State
	{
		void operator=(const State& other)
		{
			cursorPos[0] = other.cursorPos[0];
			cursorPos[1] = other.cursorPos[1];
			Buttons[1] = other.Buttons[1];
			Buttons[2] = other.Buttons[2];
		}
		long cursorPos[2] = { 0 };
		bool Buttons[3] = { false, false, false };
	};

public:

	static void SetMousePos(long x, long y);
	static void SetButton(int virtualKey, bool flag);

private:

	static State m_currState;
	static State m_lastState;
};

};