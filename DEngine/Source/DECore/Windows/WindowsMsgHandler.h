#pragma once

// Engine
#include <DECore/DECore.h>
#include <DECore/Input/Keyboard.h>
#include <DECore/Input/Mouse.h>
// Windows
#include <Windowsx.h>
#include <windows.h>
#include <Winuser.h>
#include <stdint.h>

namespace DE
{

struct WindowsMsgHandler
{
	static void Handle(uint32_t msg, WPARAM wParam, LPARAM lParam)
	{
		// translate keyboard/mouse message to engine specific setting
		if (msg == WM_MOUSEMOVE)
		{
			Mouse::SetMousePos(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		else if (msg == WM_LBUTTONDOWN)
		{
			Mouse::SetButton(MK_LBUTTON, true);
		}
		else if (msg == WM_LBUTTONUP)
		{
			Mouse::SetButton(MK_LBUTTON, false);
		}
		else if (msg == WM_RBUTTONDOWN)
		{
			Mouse::SetButton(MK_RBUTTON, true);
		}
		else if (msg == WM_RBUTTONUP)
		{
			Mouse::SetButton(MK_RBUTTON, false);
		}
		//else if (msg >= WM_KEYFIRST && msg <= WM_KEYLAST)
		else if (msg == WM_KEYDOWN)
		{
			switch (wParam)
			{
			case VK_W:
			case VK_S:
			case VK_A:
			case VK_D:
				Keyboard::SetInputKey(wParam, true);
			}
		}
		else if (msg == WM_KEYUP)
		{
			switch (wParam)
			{
			case VK_W:
			case VK_S:
			case VK_A:
			case VK_D:
				Keyboard::SetInputKey(wParam, false);
			}
		}
	}
};

}
