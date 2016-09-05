/**
 * @file:   file.h
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

#ifndef LEARY_PLATFORM_H
#define LEARY_PLATFORM_H

#include <string>

enum class EnvironmentFolder {
	GameData,        // Game data,        r,   e.g. shaders, textures, models
	UserPreferences, // Game preferences, r/w
	UserData,        // User data,        r/w, e.g. save games.
};

std::string resolve_path(EnvironmentFolder type, const char* filename);

bool directory_exists(const char* path);
void create_directory(const char* path);


#endif // LEARY_PLATFORM_H
