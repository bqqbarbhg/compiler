#include "prelude.h"
#include "ident_cache.h"
#include <unordered_map>
#include <string>

struct ident_cache_impl : ident_cache
{
	std::unordered_map<std::string, ident> Map;
};

ident_cache *MakeIdentCache(const ident_cache *base)
{
	ident_cache_impl *cache = new ident_cache_impl();

	if (base)
	{
		cache->Map = ((ident_cache_impl*)base)->Map;
	}

	return cache;
}

void FreeIdentCache(ident_cache *cache)
{
	delete (ident_cache_impl*)cache;
}

void ident_cache::CacheIdents(ident *idents, const char **data, uint32_t *length, uint32_t num)
{
	ident_cache_impl *cache = (ident_cache_impl*)this;

	const uint32_t batchSize = 32;
	ident batchIdents[batchSize];
	const char *batchData[batchSize];
	uint32_t batchLength[batchSize];
	uint32_t batchIndex[batchSize];
	uint32_t batchCount = 0;

	for (uint32_t i = 0; i < num; i++)
	{
		std::string identStr(data[i], length[i]);
		auto it = cache->Map.find(identStr);
		if (it != cache->Map.end())
		{
			idents[i] = it->second;
			continue;
		}

		if (batchCount >= batchSize)
		{
			MakeIdents(batchIdents, batchData, batchLength, batchCount);
			for (uint32_t batchI = 0; batchI < batchCount; batchI++)
				idents[batchIndex[batchI]] = batchIdents[batchI];
			batchCount = 0;
		}

		batchData[batchCount] = data[i];
		batchLength[batchCount] = length[i];
		batchIndex[batchCount] = i;
		batchCount++;
	}

	if (batchCount > 0)
	{
		MakeIdents(batchIdents, batchData, batchLength, batchCount);
		for (uint32_t batchI = 0; batchI < batchCount; batchI++)
			idents[batchIndex[batchI]] = batchIdents[batchI];
	}
}

ident ident_cache::CacheIdent(const char *data, uint32_t length)
{
	ident_cache_impl *cache = (ident_cache_impl*)this;

	std::string identStr(data, length);
	auto it = cache->Map.find(identStr);
	if (it != cache->Map.end())
		return it->second;

	ident id = MakeIdent(data, length);
	cache->Map.insert(std::make_pair(identStr, id));
	return id;
}

