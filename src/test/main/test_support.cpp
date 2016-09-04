#include "prelude.h"
#include "test_support.h"
#include <stdio.h>
#include <string.h>
#include <unordered_set>

static test_case *g_testCases[1024];
static uint32_t g_numTestCases = 0;

test_case::test_case(const char *name, test_func *func)
	: Name(name)
	, Func(func)
{
	g_testCases[g_numTestCases++] = this;
}

void ListAllTestCases()
{
	for (uint32_t i = 0; i < g_numTestCases; i++)
	{
		printf("%s\n", g_testCases[i]->Name);
	}
}

int RunTestCase(const char *name)
{
	for (uint32_t i = 0; i < g_numTestCases; i++)
	{
		if (!strcmp(g_testCases[i]->Name, name))
		{
			g_testCases[i]->Func();

			return 0;
		}
	}
	fprintf(stderr, "Test case '%s' not found!\n", name);
	return 1;
}

