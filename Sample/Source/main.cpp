// main.cpp: This file contains the 'main' function. Program execution begins and ends there.

// Window
#include <Windows.h>
#include <d3d12.h>
// Cpp
#include <sstream>
#include <iostream>
#include <chrono>
// Engine
#include <DECore/DECore.h>
#include <DECore/Windows/WindowsMsgHandler.h>
#include <DERendering/DERendering.h>

#include "SampleModel.h"

using namespace DE;

constexpr const char* WINDOW_CLASS_NAME = "DESample";

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
	WNDCLASSEX wc =
	{
		sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
		WINDOW_CLASS_NAME, NULL
	};

	RegisterClassEx(&wc);

	// Create the application's window
	HWND hWnd = CreateWindow(WINDOW_CLASS_NAME, WINDOW_CLASS_NAME,
		WS_OVERLAPPEDWINDOW, 0, 0, 1024, 768,
		NULL, NULL, wc.hInstance, NULL);

	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);

	// Memory
	MemoryManager::GetInstance()->ConstructDefaultPool();

	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	uint32_t numThread = sysInfo.dwNumberOfProcessors;

	JobScheduler::Instance()->StartUp(numThread);

	RenderDevice::Desc desc = {};
	desc.hWnd_ = hWnd;
	desc.windowWidth_ = 1024;
	desc.windowHeight_ = 768;
	desc.backBufferCount_ = 2;
	RenderDevice* renderDevice = new RenderDevice();
	renderDevice->Init(desc);

	SampleModel sample;
	sample.Setup(*renderDevice);

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
			sample.Update(*renderDevice, elaspedTime);
			renderDevice->ExecuteAndPresent();

			elaspedTime = 0.0f;
			start = end;
		}

		elaspedTime += std::chrono::duration<float>(end - start).count();
	}

	delete renderDevice;
	renderDevice = nullptr;

	JobScheduler::Instance()->ShutDown();

	UnregisterClass(WINDOW_CLASS_NAME, wc.hInstance);
	return 0;
}