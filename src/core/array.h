/**
 * file:    array.h
 * created: 2017-03-20
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef ARRAY_H
#define ARRAY_H

#include "platform/platform_debug.h"

template<typename T>
struct Array {
    T* data        = nullptr;
    isize count    = 0;
    isize capacity = 0;

    Allocator *allocator = nullptr;

    T& operator[] (isize i)
    {
        assert(i < count);
        return data[i];
    }
};

template<typename T>
struct StaticArray {
    T* data        = nullptr;
    isize count    = 0;
    isize capacity = 0;

    T& operator[] (isize i)
    {
        assert(i < count);
        return data[i];
    }
};

template<typename T>
isize array_add(Array<T> *a, T e);

#endif /* ARRAY_H */

