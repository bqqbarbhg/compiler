#include "prelude.h"
#include "ident.h"
#include <unordered_map>
#include <mutex>
#include <string>

static std::unordered_map<std::string, ident_impl*> g_identMap;
static std::mutex g_identMtx;

static ident MakeIdentRaw(ident_impl *impl)
{
	ident i;
	i.Impl = impl;
	return i;
}

static ident MakeIdentImpl(const char *data, uint32_t length)
{
	std::string identStr(data, length);
	auto it = g_identMap.find(identStr);
	if (it != g_identMap.end())
		return MakeIdentRaw(it->second);

	ident_impl *impl = (ident_impl*)malloc(sizeof(uint32_t) + length + 1);
	impl->Length = length;
	memcpy(impl->Data, data, length);
	impl->Data[length] = '\0';
	g_identMap.insert(std::make_pair(identStr, impl));
	return MakeIdentRaw(impl);
}

ident NullIdent()
{
	ident i;
	i.Impl = NULL;
	return i;
}

void MakeIdents(ident *idents, const char **data, uint32_t *length, uint32_t num)
{
	g_identMtx.lock();
	for (uint32_t i = 0; i < num; i++)
		idents[i] = MakeIdentImpl(data[i], length[i]);
	g_identMtx.unlock();
}

ident MakeIdent(const char* data, uint32_t length)
{
	g_identMtx.lock();
	ident ret = MakeIdentImpl(data, length);
	g_identMtx.unlock();
	return ret;
}

ident MakeIdent(const char* data)
{
	return MakeIdent(data, strlen(data));
}

