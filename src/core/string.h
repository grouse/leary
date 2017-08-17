/**
 * file:    string.h
 * created: 2017-08-17
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef LEARY_STRING_H
#define LEARY_STRING_H

#include "platform/platform_debug.h"

// NOTE: IMPORTANT: this will likely _never_ have a constructor or + operator or
// copy constructors or anything on those lines. This struct is _only_ intended
// to be a _slightly_ more powerful extension to c-strings with which I can have
// several Struct objects point to the same memory but with different lengths,
// to reduce memory allocation overhead
struct String {
    // TODO(jesper): for now these are both null-terminated and have a set
    // length in the struct. this is for clib compatability for now, but will be
    // going away when I've built my own strcat w/ friends.
    isize length;
    char  *bytes;

    char& operator[] (isize i)
    {
        DEBUG_ASSERT(i < length);
        return bytes[i];
    }
};

#endif /* LEARY_STRING_H */
