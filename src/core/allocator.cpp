/**
 * file:    allocator.cpp
 * created: 2017-03-12
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

// TODO(jesper): realloc
// TODO(jesper): zalloc - allocate and zero memset
// TODO(jesper): ialloc - allocate and default initialise struct
// TODO(jesper): allocation header infront of allocated ptr for meta data


LinearAllocator make_linear_allocator(void *start, isize size)
{
	LinearAllocator a;
	a.start   = start;
	a.current = a.start;
	a.last    = nullptr;
	a.size    = size;
	return a;
}

StackAllocator make_stack_allocator(void *start, isize size)
{
	StackAllocator a;
	a.start   = start;
	a.current = a.start;
	a.size    = size;
	return a;
}

void *alloc(LinearAllocator *a, isize size)
{
	void *ptr  = a->current;
	a->current = (u8*)a->current + size;
	a->last    = ptr;
	DEBUG_ASSERT(a->current < ((u8*)a->start + a->size));

	return ptr;
}

void *alloc(StackAllocator *a, isize size)
{
	void *ptr  = a->current;
	a->current = (u8*)a->current + size;
	DEBUG_ASSERT(a->current < ((u8*)a->start + a->size));
	return ptr;
}

template <typename T>
T* alloc(LinearAllocator *a)
{
	return (T*)alloc(a, sizeof(T));
}

template <typename T>
T* alloc_array(LinearAllocator *a, i32 count)
{
	return (T*)alloc(a, sizeof(T) * count);
}

template <typename T>
T* alloc(StackAllocator *a)
{
	return (T*)alloc(a, sizeof(T));
}

template <typename T>
T* alloc_array(StackAllocator *a, i32 count)
{
	return (T*)alloc(a, sizeof(T) * count);
}

void dealloc(LinearAllocator *a, void *ptr)
{
	if (a->last != nullptr && a->last == ptr) {
		a->current = ptr;
		a->last    = nullptr;
	} else {
		DEBUG_LOG("calling dealloc on linear allocator, leaking memory");
	}
}

void dealloc(StackAllocator *a, void *ptr)
{
	a->current = ptr;
}

void reset(LinearAllocator *a)
{
	a->current = a->start;
	a->last    = nullptr;
}

