/**
 * @file:   platform_main.h
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

#ifndef LEARY_PLATFORM_MAIN_H
#define LEARY_PLATFORM_MAIN_H

#include <cstring>
#include "core/types.h"

#if defined(__linux__)
    #include <X11/Xlib.h>
    #include <X11/XKBlib.h>
    #include <X11/extensions/XInput2.h>

    #define DL_EXPORT extern "C"

    #define FILE_SEP "/"
    #define FILE_EOL "\n"

    #define VK_USE_PLATFORM_XLIB_KHR

    #define PACKED(decl) decl __attribute__((__packed__))
#elif defined(_WIN32)
    #include <Windows.h>
    #include <Shlobj.h>
    #include <Shlwapi.h>

    #define DL_EXPORT extern "C" __declspec(dllexport)

    #define VK_USE_PLATFORM_WIN32_KHR

    #undef near
    #undef far

    #define FILE_SEP "\\"
    #define FILE_EOL "\r\n"

    #define PACKED(decl) __pragma(pack(push, 1)) decl __pragma(pack(pop))
#else
    #error "unsupported platform"
#endif

#include <vulkan/vulkan.h>

#ifndef INTROSPECT
    #define INTROSPECT
#endif


#ifndef LEARY_DYNAMIC
#define LEARY_DYNAMIC 0
#endif

#if !LEARY_DYNAMIC
    #undef DL_EXPORT
    #define DL_EXPORT
#endif


#include "leary.h"
#include "core/profiling.h"
#include "platform_file.h"

#if defined(__linux__)
    #define PLATFORM_INIT_FUNC(fname)       void fname(PlatformState *platform)
#elif defined(_WIN32)
    #define PLATFORM_INIT_FUNC(fname)       void fname(PlatformState *platform, HINSTANCE instance)
#else
    #error "unsupported platform"
#endif

#define PLATFORM_PRE_RELOAD_FUNC(fname) void  fname(PlatformState *platform)
#define PLATFORM_RELOAD_FUNC(fname)     void  fname(PlatformState *platform)
#define PLATFORM_UPDATE_FUNC(fname)     void  fname(PlatformState *platform, f32 dt)

struct Mutex {
#if defined(__linux__)
    pthread_mutex_t native;
#elif defined(_WIN32)
    HANDLE native;
#else
    #error "unsupported platform"
#endif
};

void init_mutex(Mutex *m);
void lock_mutex(Mutex *m);
void unlock_mutex(Mutex *m);

void platform_quit();
void platform_toggle_raw_mouse();
void platform_set_raw_mouse(bool enable);

#define CATALOG_CALLBACK(fname)  void fname(Path path)
typedef CATALOG_CALLBACK(catalog_callback_t);

void create_catalog_thread(const char *folder, catalog_callback_t *callback);
Array<Path> list_files(const char *folder, Allocator *allocator);

INTROSPECT struct Resolution
{
    i32 width  = 1280;
    i32 height = 720;
};

INTROSPECT struct VideoSettings
{
    Resolution resolution;

    // NOTE: these are integers to later support different fullscreen and vsync techniques
    i16 fullscreen = 0;
    i16 vsync      = 1;
};

INTROSPECT struct Settings
{
    VideoSettings video;
};

struct PlatformState {
    StaticArray<ProfileTimer> profile      = {};
    StaticArray<ProfileTimer> profile_prev = {};

    bool         raw_mouse = false;
    void         *native   = nullptr;

    HeapAllocator   *heap;
    LinearAllocator *frame;
    LinearAllocator *debug_frame;
    LinearAllocator *persistent;
    StackAllocator  *stack;

    void *game_reload_state;

};

#endif // LEARY_PLATFORM_MAIN_H
