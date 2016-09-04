#pragma once

struct in_stream
{
	const char *Buffer;
	const char *Pos;
	const char *End;

	uint8_t Le8()
	{
		uint8_t val = *(const uint8_t*)Pos;
		Pos += sizeof(uint8_t);
		return val;
	}

	uint16_t Le16()
	{
		uint16_t val = *(const uint16_t*)Pos;
		Pos += sizeof(uint16_t);
		return val;
	}

	uint32_t Le32()
	{
		uint32_t val = *(const uint32_t*)Pos;
		Pos += sizeof(uint32_t);
		return val;
	}

	uint64_t Le64()
	{
		uint64_t val = *(const uint64_t*)Pos;
		Pos += sizeof(uint64_t);
		return val;
	}

	in_stream GetSubStreamAtCurrent()
	{
		in_stream s;
		s.Pos = s.Buffer = Pos;
		s.End = End;
		return s;
	}

	in_stream GetSubStreamAtCurrent(uint32_t size)
	{
		in_stream s;
		s.Pos = s.Buffer = Pos;
		s.End = Pos + size;
		return s;
	}

	in_stream GetSubStreamAtAbsolute(uint32_t begin)
	{
		in_stream s;
		s.Pos = s.Buffer = Buffer + begin;
		s.End = End;
		return s;
	}

	in_stream GetSubStreamAtAbsolute(uint32_t begin, uint32_t size)
	{
		in_stream s;
		s.Pos = s.Buffer = Buffer + begin;
		s.End = s.Pos + size;
		return s;
	}

	uint32_t GetPos()
	{
		return Pos - Buffer;
	}

	void Skip(uint32_t num)
	{
		Pos += num;
	}

	const void *GetCurrentPtr()
	{
		return Pos;
	}

	const void *GetSkip(uint32_t size)
	{
		const void *p = Pos;
		Pos += size;
		return p;
	}

	const char *GetZeroTerminatedStr()
	{
		const char *p = Pos;
		Pos += strlen(p);
		return p;
	}

	const void *GetAbsolutePtr(uint32_t offset)
	{
		return Buffer + offset;
	}

	bool IsAtEnd()
	{
		return Pos == End;
	}
};

