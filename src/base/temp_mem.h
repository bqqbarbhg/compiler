#pragma once

#include "base/slice.h"
#include <new>

struct temp_mem
{
	void Destroy();

	void *AllocateInternalBlock(uint32_t sizeClass);
	void *AllocBySizeClass(uint32_t sizeClass);

	void *Alloc(size_t size);
	void *Realloc(void *ptr, size_t size);
	void Free(void *ptr);


	void *Push(void **data, uint32_t *pcount, uint32_t sizeOfElem, uint32_t num);

	template <typename T>
	T* Push(T*& data, uint32_t& count)
	{
		T* t = (T*)Push((void**)&data, &count, sizeof(T), 1);
		new (t) T();
		return t;
	}

	template <typename T>
	T* Push(T*& data, uint32_t& count, uint32_t num)
	{
		T* t = (T*)Push((void**)&data, &count, sizeof(T), num);
		for (uint32_t i = 0; i < num; i++)
			new (t + i) T();
		return t;
	}

	template <typename T>
	T* Push(slice<T>& s)
	{
		return (T*)Push(s.Data, s.Count);
	}

	template <typename T>
	T* Push(slice<T>& s, uint32_t num)
	{
		return (T*)Push(s.Data, s.Count, num);
	}

	template <typename T>
	T *New()
	{
		T* t = (T*)Alloc(sizeof(T));
		new (t) T();
		return t;
	}

	template <typename T>
	T *New(uint32_t num)
	{
		T* t = (T*)Alloc(sizeof(T) * num);
		for (uint32_t i = 0; i < num; i++)
			new (t + i) T();
		return t;
	}

	template <typename T>
	slice<T> NewSlice(uint32_t num)
	{
		T* t = (T*)Alloc(sizeof(T) * num);
		for (uint32_t i = 0; i < num; i++)
			new (t + i) T();
		return Slice(t, num);
	}

	void *CurrentBlock;
	uint32_t CurrentPos;
	uint32_t CurrentSize;

	void *NextFreeOfSizeClass[32];
};

