#pragma once

#include "base/slice.h"

struct obj_file;
struct temp_mem;

struct link_options
{
	temp_mem *memory;
};

int LinkObjects(obj_file& dst, slice<const obj_file*> objects, const link_options &opt);

