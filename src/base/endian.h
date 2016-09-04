#pragma once

#include <intrin.h>

uint32_t ReadUnalignedLE32(const void *ptr)
{
	return *(uint32_t*)ptr;
}

uint32_t ReadUnalignedBE32(const void *ptr)
{
	return _byteswap_ulong(*(uint32_t*)ptr);
}

void WriteUnalignedLE32(void *ptr, uint32_t value)
{
	*(uint32_t*)ptr = value;
}

void WriteUnalignedBE32(void *ptr, uint32_t value)
{
	*(uint32_t*)ptr = _byteswap_ulong(value);
}

uint64_t ReadUnalignedLE64(const void *ptr)
{
	return *(uint64_t*)ptr;
}

uint64_t ReadUnalignedBE64(const void *ptr)
{
	return _byteswap_uint64(*(uint64_t*)ptr);
}

void WriteUnalignedLE64(void *ptr, uint64_t value)
{
	*(uint64_t*)ptr = value;
}

void WriteUnalignedBE64(void *ptr, uint64_t value)
{
	*(uint64_t*)ptr = _byteswap_uint64(value);
}


