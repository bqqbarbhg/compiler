#pragma once

struct ident_impl
{
	uint32_t Length;
	char Data[1];
};

struct ident
{
	ident_impl *Impl;

	bool IsNull() const
	{
		return Impl == NULL;
	}

	uint32_t Length() const
	{
		return Impl->Length;
	}

	const char *Str() const
	{
		return Impl->Data;
	}
};

inline bool operator==(const ident& a, const ident& b)
{
	return a.Impl == b.Impl;
}

inline bool operator!=(const ident& a, const ident& b)
{
	return a.Impl != b.Impl;
}
inline bool operator<(const ident& a, const ident& b)
{
	return a.Impl < b.Impl;
}

ident NullIdent();
void MakeIdents(ident *idents, const char **data, uint32_t *length, uint32_t num);
ident MakeIdent(const char* data, uint32_t length);
ident MakeIdent(const char* data);

