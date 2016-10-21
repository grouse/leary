/**
 * @file:   win32_vulkan.cpp
 * @author: Jesper Stefansson (grouse)
 * @email:  jesper.stefansson@gmail.com
 *
 * Copyright (c) 2016 Jesper Stefansson
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

#include "platform_vulkan.h"

#include <Windows.h>

VkResult
vulkan_create_surface(VkInstance instance, 
                      VkSurfaceKHR *surface, 
                      PlatformState platform_state)
{
	VkWin32SurfaceCreateInfoKHR create_info = {
		VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		nullptr,
		0,
		platform_state.win32.hinstance,
		platform_state.win32.hwnd
	};

	return vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, surface);
}
