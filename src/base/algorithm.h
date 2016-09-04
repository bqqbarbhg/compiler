#pragma once

#include <algorithm>

template <typename T, typename F>
inline void StableSort(T& t, F f)
{
	std::stable_sort(t.begin(), t.end(), f);
}

template <typename T, typename U>
inline typename T::iterator BinarySearch(T& t, const U& u)
{
	return std::lower_bound(t.begin(), t.end(), u);
}

inline uint32_t AlignValue(uint32_t val, uint32_t alignment)
{
	return val + (alignment - val % alignment) % alignment;
}

