#pragma once

#include "base/ident.h"
#include <unordered_map>

template <typename T>
struct ident_map
{
	typedef std::unordered_map<void*, T> inner_map;
	inner_map InnerMap;

	bool Insert(T*& t, ident id)
	{
		auto it = InnerMap.find(id.Impl);
		if (it != InnerMap.end())
		{
			t = &it->second;
			return false;
		}
		else
		{
			t = &InnerMap[id.Impl];
			return true;
		}
	}

	T* Find(ident id)
	{
		auto it = InnerMap.find(id.Impl);
		if (it != InnerMap.end())
			return &it->second;
		return NULL;
	}

	struct iterator
	{
		inner_map::iterator It;

		ident GetKey()
		{
			ident i;
			i.Impl = It.first;
			return i;
		}

		T& GetValue()
		{
			return It.second;
		}

		iterator& operator++()
		{
			++It;
			return *this;
		}

		iterator operator++(int)
		{
			iterator ret;
			ret.It = It++;
			return ret;
		}
	};

	iterator begin() { return InnerMap.begin(); }
	iterator end() { return InnerMap.end(); }
};

