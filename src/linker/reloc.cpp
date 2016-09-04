#include "prelude.h"
#include "reloc.h"
#include "base/endian.h"

void ApplyRelocations(obj_reloc_type type, slice<reloc_addr> relocs, uint64_t imageBase)
{
	switch (type)
	{
		case RelocUnknown:
			break;

		case RelocAbsoluteLE32:
			for (reloc_addr &r : relocs)
			{
				WriteUnalignedLE32(r.PtrToData, (uint32_t)imageBase + (uint32_t)r.Value);
			}
			break;
		case RelocAbsoluteLE64:
			for (reloc_addr &r : relocs)
			{
				WriteUnalignedLE64(r.PtrToData, (uint32_t)imageBase + r.Value);
			}
			break;
		case RelocImageBaseLE32:
			for (reloc_addr &r : relocs)
			{
				WriteUnalignedLE32(r.PtrToData, (uint32_t)r.Value);
			}
			break;
		case RelocRelativeLE32:
			for (reloc_addr &r : relocs)
			{
				WriteUnalignedLE32(r.PtrToData, r.Value - r.DataAddress);
			}
			break;
	}
}

