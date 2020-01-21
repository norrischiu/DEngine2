#include <DECore/DECore.h>
#include <DECore/Input/Keyboard.h>

namespace DE
{
Keyboard::State Keyboard::m_currState;
Keyboard::State Keyboard::m_lastState;

void Keyboard::SetInputKey(uint64_t virtualKey, bool flag)
{
	m_currState.Keys[virtualKey] = flag;
}

void Keyboard::Tick()
{
	m_lastState.Keys = m_currState.Keys;
}

};