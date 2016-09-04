#pragma once

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef void test_func();

struct test_case
{
	test_case(const char *name, test_func *func);

	const char *Name;
	test_func *Func;
};

#define TestCase(p_name) \
	void DoTest_##p_name(); \
	test_case test_##p_name(#p_name, &DoTest_##p_name); \
	void DoTest_##p_name()

#define TestAssert(cond) do { if (!(cond)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #cond); exit(1); } } while (0)

void ListAllTestCases();
int RunTestCase(const char *name);
