#pragma once

template <typename T>
struct slice
{
	T *Data;
	uint32_t Count;

	slice()
		: Data(NULL)
		, Count(0)
	{
	}

	slice(T* data, uint32_t count)
		: Data(data)
		, Count(count)
	{
	}

	T& operator[](uint32_t index)
	{
		return Data[index];
	}
	const T& operator[](uint32_t index) const
	{
		return Data[index];
	}

	typedef T* iterator;

	T* begin() { return Data; }
	T* end() { return Data + Count; }
	const T* begin() const { return Data; }
	const T* end() const { return Data + Count; }
};

template <typename T>
slice<T> Slice(T* data, uint32_t count)
{
	return slice<T>(data, count);
}

template <typename T, uint32_t N>
slice<T> Slice(T (&data)[N])
{
	return slice<T>(data, N);
}

