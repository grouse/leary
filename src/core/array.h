/**
 * file:    array.h
 * created: 2018-01-02
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2018 - all rights reserved
 */

#ifndef LEARY_ARRAY_H
#define LEARY_ARRAY_H

template<typename T>
struct Array {
    T* data      = nullptr;
    i32 count    = 0;
    i32 capacity = 0;

    Allocator *allocator = nullptr;

    T& operator[] (i32 i)
    {
        ASSERT(i < count);
        ASSERT(i >= 0);
        return data[i];
    }

    T* begin()
    {
        return &data[0];
    }

    T* end()
    {
        return &data[count];
    }
};

template<typename T>
struct StaticArray {
    T* data      = nullptr;
    i32 count    = 0;
    i32 capacity = 0;

    T& operator[] (i32 i)
    {
        ASSERT(i < count);
        ASSERT(i >= 0);
        return data[i];
    }

    T* begin()
    {
        return &data[0];
    }

    T* end()
    {
        return &data[count];
    }
};

template<typename T>
i32 array_add(Array<T> *a, T e);

#endif // LEARY_ARRAY_H
