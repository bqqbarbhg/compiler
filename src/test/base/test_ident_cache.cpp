#include "prelude.h"
#include "base/ident_cache.h"
#include "test/main/test_support.h"

TestCase(ident_cache_basic)
{
	ident_cache *cache = MakeIdentCache(NULL);
	TestAssert(cache != NULL);

	ident a = cache->CacheIdent("asd", 3);
	ident b = cache->CacheIdent("asd", 3);

	TestAssert(a == b);

	ident ids[2];
	const char *datas[] = { "test", "test" };
	uint32_t lengths[] = { 4, 4 };

	cache->CacheIdents(ids, datas, lengths, 2);
	TestAssert(ids[0] == ids[1]);

	TestAssert(a != ids[0]);
	TestAssert(b != ids[1]);

	FreeIdentCache(cache);
}

