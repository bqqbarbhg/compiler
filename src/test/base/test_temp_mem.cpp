#include "prelude.h"
#include "base/temp_mem.h"
#include "test/main/test_support.h"

TestCase(temp_mem_basic)
{
	temp_mem tm = { 0 };

	uint32_t *a = (uint32_t*)tm.Alloc(4);
	uint32_t *b = (uint32_t*)tm.Alloc(4);

	*a = 0xABCDEF01;
	*b = 0xABCDEF01;

	TestAssert(a != b);
	TestAssert(*a = *b);

	tm.Free(b);
	uint32_t *c = (uint32_t*)tm.Alloc(4);

	*c = 0x12341234;

	uint32_t *d = (uint32_t*)tm.Realloc(c, 8);
	d[1] = 0x56785678;

	TestAssert(d[0] == 0x12341234);
	TestAssert(d[1] == 0x56785678);

	tm.Destroy();
}

TestCase(temp_mem_array)
{
	temp_mem tm = { 0 };

	int *arr = NULL;
	uint32_t num = 0;

	for (int i = 0; i < 50; i++)
		*tm.Push(arr, num) = i + 1;

	for (int i = 0; i < 50; i++)
		TestAssert(arr[i] == i + 1);

	tm.Destroy();
}

TestCase(temp_mem_slice)
{
	temp_mem tm = { 0 };

	slice<int> a;
	slice<int> b;

	for (int i = 0; i < 50; i++)
		*tm.Push(a) = i + 1;
	for (int i = 0; i < 50; i++)
		*tm.Push(b) = i + 2;

	for (int i = 0; i < 50; i++)
		TestAssert(a[i] == i + 1);
	for (int i = 0; i < 50; i++)
		TestAssert(b[i] == i + 2);

	tm.Destroy();
}

