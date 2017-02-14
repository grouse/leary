/**
 * @file:   win32_main.cpp
 * @author: Jesper Stefansson (grouse)
 * @email:  jesper.stefansson@gmail.com
 *
 * Copyright (c) 2015-2016 Jesper Stefansson
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <cstdio>
#include <cstdint>
#include <cinttypes>
#include <cstdlib>

#include "platform_main.h"

#include "win32_debug.cpp"
#include "win32_vulkan.cpp"
#include "win32_file.cpp"

// TODO(jesper): move this into its own translation unit. Eventually i want to be able to compile
// the game into a .dll and load it dynamically in the platform layer, if feature is turned off,
// for live code editing.
#include "leary.cpp"

namespace {
	Settings      settings;
	PlatformState platform_state;
	GameState     game_state;
}

void platform_quit()
{
	_exit(EXIT_SUCCESS);
}


LRESULT CALLBACK
window_proc(HWND   hwnd,
	        UINT   message,
	        WPARAM wparam,
	        LPARAM lparam)
{
	switch (message) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));

		EndPaint(hwnd, &ps);
		break;
	}

	case WM_KEYUP:
	{
		switch (wparam) {
		case VK_UP:
			game_input(&game_state, InputAction_move_player_vertical_end);
			break;
		case VK_DOWN:
			game_input(&game_state, InputAction_move_player_vertical_end);
			break;
		case VK_LEFT:
			game_input(&game_state, InputAction_move_player_horizontal_end);
			break;
		case VK_RIGHT:
			game_input(&game_state, InputAction_move_player_horizontal_end);
			break;
		case 0x57:
			game_input(&game_state, InputAction_move_vertical_end);
			break;
		case 0x53:
			game_input(&game_state, InputAction_move_vertical_end);
			break;
		case 0x41:
			game_input(&game_state, InputAction_move_horizontal_end);
			break;
		case 0x44:
			game_input(&game_state, InputAction_move_horizontal_end);
			break;
		default:
			std::printf("unhandled key release event: %" PRIuPTR "\n", wparam);
			break;
		}
		break;
	}

	case WM_KEYDOWN:
	{
		if (!(lparam & (1 << 30))) {
			switch (wparam) {
			case VK_UP:
				game_input(&game_state, InputAction_move_player_vertical_start, -1.0f);
				break;
			case VK_DOWN:
				game_input(&game_state, InputAction_move_player_vertical_start, -1.0f);
				break;
			case VK_LEFT:
				game_input(&game_state, InputAction_move_player_horizontal_start, -1.0f);
				break;
			case VK_RIGHT:
				game_input(&game_state, InputAction_move_player_horizontal_start, 1.0f);
				break;
			case 0x57:
				game_input(&game_state, InputAction_move_vertical_start, -1.0f);
				break;
			case 0x53:
				game_input(&game_state, InputAction_move_vertical_start, -1.0f);
				break;
			case 0x41:
				game_input(&game_state, InputAction_move_horizontal_start, -1.0f);
				break;
			case 0x44:
				game_input(&game_state, InputAction_move_horizontal_start, 1.0f);
				break;
			case VK_ESCAPE:
				game_quit(&settings, &game_state);
				break;

			default:
				std::printf("unhandled key press event: %" PRIuPTR "\n", wparam);
				break;
			}
			break;
		}
	}

	default:
		std::printf("unhandled event: %d\n", message);
		return DefWindowProc(hwnd, message, wparam, lparam);

	}

	return 0;
}

int WINAPI
WinMain(HINSTANCE instance,
        HINSTANCE /*prev_instance*/,
        LPSTR     /*cmd_line*/,
        int       /*cmd_show*/)
{
	platform_state = {};
	game_state = {};
	settings = {};

	game_load_settings(&settings);

	platform_state.win32.hinstance = instance;

	WNDCLASS wc = {};
	wc.lpfnWndProc   = window_proc;
	wc.hInstance     = instance;
	wc.lpszClassName = "leary";

	RegisterClass(&wc);

	platform_state.win32.hwnd = CreateWindow("leary",
	                                         "leary",
	                                         WS_TILED | WS_VISIBLE,
	                                         0,
	                                         0,
	                                         settings.video.resolution.width,
	                                         settings.video.resolution.height,
	                                         nullptr,
	                                         nullptr,
	                                         instance,
	                                         nullptr);

	if (platform_state.win32.hwnd == nullptr) {
		platform_quit();
	}

	game_init(&settings, &platform_state, &game_state);

	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	LARGE_INTEGER last_time;
	QueryPerformanceCounter(&last_time);

	MSG msg;
	while(true) {
		LARGE_INTEGER current_time;
		QueryPerformanceCounter(&current_time);
		i64 elapsed = current_time.QuadPart - last_time.QuadPart;
		last_time = current_time;

		f32 dt = (f32)elapsed / frequency.QuadPart;


		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT) {
			platform_quit();
		}


		game_update_and_render(&game_state, dt);
	}

	return 0;
}
