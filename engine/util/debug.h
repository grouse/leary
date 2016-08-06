/**
 * @file:   debug.h
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

#ifndef LEARY_LEARY_DEBUG_H
#define LEARY_LEARY_DEBUG_H

#include <cstdint>
#include <cstring>

#if LEARY_WIN
    #define LEARY_FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
    #define LEARY_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#if LEARY_COMPILER_CLANG || LEARY_COMPILER_GCC
    #define LEARY_FUNCTION_NAME __PRETTY_FUNCTION__
#else
    #define LEARY_FUNCTION_NAME __FUNCTION__
#endif

#define LEARY_LOGF(type, format, ...)                                                              \
	debug::printf(type, LEARY_FUNCTION_NAME, __LINE__, LEARY_FILENAME, format, __VA_ARGS__)

#define LEARY_LOG(type, msg)                                                                       \
	debug::printf(type, LEARY_FUNCTION_NAME, __LINE__, LEARY_FILENAME, "%s", msg)

#define LEARY_UNUSED(x) (void)(x)

#if LEARY_DEBUG

    #define LEARY_ASSERT(condition)                                                                \
	    do {                                                                                       \
	        if (!(condition))                                                                      \
	            LEARY_LOGF(eLogType::Assert, "Assertion failed: %s",  #condition);                 \
	    } while(0)

    #define LEARY_ASSERT_PRINT(condition, msg)                                                     \
	    do {                                                                                       \
	        if (!(condition))                                                                      \
	            LEARY_LOGF(eLogType::Assert, "Assertion failed %s - %s", #condition, msg);         \
	    } while(0)

    #define LEARY_ASSERT_PRINTF(condition, format, ...)                                            \
	    do {                                                                                       \
	        if (!(condition))                                                                      \
	            LEARY_LOGF(eLogType::Assert, "Assertion failed %s - " format,                      \
                           #condition, __VA_ARGS__);                                               \
	    } while(0)

    #define LEARY_UNIMPLEMENTED_FUNCTION                                                           \
	    do {                                                                                       \
	        LEARY_LOG(eLogType::Error, "Unimplemented function!");                                 \
	    } while(0)

#else
    #define LEARY_ASSERT(condition)                     do { } while(0)
    #define LEARY_ASSERT_PRINT(condition, msg)          do { } while(0)
    #define LEARY_ASSERT_PRINTF(condition, format, ...) do { } while(0)
    #define LEARY_UNIMPLEMENTED_FUNCTION
#endif // LEARY_DEBUG

enum class eLogType {
	Error    = (1 << 0),
	Warning  = (1 << 1),
	Info     = (1 << 2),
	Assert   = (1 << 3),
	Any      = (1 << 4) - 1
};

inline int32_t operator &  (eLogType x, eLogType y)
{ 
	return static_cast<int32_t>(x) & static_cast<int32_t>(y);
}

namespace debug
{
	void printf(const char*      func,
	            const uint32_t&  line,
	            const char*      file,
	            const char*      fmt, ...);

	void printf(eLogType chan,
	            const char*      func,
	            const uint32_t&  line,
	            const char*      file,
	            const char*      fmt, ...);
}




#endif // LEARY_LEARY_DEBUG_H
