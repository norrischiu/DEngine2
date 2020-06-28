// main.cpp: This file contains the 'main' function. Program execution begins and ends there.

// Window
#include <Windows.h>
#include <d3d12.h>
#include <tchar.h>
#include <DXProgrammableCapture.h>
// Cpp
#include <sstream>
#include <iostream>
#include <chrono>
// Engine
#include <DECore/Memory/MemoryManager.h>
#include <DECore/Job/JobScheduler.h>
#include <DECore/Windows/WindowsMsgHandler.h>
#include <DERendering/Device/RenderDevice.h>
#include <DERendering/Imgui/imgui.h>

#include "SampleModel.h"

using namespace DE;

//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: The window's message handler
//-----------------------------------------------------------------------------
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}
	else if (msg == WM_PAINT)
	{
		ValidateRect(hWnd, NULL);
		return 0;
	}
	else
	{
		InvalidateRect(hWnd, NULL, TRUE);
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

//-----------------------------------------------------------------------------
// Name: wWinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
INT WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, INT)
{
	UNREFERENCED_PARAMETER(hInst);

	// Register the window class
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_CLASSDC;
	wc.lpfnWndProc = MsgProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = _T("namespaceDE");

	RegisterClassEx(&wc);

	// Create the application's window
	HWND hWnd = CreateWindow(wc.lpszClassName,
							 __T("DE Sample"),
							 WS_OVERLAPPEDWINDOW,
							 100,
							 100,
							 1024,
							 768,
							 NULL,
							 NULL,
							 wc.hInstance,
							 NULL);

	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);

	// Memory
	MemoryManager::GetInstance()->ConstructDefaultPool();

	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	uint32_t numThread = max(sysInfo.dwNumberOfProcessors / 2, 1);

	JobScheduler::Instance()->StartUp(numThread);

	RenderDevice::Desc desc = {};
	desc.hWnd_ = hWnd;
	desc.windowWidth_ = 1024;
	desc.windowHeight_ = 768;
	desc.backBufferCount_ = 2;
	RenderDevice *renderDevice = new RenderDevice();
	renderDevice->Init(desc);

	// init imgui setup
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	RECT rect;
	::GetClientRect(hWnd, &rect);
	io.DisplaySize =  ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

	SampleModel sample;
	sample.Setup(renderDevice);

	// Show the window
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	ShowCursor(true);

	/// Timer
	//Timer m_Timer;
	const float FPS = 30.0f;
	float elaspedTime = 0.0f;
	auto start = std::chrono::high_resolution_clock::now();

	// Main loop
	bool bQuit = false;
	while (!bQuit)
	{
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				bQuit = true;
				break;
			}
			WindowsMsgHandler::Handle(msg.message, msg.wParam, msg.lParam);
		}

		// Inside one frame
		auto end = std::chrono::high_resolution_clock::now();
		if (elaspedTime >= 1.0f / FPS)
		{
			// imgui state set
			ImGuiIO &io = ImGui::GetIO();
			if (::GetForegroundWindow() == hWnd)
			{
				RECT rect;
				::GetClientRect(hWnd, &rect);
				io.DisplaySize =  ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
				io.MousePos = ImVec2((float)Mouse::m_currState.cursorPos.x, (float)(Mouse::m_currState.cursorPos.y));
				io.MouseDown[0] = Mouse::m_currState.Buttons[0];
				io.MouseDown[1] = Mouse::m_currState.Buttons[1];
			}
			ImGui::NewFrame();

			// app update
			sample.Update(*renderDevice, elaspedTime);
			static ComPtr<IDXGraphicsAnalysis> ga;
			if (!ga && (DXGIGetDebugInterface1(0, IID_PPV_ARGS(&ga)) == S_OK)) {
				ga->BeginCapture();
				renderDevice->ExecuteAndPresent();
				ga->EndCapture();
			}
			else renderDevice->ExecuteAndPresent();

			elaspedTime = 0.0f;
			start = end;

			// update input state
			Keyboard::Tick();
			Mouse::Tick();
		}

		elaspedTime += std::chrono::duration<float>(end - start).count();
	}

	delete renderDevice;
	renderDevice = nullptr;

	JobScheduler::Instance()->ShutDown();

	UnregisterClass(wc.lpszClassName, wc.hInstance);
	return 0;
}