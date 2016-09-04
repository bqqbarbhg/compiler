#pragma once

#include "base/slice.h"
#include "obj/obj_file.h"

struct reloc_addr
{
	void *PtrToData;
	uint32_t Value;
	uint32_t DataAddress;
};

void ApplyRelocations(obj_reloc_type type, slice<reloc_addr> relocs, uint64_t imageBase);

