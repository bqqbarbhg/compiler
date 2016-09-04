#include "prelude.h"
#include "base/ident.h"
#include "test/main/test_support.h"

TestCase(ident_basic)
{
	ident a = MakeIdent("asd", 3);
	ident b = MakeIdent("asd");

	TestAssert(a == b);

	ident ids[2];
	const char *datas[] = { "test", "test" };
	uint32_t lengths[] = { 4, 4 };

	MakeIdents(ids, datas, lengths, 2);
	TestAssert(ids[0] == ids[1]);

	TestAssert(a != ids[0]);
	TestAssert(b != ids[1]);
}

