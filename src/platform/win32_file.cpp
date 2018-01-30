/**
 * @file:   win32_file.cpp
 * @author: Jesper Stefansson (grouse)
 * @email:  jesper.stefansson@gmail.com
 *
 * Copyright (c) 2016-2017 Jesper Stefansson
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

#include "core/file.h"

#include "core/maths.h"
#include "platform.h"
#include "leary_macros.h"

extern SystemAllocator *g_system_alloc;

struct PlatformPaths {
    String preferences;
    String data;
    String exe;
    String shaders;
    String textures;
    String models;
};

PlatformPaths g_paths;

void init_paths(Allocator *a)
{
    g_paths = {};

    HRESULT result;
    WCHAR buffer[MAX_PATH];

    // --- exe dir
    // NOTE(jesper): other paths depend on this being resolved early
    DWORD module_length = GetModuleFileNameW(NULL, buffer, MAX_PATH);

    if (module_length != 0) {
        if (PathRemoveFileSpecW(buffer) == TRUE)
            module_length = (DWORD) wcslen(buffer);

        buffer[module_length] = '\0';
        g_paths.exe = create_string(a, buffer, "\\");
    }

    // --- app data dir
    // NOTE(jesper): other paths depend on this being resolved early
    DWORD env_length = GetEnvironmentVariableW(L"LEARY_DATA_ROOT", buffer, MAX_PATH);

    if (env_length != 0) {
        buffer[env_length-1] = '\0';
        g_paths.data = create_string(a, buffer);
    } else {
        g_paths.data = create_string(a, g_paths.exe, "..\\assets\\");
    }

    // --- app preferences dir
    result = SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buffer);
    if (result == S_OK) {
        g_paths.preferences = create_string(a, buffer, "\\leary\\");
    }

    g_paths.shaders  = create_string(a, g_paths.exe, "data\\shaders\\");
    g_paths.textures = create_string(a, g_paths.data, "textures\\");
    g_paths.models   = create_string(a, g_paths.data, "models\\");
}

Array<FilePath> list_files(FolderPath folder, Allocator *allocator)
{
    Array<FilePath> files = {};
    files.allocator   = allocator;

    // TODO(jesper): ROBUSTNESS: better path length
    char path[2048];
    snprintf(path, sizeof path, "%s\\*.*", folder.absolute.bytes);

    bool eslash = (folder[folder.absolute.size-1] == '\\');

    HANDLE h = NULL;
    WIN32_FIND_DATA fd;
    h = FindFirstFile(path, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        LOG(Log_error, "could not find file in folder: %s", folder);
        return {};
    }

    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) {
            continue;
        }
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // TODO(jesper): handle sub-folders
            LOG(Log_warning, "sub-folders are unimplemented");
        } else {
            FilePath p = {};
            if (!eslash) {
                p = create_file_path(allocator, folder, "\\", fd.cFileName);
            } else {
                p = create_file_path(allocator, folder, fd.cFileName);
            }

            LOG("adding file: %s", p.absolute.bytes);
            array_add(&files, p);
        }
    } while (FindNextFile(h, &fd));

    FindClose(h);
    return files;
}

char* resolve_relative(const char *path)
{
    DWORD result;

    char buffer[MAX_PATH];
    result = GetFullPathName(path, MAX_PATH, buffer, nullptr);
    ASSERT(result > 0);

    // TODO(jesper): currently leaking memory
    return strdup(buffer);
}

FilePath resolve_file_path(GamePath rp, StringView path, Allocator *a)
{
    String root;

    switch (rp) {
    case GamePath_data:
        root = g_paths.data;
        break;
    case GamePath_exe:
        root = g_paths.exe;
        break;
    case GamePath_shaders:
        root = g_paths.shaders;
        break;
    case GamePath_textures:
        root = g_paths.textures;
        break;
    case GamePath_models:
        root = g_paths.models;
        break;
    case GamePath_preferences:
        root = g_paths.preferences;
        break;
    default:
        LOG(Log_error, "unknown path root: %d", rp);
        ASSERT(false);
        return {};
    }

    FilePath resolved = create_file_path(a, root, path);
    for (i32 i = 0; i < resolved.absolute.size; i++) {
        if (resolved[i] == '/') {
            resolved[i] = '\\';
        }
    }

    return resolved;
}

FolderPath resolve_folder_path(GamePath rp, StringView path, Allocator *a)
{
    String root;

    switch (rp) {
    case GamePath_data:
        root = g_paths.data;
        break;
    case GamePath_exe:
        root = g_paths.exe;
        break;
    case GamePath_shaders:
        root = g_paths.shaders;
        break;
    case GamePath_textures:
        root = g_paths.textures;
        break;
    case GamePath_models:
        root = g_paths.models;
        break;
    case GamePath_preferences:
        root = g_paths.preferences;
        break;
    default:
        LOG(Log_error, "unknown path root: %d", rp);
        ASSERT(false);
        return {};
    }

    FolderPath resolved = create_folder_path(a, root, path);
    for (i32 i = 0; i < resolved.absolute.size; i++) {
        if (resolved[i] == '/') {
            resolved[i] = '\\';
        }
    }

    return resolved;
}

bool file_exists(FilePathView file)
{
    return PathFileExists(file.absolute.bytes) == TRUE;
}

bool folder_exists(const char *path)
{
    DWORD r = GetFileAttributes(path);
    return (r != INVALID_FILE_ATTRIBUTES && (r & FILE_ATTRIBUTE_DIRECTORY));
}

bool create_file(FilePathView p, bool create_folders = false)
{
    if (create_folders) {
        // TODO(jesper): replace with create_folder_path_view
        char folder[MAX_PATH];
        strncpy(folder, p.absolute.bytes, p.absolute.size - p.filename.size - 1);

        if (!folder_exists(folder)) {
            int result = SHCreateDirectoryEx(NULL, folder, NULL);
            if (result != ERROR_SUCCESS) {
                LOG(Log_error, "couldn't create folders: %s", folder);
                return false;
            }
        }
    }


    HANDLE file_handle = CreateFile(
        p.absolute.bytes,
        0, 0, NULL,
        CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (file_handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    CloseHandle(file_handle);
    return true;
}

void* open_file(FilePathView path, FileAccess access)
{
    DWORD flags;
    DWORD share_mode;

    switch (access) {
    case FileAccess_read:
        flags      = GENERIC_READ;
        share_mode = FILE_SHARE_READ;
        break;
    case FileAccess_write:
        flags      = GENERIC_WRITE;
        share_mode = 0;
        break;
    case FileAccess_read_write:
        flags      = GENERIC_READ | GENERIC_WRITE;
        share_mode = 0;
        break;
    default:
        ASSERT(false);
        return nullptr;
    }

    HANDLE file_handle = CreateFile(
        path.absolute.bytes,
        flags, share_mode,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (file_handle == INVALID_HANDLE_VALUE) {
        return nullptr;
    }

    return (void*)file_handle;
}

void close_file(void *file_handle)
{
    CloseHandle((HANDLE)file_handle);
}

char* read_file(FilePathView filename, usize *o_size, Allocator *a)
{
    char *buffer = nullptr;

    HANDLE file = CreateFile(
        filename.absolute.bytes,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (file == INVALID_HANDLE_VALUE) {
        LOG(Log_error,
            "failed to open file %s - %s",
            filename.absolute.bytes,
            win32_system_error_message(GetLastError()));
        return nullptr;
    }

    LARGE_INTEGER file_size;
    if (GetFileSizeEx(file, &file_size)) {
        // NOTE(jesper): ReadFile only works on 32 bit sizes
        ASSERT(file_size.QuadPart <= 0xFFFFFFFF);
        *o_size = (usize) file_size.QuadPart;

        buffer = (char*)a->alloc((usize)file_size.QuadPart);
        if (buffer == nullptr) {
            LOG(Log_error,
                "failed to allocate %d bytes from allocator for file %s",
                file_size.QuadPart, filename);
            return nullptr;
        }

        DWORD bytes_read;
        if (!ReadFile(file, buffer, (u32)file_size.QuadPart, &bytes_read, 0)) {
            a->dealloc(buffer);
            buffer = nullptr;
        }

        CloseHandle(file);
    }

    return buffer;
}

void write_file(void *file_handle, void *buffer, usize bytes)
{
    BOOL result;

    // NOTE(jesper): WriteFile takes 32 bit number of bytes to write
    ASSERT(bytes <= 0xFFFFFFFF);

    DWORD bytes_written;
    result = WriteFile((HANDLE) file_handle,
                            buffer,
                            (DWORD) bytes,
                            &bytes_written,
                            NULL);


    ASSERT(bytes  == bytes_written);
    ASSERT(result == TRUE);
}

