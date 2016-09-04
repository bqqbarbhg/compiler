#include "prelude.h"
#include "test_support.h"
#include <string.h>

int main(int argc, char **argv)
{
	if (!strcmp(argv[1], "list"))
	{
		ListAllTestCases();
		return 0;
	}
	else if (!strcmp(argv[1], "run"))
	{
		return RunTestCase(argv[2]);
	}
	else
	{
		return 1;
	}
}

