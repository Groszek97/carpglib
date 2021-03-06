﻿#include "Pch.h"
#include "Engine.h"

#include "App.h"
#include "Gui.h"
#include "Input.h"
#include "Physics.h"
#include "Render.h"
#include "ResourceManager.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "WindowsIncludes.h"

//-----------------------------------------------------------------------------
Engine* app::engine;
const Int2 Engine::MIN_WINDOW_SIZE = Int2(800, 600);
const Int2 Engine::DEFAULT_WINDOW_SIZE = Int2(1024, 768);
constexpr int WINDOWED_FLAGS = WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME;
constexpr int FULLSCREEN_FLAGS = WS_POPUP;

//=================================================================================================
Engine::Engine() : initialized(false), shutdown(false), timer(false), hwnd(nullptr), cursor_visible(true), replace_cursor(false), locked_cursor(true),
active(false), activation_point(-1, -1), phy_world(nullptr), title("Window"), force_pos(-1, -1), force_size(-1, -1), hidden_window(false),
wnd_size(DEFAULT_WINDOW_SIZE), client_size(wnd_size)
{
	if(!Logger::GetInstance())
		Logger::SetInstance(new Logger);
	app::gui = new Gui;
	app::input = new Input;
	app::render = new Render;
	app::res_mgr = new ResourceManager;
	app::scene_mgr = new SceneManager;
	app::sound_mgr = new SoundManager;
}

//=================================================================================================
Engine::~Engine()
{
	delete Logger::GetInstance();
}

//=================================================================================================
// Adjust window size to take exact value
void Engine::AdjustWindowSize()
{
	if(!fullscreen)
	{
		Rect rect = Rect::Create(Int2::Zero, wnd_size);
		AdjustWindowRect((RECT*)&rect, WINDOWED_FLAGS, false);
		real_size = rect.Size();
	}
	else
		real_size = wnd_size;
}

//=================================================================================================
// Cleanup engine
void Engine::Cleanup()
{
	Info("Engine: Cleanup.");

	app::app->OnCleanup();

	delete app::input;
	delete app::res_mgr;
	delete app::render;
	delete app::gui;
	delete app::scene_mgr;
	delete app::sound_mgr;

	CustomCollisionWorld::Cleanup(phy_world);
}

