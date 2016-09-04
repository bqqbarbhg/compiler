#include "prelude.h"
#include "temp_mem.h"
#include "base/algorithm.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct temp_alloc_header
{
	uint32_t SizeClass;
	uint32_t Capacity;
};

static uint32_t GetSizeClass(size_t size)
{
	uint32_t sizeClass = 0;
	size -= 1;
	size >>= 4;
	while (size)
	{
		size >>= 1;
		sizeClass++;
	}
	return sizeClass;
}

static uint32_t GetSizeFromClass(uint32_t sizeClass)
{
	return 1 << sizeClass << 4;
}

void temp_mem::Destroy()
{
	void *nextPtr = CurrentBlock;
	while (nextPtr)
	{
		void *ptrToFree = nextPtr;
		nextPtr = *(void**)nextPtr;
		free(ptrToFree);
	}
}

void *temp_mem::AllocateInternalBlock(uint32_t sizeClass)
{
	uint32_t size = GetSizeFromClass(sizeClass);
	if (CurrentPos + size >= CurrentSize)
	{
		uint32_t newBlockSize = 1024*1024;
		void *newBlock = malloc(newBlockSize);

		*(void**)newBlock = CurrentBlock;

		CurrentBlock = newBlock;
		CurrentPos = AlignValue(sizeof(void*), 8);
		CurrentSize = newBlockSize;
	}

	char *block = (char*)CurrentBlock;
	uint32_t pos = CurrentPos;
	assert((pos % 8) == 0);

	temp_alloc_header *hdr = (temp_alloc_header*)(block + pos);
	pos += sizeof(temp_alloc_header);

	hdr->SizeClass = sizeClass;
	hdr->Capacity = size;

	CurrentPos = pos + size;
	return block + pos;
}

void *temp_mem::AllocBySizeClass(size_t sizeClass)
{
	void *ptr = NextFreeOfSizeClass[sizeClass];
	if (ptr)
	{
		NextFreeOfSizeClass[sizeClass] = *(void**)ptr;
		return ptr;
	}

	return AllocateInternalBlock(sizeClass);
}

void *temp_mem::Alloc(size_t size)
{
	return AllocBySizeClass(GetSizeClass(size));
}

void *temp_mem::Realloc(void *ptr, size_t size)
{
	uint32_t sizeClass = GetSizeClass(size);

	if (!ptr)
		return AllocBySizeClass(sizeClass);

	temp_alloc_header *hdr = (temp_alloc_header*)ptr - 1;
	if (hdr->SizeClass == sizeClass)
		return ptr;

	void *newPtr = AllocBySizeClass(sizeClass);
	uint32_t copySize = hdr->Capacity;
	if (size < copySize)
		copySize = size;

	memcpy(newPtr, ptr, copySize);

	Free(ptr);

	return newPtr;
}

void temp_mem::Free(void *ptr)
{
	temp_alloc_header *hdr = (temp_alloc_header*)ptr - 1;
	uint32_t sizeClass = hdr->SizeClass;
	*(void**)ptr = NextFreeOfSizeClass[sizeClass];
	NextFreeOfSizeClass[sizeClass] = ptr;
}

void *temp_mem::Push(void **data, uint32_t *pcount, uint32_t sizeOfElem, uint32_t num)
{
	uint32_t count = *pcount;
	*pcount += num;

	size_t size = (count + num) * sizeOfElem;

	void *ptr = *data;
	if (ptr == NULL)
	{
		ptr = (char*)Alloc(size);
		*data = ptr;
	}
	else
	{
		temp_alloc_header *header = (temp_alloc_header*)ptr - 1;
		if (size > header->Capacity)
		{
			ptr = Realloc(ptr, size);
			*data = ptr;
		}
	}

	return (char*)ptr + count * sizeOfElem;
}

