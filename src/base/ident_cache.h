#pragma once

#include "ident.h"

struct ident_cache
{
	void CacheIdents(ident *idents, const char **data, uint32_t *length, uint32_t num);
	ident CacheIdent(const char *data, uint32_t length);
};

ident_cache *MakeIdentCache(const ident_cache *base);
void FreeIdentCache(ident_cache *cache);