//=================================================================================================
// Do pseudo update tick, used to render in update loop
void Engine::DoPseudotick(bool msg_only)
{
	MSG msg = { 0 };
	if(!timer.IsStarted())
		timer.Start();

	while(msg.message != WM_QUIT && PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if(msg_only)
		timer.Tick();
	else
		DoTick(false);
}

//=================================================================================================
// Common part for WindowLoop and DoPseudotick
void Engine::DoTick(bool update_game)
{
	const float dt = timer.Tick();
	assert(dt >= 0.f);

	// calculate fps
	frames++;
	frame_time += dt;
	if(frame_time >= 1.f)
	{
		fps = frames / frame_time;
		frames = 0;
		frame_time = 0.f;
	}

	// update activity state
	bool is_active = IsWindowActive();
	bool was_active = active;
	UpdateActivity(is_active);

	// handle cursor movement
	Int2 mouse_dif = Int2::Zero;
	if(active)
	{
		if(locked_cursor)
		{
			if(replace_cursor)
				replace_cursor = false;
			else if(was_active)
			{
				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(hwnd, &pt);
				mouse_dif = Int2(pt.x, pt.y) - client_size / 2;
			}
			PlaceCursor();
		}
	}
	else if(!locked_cursor && lock_on_focus)
		locked_cursor = true;
	app::input->UpdateMouseDif(mouse_dif);

	// update keyboard shortcuts info
	app::input->UpdateShortcuts();

	// update game
	if(update_game)
		app::app->OnUpdate(dt);
	if(shutdown)
	{
		if(active && locked_cursor)
		{
			Rect rect;
			GetClientRect(hwnd, (RECT*)&rect);
			Int2 wh = rect.Size();
			POINT pt;
			pt.x = int(float(unlock_point.x)*wh.x / wnd_size.x);
			pt.y = int(float(unlock_point.y)*wh.y / wnd_size.y);
			ClientToScreen(hwnd, &pt);
			SetCursorPos(pt.x, pt.y);
		}
		return;
	}
	app::input->UpdateMouseWheel(0);

	app::app->OnDraw();
	app::input->Update();
	app::sound_mgr->Update(dt);
}

//=================================================================================================
bool Engine::IsWindowActive()
{
	HWND foreground = GetForegroundWindow();
	if(foreground != hwnd)
		return false;
	return !IsIconic(hwnd);
}

//=================================================================================================
// Start closing engine
void Engine::Shutdown()
{
	if(!shutdown)
	{
		shutdown = true;
		Info("Engine: Started closing engine...");
	}
}

//=================================================================================================
// Show fatal error
void Engine::FatalError(cstring err)
{
	assert(err);
	ShowError(err, Logger::L_FATAL);
	Shutdown();
}

//=================================================================================================
// Handle windows events
long Engine::HandleEvent(HWND in_hwnd, uint msg, uint wParam, long lParam)
{
	switch(msg)
	{
	// window closed/destroyed
	case WM_CLOSE:
	case WM_DESTROY:
		shutdown = true;
		return 0;

	// window size change
	case WM_SIZE:
		if(wParam == SIZE_MAXIMIZED)
			SetFullscreen(true);
		else if(wParam != SIZE_MINIMIZED)
		{
			if(!in_resize)
			{
				RECT rect = {};
				GetWindowRect((HWND)hwnd, &rect);
				real_size = Int2(rect.right - rect.left, rect.bottom - rect.top);

				GetClientRect((HWND)hwnd, &rect);
				client_size = Int2(rect.right - rect.left, rect.bottom - rect.top);
			}

			app::render->OnChangeResolution();
			if(initialized)
				app::app->OnResize();
		}
		return 0;

	// handle keyboard
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		app::input->Process((Key)wParam, true);
		return 0;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		app::input->Process((Key)wParam, false);
		return 0;

	// handle mouse
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDBLCLK:
		{
			byte key;
			int result;
			bool down = MsgToKey(msg, wParam, key, result);

			if((!locked_cursor || !active) && down && lock_on_focus)
			{
				ShowCursor(false);
				Rect rect;
				GetClientRect(hwnd, (RECT*)&rect);
				Int2 wh = rect.Size();
				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(hwnd, &pt);
				activation_point = Int2(pt.x * wnd_size.x / wh.x, pt.y * wnd_size.y / wh.y);
				PlaceCursor();

				if(active)
					locked_cursor = true;

				return result;
			}

			app::input->Process((Key)key, down);
			return result;
		}

	// close alt+space menu
	case WM_MENUCHAR:
		return MAKELRESULT(0, MNC_CLOSE);

	// handle text input
	case WM_CHAR:
	case WM_SYSCHAR:
		app::gui->OnChar((char)wParam);
		return 0;

	// handle mouse wheel
	case WM_MOUSEWHEEL:
		app::input->UpdateMouseWheel(app::input->GetMouseWheel() + float(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA);
		return 0;
	}

	// return default message
	return DefWindowProc(in_hwnd, msg, wParam, lParam);
}

//=================================================================================================
// Convert message to virtual key
bool Engine::MsgToKey(uint msg, uint wParam, byte& key, int& result)
{
	bool down = false;

	switch(msg)
	{
	default:
		assert(0);
		break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		down = true;
	case WM_LBUTTONUP:
		key = VK_LBUTTON;
		result = 0;
		break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
		down = true;
	case WM_RBUTTONUP:
		key = VK_RBUTTON;
		result = 0;
		break;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
		down = true;
	case WM_MBUTTONUP:
		key = VK_MBUTTON;
		result = 0;
		break;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONDBLCLK:
		down = true;
	case WM_XBUTTONUP:
		key = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2);
		result = TRUE;
		break;
	}

	return down;
}

//=================================================================================================
// Create window
void Engine::InitWindow()
{
	wnd_size = Int2::Max(wnd_size, MIN_WINDOW_SIZE);

	// register window class
	WNDCLASSEX wc = {
		sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
		[](HWND hwnd, uint msg, WPARAM wParam, LPARAM lParam) -> LRESULT { return app::engine->HandleEvent(hwnd, msg, wParam, lParam); },
		0, 0, GetModuleHandle(nullptr), LoadIcon(GetModuleHandle(nullptr), "Icon"), LoadCursor(nullptr, IDC_ARROW), (HBRUSH)GetStockObject(BLACK_BRUSH),
		nullptr, "Krystal", nullptr
	};
	if(!RegisterClassEx(&wc))
		throw Format("Failed to register window class (%d).", GetLastError());

	// create window
	AdjustWindowSize();
	hwnd = CreateWindowEx(0, "Krystal", title.c_str(), fullscreen ? WS_POPUPWINDOW : WS_OVERLAPPEDWINDOW, 0, 0, real_size.x, real_size.y,
		nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
	if(!hwnd)
		throw Format("Failed to create window (%d).", GetLastError());

	// position window
	if(fullscreen)
	{
		fullscreen = false;
		SetFullscreen(true);
	}
	else
	{
		if(force_pos != Int2(-1, -1) || force_size != Int2(-1, -1))
		{
			// set window position from config file
			Rect rect;
			GetWindowRect(hwnd, (RECT*)&rect);
			if(force_pos.x != -1)
				rect.Left() = force_pos.x;
			if(force_pos.y != -1)
				rect.Top() = force_pos.y;
			Int2 size = real_size;
			if(force_size.x != -1)
				size.x = force_size.x;
			if(force_size.y != -1)
				size.y = force_size.y;
			SetWindowPos(hwnd, 0, rect.Left(), rect.Top(), size.x, size.y, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
		}
		else
		{
			// set window at center of screen
			MoveWindow(hwnd,
				(GetSystemMetrics(SM_CXSCREEN) - real_size.x) / 2,
				(GetSystemMetrics(SM_CYSCREEN) - real_size.y) / 2,
				real_size.x, real_size.y, false);
		}
	}

	// show window
	ShowWindow(hwnd, hidden_window ? SW_HIDE : SW_SHOWNORMAL);

	// reset cursor
	replace_cursor = true;
	app::input->UpdateMouseDif(Int2::Zero);
	unlock_point = real_size / 2;

	Info("Engine: Window created.");
}

//=================================================================================================
// Place cursor on window center
void Engine::PlaceCursor()
{
	POINT dest_pt = { client_size.x / 2, client_size.y / 2 };
	ClientToScreen(hwnd, &dest_pt);
	SetCursorPos(dest_pt.x, dest_pt.y);
}

//=================================================================================================
// Change window title
void Engine::SetTitle(cstring title)
{
	assert(title);
	this->title = title;
	if(initialized)
		SetWindowTextA(hwnd, title);
}

//=================================================================================================
// Show/hide cursor
void Engine::ShowCursor(bool _show)
{
	if(IsCursorVisible() != _show)
	{
		::ShowCursor(_show);
		cursor_visible = _show;
	}
}

//=================================================================================================
// Show error
void Engine::ShowError(cstring msg, Logger::Level level)
{
	assert(msg);

	ShowWindow(hwnd, SW_HIDE);
	ShowCursor(true);
	Logger* logger = Logger::GetInstance();
	logger->Log(level, msg);
	logger->Flush();
	MessageBox(nullptr, msg, nullptr, MB_OK | MB_ICONERROR | MB_APPLMODAL);
}

//=================================================================================================
// Initialize and start engine
bool Engine::Start()
{
	// initialize engine
	try
	{
		Init();
	}
	catch(cstring e)
	{
		ShowError(Format("Engine: Failed to initialize engine!\n%s", e), Logger::L_FATAL);
		Cleanup();
		return false;
	}

	// initialize game
	try
	{
		if(!app::app->OnInit())
		{
			Cleanup();
			return false;
		}
	}
	catch(cstring e)
	{
		ShowError(Format("Engine: Failed to initialize app!\n%s", e), Logger::L_FATAL);
		Cleanup();
		return false;
	}

	// loop game
	try
	{
		if(locked_cursor && active)
			PlaceCursor();
		WindowLoop();
	}
	catch(cstring e)
	{
		ShowError(Format("Engine: Game error!\n%s", e));
		Cleanup();
		return false;
	}

	// cleanup
	Cleanup();
	return true;
}

//=================================================================================================
void Engine::Init()
{
	InitWindow();
	app::render->Init();
	app::sound_mgr->Init();
	phy_world = CustomCollisionWorld::Init();
	app::res_mgr->Init();
	app::gui->Init();
	app::scene_mgr->Init();
	initialized = true;
}

//=================================================================================================
// Unlock cursor - show system cursor and allow to move outside of window
void Engine::UnlockCursor(bool lock_on_focus)
{
	this->lock_on_focus = lock_on_focus;
	if(!locked_cursor)
		return;
	locked_cursor = false;

	if(!IsCursorVisible())
	{
		Rect rect;
		GetClientRect(hwnd, (RECT*)&rect);
		Int2 wh = rect.Size();
		POINT pt;
		pt.x = int(float(unlock_point.x)*wh.x / wnd_size.x);
		pt.y = int(float(unlock_point.y)*wh.y / wnd_size.y);
		ClientToScreen(hwnd, &pt);
		SetCursorPos(pt.x, pt.y);
	}

	ShowCursor(true);
}

//=================================================================================================
// Lock cursor when window gets activated
void Engine::LockCursor()
{
	if(locked_cursor)
		return;
	lock_on_focus = true;
	locked_cursor = true;
	if(active)
	{
		ShowCursor(false);
		PlaceCursor();
	}
}

//=================================================================================================
void Engine::HideWindow(bool hide)
{
	if(hide == hidden_window)
		return;
	hidden_window = hide;
	if(initialized)
		ShowWindow(hwnd, hide ? SW_HIDE : SW_SHOWNORMAL);
}

//=================================================================================================
// Update window activity
void Engine::UpdateActivity(bool is_active)
{
	if(is_active == active)
		return;
	active = is_active;
	if(active)
	{
		if(locked_cursor)
		{
			ShowCursor(false);
			PlaceCursor();
		}
	}
	else
	{
		ShowCursor(true);
		app::input->ReleaseKeys();
	}
	app::app->OnFocus(active, activation_point);
	activation_point = Int2(-1, -1);
}

//=================================================================================================
// Main window loop
void Engine::WindowLoop()
{
	MSG msg = { 0 };

	// start timer
	timer.Start();
	frames = 0;
	frame_time = 0.f;
	fps = 0.f;

	while(msg.message != WM_QUIT)
	{
		// handle winapi messages
		if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
			DoTick(true);

		if(shutdown)
			break;
	}
}

//=================================================================================================
void Engine::SetWindowSizeInternal(const Int2& size)
{
	wnd_size = size;
	AdjustWindowSize();
	SetWindowPos(hwnd, HWND_NOTOPMOST, (GetSystemMetrics(SM_CXSCREEN) - real_size.x) / 2, (GetSystemMetrics(SM_CYSCREEN) - real_size.y) / 2,
		real_size.x, real_size.y, SWP_SHOWWINDOW | SWP_DRAWFRAME);
}


//=================================================================================================
void Engine::SetFullscreen(bool fullscreen)
{
	if(this->fullscreen == fullscreen)
		return;
	this->fullscreen = fullscreen;
	if(!hwnd)
		return;
	in_resize = true;
	HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO monitorInfo;
	monitorInfo.cbSize = sizeof(MONITORINFO);
	GetMonitorInfoA(monitor, &monitorInfo);
	Rect& monitorRect = *reinterpret_cast<Rect*>(&monitorInfo.rcMonitor);
	if(fullscreen)
	{
		client_size = monitorRect.Size();
		real_size = client_size;
		SetWindowLong((HWND)hwnd, GWL_STYLE, FULLSCREEN_FLAGS);
		SetWindowPos((HWND)hwnd, HWND_NOTOPMOST,
			monitorRect.Left(), monitorRect.Top(),
			client_size.x, client_size.y,
			SWP_FRAMECHANGED | SWP_NOACTIVATE);
		ShowWindow((HWND)hwnd, SW_MAXIMIZE);
	}
	else
	{
		client_size = wnd_size;
		AdjustWindowSize();
		SetWindowLong((HWND)hwnd, GWL_STYLE, WINDOWED_FLAGS);
		SetWindowPos((HWND)hwnd, HWND_NOTOPMOST,
			monitorRect.Left() + (monitorRect.SizeX() - real_size.x) / 2,
			monitorRect.Top() + (monitorRect.SizeY() - real_size.y) / 2,
			real_size.x, real_size.y,
			SWP_FRAMECHANGED | SWP_NOACTIVATE);
		ShowWindow((HWND)hwnd, SW_NORMAL);
	}
	in_resize = false;
}

//=================================================================================================
void Engine::SetWindowSize(const Int2& size)
{
	if(wnd_size == size)
		return;
	wnd_size = size;
	if(!initialized)
		return;
	if(!fullscreen)
	{
		client_size = size;
		AdjustWindowSize();
		SetWindowPos((HWND)hwnd, HWND_NOTOPMOST,
			(GetSystemMetrics(SM_CXSCREEN) - real_size.x) / 2,
			(GetSystemMetrics(SM_CYSCREEN) - real_size.y) / 2,
			real_size.x, real_size.y,
			SWP_NOACTIVATE);
	}
}
