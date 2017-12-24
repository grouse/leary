/**
 * @file:   platform_main.h
 * @author: Jesper Stefansson (grouse)
 * @email:  jesper.stefansson@gmail.com
 *
 * Copyright (c) 2015-2017 Jesper Stefansson
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

#include "build_config.h"
#include "core/types.h"


// -- platform specific includes
#if defined(__linux__)
    #include <X11/Xlib.h>
    #include <X11/XKBlib.h>
    #include <X11/extensions/XInput2.h>

    #define VK_USE_PLATFORM_XLIB_KHR

extern "C" {
    #define EXIT_SUCCESS 0
    void exit(i32);
    char* getenv(const char*);
    char* realpath(const char *path, char *resolved_path);
    void* malloc(size_t);
    void free(void*);

}
#elif defined(_WIN32)
    #include <Windows.h>

    #include <Shlobj.h>
    #include <Shlwapi.h>

    #undef near
    #undef far

    #define VK_USE_PLATFORM_WIN32_KHR
#else
    #error "unsupported platform"
#endif

// -- platform generic includes
#include <cstring>
#include <vulkan/vulkan.h>

#include "core/core.h"
#include "core/file.h"

#include "platform_debug.h"
#include "platform_input.h"

// -- platform generic defines
#ifndef INTROSPECT
#define INTROSPECT
#endif

#define PLATFORM_PRE_RELOAD_FUNC(fname) void  fname(PlatformState *platform)
#define PLATFORM_RELOAD_FUNC(fname)     void  fname(PlatformState *platform)
#define PLATFORM_UPDATE_FUNC(fname)     void  fname(PlatformState *platform, f32 dt)

// -- platform specific defines
#if defined(__linux__)
    #if LEARY_DYNAMIC
        #define DL_EXPORT extern "C"
    #else
        #define DL_EXPORT
    #endif // LEARY_DYNAMIC

    #define FILE_SEP "/"
    #define FILE_EOL "\n"

    #define PACKED(decl) decl __attribute__((__packed__))

    #define PLATFORM_INIT_FUNC(fname)       void fname(PlatformState *platform)
#elif defined(_WIN32)
    #if LEARY_DYNAMIC
        #define DL_EXPORT extern "C" __declspec(dllexport)
    #else
        #define DL_EXPORT
    #endif // LEARY_DYNAMIC

    #define FILE_SEP "\\"
    #define FILE_EOL "\r\n"

    #define PACKED(decl) __pragma(pack(push, 1)) decl __pragma(pack(pop))

    #define PLATFORM_INIT_FUNC(fname)       void fname(PlatformState *platform, HINSTANCE instance)
#else
    #error "unsupported platform"
#endif



// -- platform specific types
#if defined(__linux__)

struct NativePlatformState {
    Window     window;
    Display    *display;
    XkbDescPtr xkb;
    i32        xinput2;
    Cursor     hidden_cursor;

    Atom WM_DELETE_WINDOW;

    struct {
        i32 x;
        i32 y;
    } mouse;
};

typedef pthread_mutex_t NativeMutex;

#elif defined(_WIN32)

struct NativePlatformState {
    HINSTANCE hinstance;
    HWND      hwnd;
};

typedef HANDLE NativeMutex;

#else
    #error "unsupported platform"
#endif



// -- platform generic types
struct PlatformState {
    NativePlatformState native;

    bool raw_mouse = false;

    struct {
        void            *game;

        HeapAllocator   *heap;
        LinearAllocator *frame;
        LinearAllocator *debug_frame;
        LinearAllocator *persistent;
        StackAllocator  *stack;
        SystemAllocator *system_alloc;
    } reload_state;
};

struct Mutex {
    NativeMutex native;
};

// -- functions
void init_mutex(Mutex *m);
void lock_mutex(Mutex *m);
void unlock_mutex(Mutex *m);

void platform_quit();
void platform_toggle_raw_mouse();
void platform_set_raw_mouse(bool enable);

#define CATALOG_CALLBACK(fname)  void fname(Path path)
typedef CATALOG_CALLBACK(catalog_callback_t);

void create_catalog_thread(Array<char*> folders, catalog_callback_t *callback);

#endif // LEARY_PLATFORM_MAIN_H
