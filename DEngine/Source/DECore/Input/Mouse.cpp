#include <DECore/DECore.h>
#include <DECore/Input/Mouse.h>

namespace DE
{
Mouse::State Mouse::m_currState;
Mouse::State Mouse::m_lastState;

void Mouse::SetMousePos(long x, long y)
{
	m_currState.cursorPos[0] = x;
	m_currState.cursorPos[1] = y;
}

void Mouse::SetButton(int virtualKey, bool flag)
{
	m_currState.Buttons[virtualKey] = flag;
}

}