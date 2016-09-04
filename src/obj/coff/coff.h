#pragma once

#include "base/slice.h"

struct obj_file;
struct temp_mem;
struct in_stream;

int ReadCoffObject(temp_mem &tm, obj_file& dst, in_stream& s);
int ReadCoffArchive(temp_mem &tm, slice<obj_file>& dst, in_stream &s);
