#include "prelude.h"
#include "linker.h"
#include "base/temp_mem.h"
#include "base/algorithm.h"
#include "base/ident_map.h"
#include "obj/obj_file.h"
#include "linker/reloc.h"
#include <string.h>

ident GetBaseNameFromSectionName(ident sectionName)
{
	char *ptr = (char*)strchr(sectionName.Str(), '$');
	if (!ptr)
		return sectionName;

	return MakeIdent(sectionName.Str(), ptr - sectionName.Str());
}

struct link_section
{
	const obj_section *Obj;
	const obj_file *ObjFile;
	uint32_t ObjIndex;
	uint32_t VirtualBase;
};

struct link_final_section
{
	ident BaseName;
	slice<link_section> Sections;

	uint32_t SharedFlags;
	uint32_t VirtualSize;
	uint32_t VirtualBase;
	uint32_t VirtualAlignment;
};

struct link_obj_file
{
	const obj_file *Obj;
	slice<link_section*> Sections;
};

bool CompareLinkSections(const link_section& a, const link_section& b)
{
	int nameCmp = strcmp(a.Obj->Name.Str(), b.Obj->Name.Str());
	if (nameCmp != 0)
		return nameCmp < 0;

	if (a.Obj->VirtualAlignment != b.Obj->VirtualAlignment)
		return a.Obj->VirtualAlignment > b.Obj->VirtualAlignment;

	return false;
}

bool CompareFinalSections(const link_final_section& a, const link_final_section& b)
{
	uint32_t af = a.SharedFlags;
	uint32_t bf = b.SharedFlags;
	uint32_t df = af ^ bf;

	// Sections with data come first
	if (df & ObjSharedFlagHasData)
		return (af & ObjSharedFlagHasData) != 0;

	return false;
}

int LinkObjects(obj_file& dst, slice<const obj_file*> objects, const link_options &opt)
{
	temp_mem &tm = *opt.memory;

	slice<link_final_section> finalSections;

	dst.Target.ISA = InstructionSetUnknown;

	ident_map<slice<obj_dll_import> > dllImports;

	uint32_t objCount = objects.Count;
	for (uint32_t objIndex = 0; objIndex < objCount; objIndex++)
	{
		const obj_file *obj = objects[objIndex];

		if (obj->Target.ISA != InstructionSetUnknown)
		{
			if (dst.Target.ISA == InstructionSetUnknown)
			{
				dst.Target = obj->Target;
			}
			else
			{
				if (dst.Target.ISA != obj->Target.ISA)
				{
					fprintf(stderr, "Architecture mismatch: %s\n", dst.Name.Str());
					return 1;
				}
			}
		}

		for (const obj_section &sec : obj->Sections)
		{
			ident baseName = GetBaseNameFromSectionName(sec.Name);
			link_final_section *finalSec = NULL;

			for (link_final_section& fsec : finalSections)
			{
				if (fsec.BaseName == baseName)
				{
					finalSec = &fsec;
					break;
				}
			}

			if (!finalSec)
			{
				finalSec = tm.Push(finalSections);
				finalSec->BaseName = baseName;
				finalSec->SharedFlags = sec.SharedFlags;
			}

			link_section *lsec = tm.Push(finalSec->Sections);
			lsec->Obj = &sec;
			lsec->ObjFile = obj;
			lsec->ObjIndex = objIndex;
			if (finalSec->SharedFlags != sec.SharedFlags)
			{
				fprintf(stderr, "Shared flag mismatch in section '%s'\n", baseName.Str());
				return 2;
			}
		}

		for (const obj_dll_import& di : obj->DllImports)
		{
			obj_dll_import *dip;
			dllImports.Insert(dip, di.DllName);
			*dip = di;
		}
	}

	{
		obj_file idataFile;
		idataFile.Target = dst.Target;
		idataFile.TimeStamp = (uint32_t)time(NULL);

		obj_section *idataSec = tm.Push(idataFile.Sections, 4);

		idataSec[0].Name = MakeIdent(".idata$0");
		idataSec[1].Name = MakeIdent(".idata$1");
		idataSec[2].Name = MakeIdent(".idata$2");
		idataSec[3].Name = MakeIdent(".idata$3");
		idataSec[4].Name = MakeIdent(".idata$4");
		idataSec[4].Name = MakeIdent(".text$zdll");

		slice<obj_smmbol> importSymbols;
		slice<obj_relocation> importRelocs;

		slice<char> importDirBody;
		slice<char> importLookupBody;
		slice<char> importStrBody;

		uint32_t importDirSz = 20;

		uint32_t lutEntrySize = (dst.Target.Flags & TargetFlag64Bit) ? 8 : 4;

		for (const auto& diPair : dllImports)
		{
			char name[128];
			int length;
			ident baseSymName, addrSymName, strSymName;

			length = sprintf(name, "__import_lookup_%s_base_");
			baseSymName = MakeIdent(name, length);

			length = sprintf(name, "__import_address_%s_base_");
			addrSymName = MakeIdent(name, length);

			length = sprintf(name, "__import_name_%s_base_");
			strSymName = MakeIdent(name, length);

			ident dllName = diPair.getKey();
			slice<obj_dll_import> imports = diPair.getValue();

			char *idir = tm.Push(importDirBody, importDirSz);
			char *ilut = tm.Push(importLookupBody, (imports.Count + 1) * lutEntrySize);
			char *istr = tm.Push(importStrBody, dllName.Length() + 1);
			memcpy(istr, dllName.Str(), dllName.Length() + 1);

			uint32_t idirOff = idir - importDirBody.Data;
			uint32_t ilutOff = ilut - importLookupBody.Data;
			uint32_t istrOff = istr - importStrBody.Data;

			memset(idir, 0, importDirSz);

			obj_symbol *sl = tm.Push(idataSec[1].Symbols);
			sl->Name = baseSymName;
			sl->Address = ilutOff;
			sl->DefinedInSection = 1;
			sl->Type = ObjSymbolDefine;
			sl->Storage = ObjSymbolStorageStatic;

			obj_symbol *as = tm.Push(idataSecp[2].Symbols);
			as->Name = addrSymName;
			as->Address = ilutOff;
			as->DefinedInSection = 2;
			as->Type = ObjSymbolDefine;
			as->Storage = ObjSymbolStorageStatic;

			obj_symbol *ts = tm.Push(idataSecp[4].Symbols);
			ts->Name = strSymName;
			ts->Address = istrOff;
			ts->DefinedInSection = 2;
			ts->Type = ObjSymbolDefine;
			ts->Storage = ObjSymbolStorageStatic;

			obj_relocation *rl = tm.Push(idataSec[0].Reloactions, 3);
			rl[0].SymbolIndex = sl - idataSec[1].Symbols.Data;
			rl[0].Address = idirOff + 12;
			rl[0].Type = RelocRelativeLE32;
			rl[0].Offset = 0;
			rl[1].SymbolIndex = as - idataSec[2].Symbols.Data;
			rl[1].Address = idirOff + 16;
			rl[1].Type = RelocRelativeLE32;
			rl[1].Offset = 0;
			rl[2].SymbolIndex = ts - idataSec[4].Symbols.Data;
			rl[2].Address = idirOff + 0;
			rl[2].Type = RelocRelativeLE32;
			rl[2].Offset = 0;

			for (const obj_dll_import &imp : imports)
			{
			}
		}
	}

	StableSort(finalSections, CompareFinalSections);

	uint32_t sectionVirtualBase = 0;
	for (link_final_section &fsec : finalSections)
	{
		StableSort(fsec.Sections, CompareLinkSections);

		uint32_t virtualBase = 0;
		uint32_t virtualAlignment = 1;

		for (link_section &lsec : fsec.Sections)
		{
			uint32_t align = lsec.Obj->VirtualAlignment;
			virtualBase = AlignValue(virtualBase, align);

			if (virtualAlignment < align)
				virtualAlignment = align;

			lsec.VirtualBase = virtualBase;
			virtualBase += lsec.Obj->VirtualSize;
		}

		uint32_t virtualSize = virtualBase;

		fsec.VirtualAlignment = virtualAlignment;
		fsec.VirtualSize = virtualSize;

		sectionVirtualBase = AlignValue(sectionVirtualBase, virtualAlignment);
		fsec.VirtualBase = sectionVirtualBase;

		sectionVirtualBase += virtualSize;
	}


	slice<link_obj_file> linkObjs = tm.NewSlice<link_obj_file>(objCount);
	for (uint32_t i = 0; i < objCount; i++)
	{
		linkObjs[i].Obj = objects[i];
	}

	for (link_final_section &fsec : finalSections)
	{
		for (link_section &lsec : fsec.Sections)
		{
			link_obj_file &lobj = linkObjs[lsec.ObjIndex];
			link_section **sec = tm.Push(lobj.Sections);
			*sec = &lsec;
		}
	}

	ident_map<uint32_t> definedSymbols;
	for (link_obj_file &lobj : linkObjs)
	{
		for (const obj_symbol &sym : lobj.Obj->Symbols)
		{
			if (sym.Type == ObjSymbolDefine)
			{
				uint32_t addr = lobj.Sections[sym.DefinedInSection]->VirtualBase + sym.Address;

				uint32_t *pAddr = 0;
				if (!definedSymbols.Insert(pAddr, sym.Name))
				{
					if (*pAddr != addr)
					{
						fprintf(stderr, "Doubly defined symbol: %s\n", sym.Name.Str());
						return 3;
					}
				}
				else
				{
					*pAddr = addr;
				}
			}
		}
	}

	dst.Sections = tm.NewSlice<obj_section>(finalSections.Count);

	slice<reloc_addr> relocAddrs[RelocTypeCount];

	dst.ImageBase = 0x8000000;

	for (uint32_t i = 0; i < finalSections.Count; i++)
	{
		link_final_section &fsec = finalSections[i];
		obj_section &os = dst.Sections[i];

		os.Name = fsec.BaseName;
		os.SharedFlags = fsec.SharedFlags;
		os.VirtualBase = fsec.VirtualBase;
		os.VirtualSize = fsec.VirtualSize;
		os.VirtualAlignment = fsec.VirtualAlignment;

		if (fsec.SharedFlags & ObjSharedFlagHasData)
		{
			void *buf = tm.Alloc(fsec.VirtualSize);
			os.Data = buf;
			os.DataSize = fsec.VirtualSize;

			for (link_section &lsec : fsec.Sections)
			{
				char *base = (char*)buf + lsec.VirtualBase;
				memcpy(base, lsec.Obj->Data, lsec.Obj->DataSize);

				slice<obj_symbol> symbols = lsec.ObjFile->Symbols;
				for (const obj_relocation& reloc : lsec.Obj->Reloactions)
				{
					uint32_t *dst = definedSymbols.Find(symbols[reloc.SymbolIndex].Name);
					if (dst == NULL)
					{
						fprintf(stderr, "Unresolved symbol '%s'\n", symbols[reloc.SymbolIndex].Name.Str());
						return 3;
					}

					reloc_addr *ra = tm.Push(relocAddrs[reloc.Type]);
					ra->PtrToData = base + reloc.Address;
					ra->DataAddress = reloc.Address;
					ra->Value = *dst + reloc.Offset;
				}
			}
		}
		else
		{
			os.Data = NULL;
			os.DataSize = 0;
		}
	}

	for (uint32_t relocTypeIx = 0; relocTypeIx < RelocTypeCount; relocTypeIx++)
	{
		if (relocAddrs[relocTypeIx].Count > 0)
			ApplyRelocations((obj_reloc_type)relocTypeIx, relocAddrs[relocTypeIx], dst.ImageBase);
	}

	return 0;
}

