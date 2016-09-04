#include "prelude.h"
#include "coff.h"
#include "coff_def.h"
#include "base/definitions.h"
#include "base/in_stream.h"
#include "base/slice.h"
#include "base/ident.h"
#include "base/temp_mem.h"
#include "obj/obj_file.h"
#include <assert.h>
#include "base/algorithm.h"

struct coff_reloc_type
{
	obj_reloc_type RelocType;
	uint32_t Offset;
};

coff_reloc_type coffX64RelocTypes[]=
{
	{ RelocIgnore, 0 }, // IMAGE_REL_AMD64_ABSOLUTE
	{ RelocAbsoluteLE64, 0 }, // IMAGE_REL_AMD64_ADDR64
	{ RelocAbsoluteLE32, 0 }, // IMAGE_REL_AMD64_ADDR32
	{ RelocImageBaseLE32, 0 }, // IMAGE_REL_AMD64_ADDR32NB
	{ RelocRelativeLE32, 0 }, // IMAGE_REL_AMD64_REL32
	{ RelocRelativeLE32, 1 }, // IMAGE_REL_AMD64_REL32_1
	{ RelocRelativeLE32, 2 }, // IMAGE_REL_AMD64_REL32_2
	{ RelocRelativeLE32, 3 }, // IMAGE_REL_AMD64_REL32_3
	{ RelocRelativeLE32, 4 }, // IMAGE_REL_AMD64_REL32_4
	{ RelocRelativeLE32, 5 }, // IMAGE_REL_AMD64_REL32_5
	{ RelocUnknown, 0 }, // IMAGE_REL_AMD64_SECTION
	{ RelocUnknown, 0 }, // IMAGE_REL_AMD64_SECREL
	{ RelocUnknown, 0 }, // IMAGE_REL_AMD64_SECREL7
	{ RelocUnknown, 0 }, // IMAGE_REL_AMD64_TOKEN
	{ RelocUnknown, 0 }, // IMAGE_REL_AMD64_SREL32
	{ RelocUnknown, 0 }, // IMAGE_REL_AMD64_PAIR
	{ RelocUnknown, 0 }, // IMAGE_REL_AMD64_SSPAN32
};

struct coff_target
{
	uint16_t CoffMachine;
	instruction_set ISA;
	uint32_t TargetFlags;
	coff_reloc_type *RelocTypes;
	uint32_t RelocTypeCount;
};

static const coff_target coffTargets[] =
{
	{
		IMAGE_FILE_MACHINE_UNKNOWN,
		InstructionSetUnknown,
		0,
	},

	{
		IMAGE_FILE_MACHINE_I386,
		InstructionSetX86,
		0,
	},

	{
		IMAGE_FILE_MACHINE_AMD64,
		InstructionSetX86_64,
		TargetFlag64Bit,
		coffX64RelocTypes,
		ArrayCount(coffX64RelocTypes),
	},

	{
		IMAGE_FILE_MACHINE_ARM,
		InstructionSetARM,
		0,
	},

	{
		IMAGE_FILE_MACHINE_ARM64,
		InstructionSetARM64,
		TargetFlag64Bit,
	},
};

enum
{
	IMPORT_ORDINAL = 0, // The import is by ordinal. This indicates that the value in the Ordinal/Hint field of the import header is the import’s ordinal. If this constant is not specified, then the Ordinal/Hint field should always be interpreted as the import’s hint.
	IMPORT_NAME = 1, // The import name is identical to the public symbol name.
	IMPORT_NAME_NOPREFIX = 2, // The import name is the public symbol name, but skipping the leading ?, @, or optionally _.
	IMPORT_NAME_UNDECORATE = 3, // The import name is the public symbol name, but skipping the leading ?, @, or optionally _, and truncating at the first @.
};

const coff_target *FindCoffTarget(uint16_t coffMachine)
{
	const coff_target *coffTarget = &coffTargets[0];
	for (const coff_target& ct : coffTargets)
	{
		if (ct.CoffMachine == coffMachine)
		{
			coffTarget = &ct;
			break;
		}
	}
	return coffTarget;
}

int ReadCoffObject(temp_mem &tm, obj_file& dst, in_stream& s)
{
	uint16_t coffMachine = s.Le16();
	uint32_t numSections = s.Le16();

	if (numSections == 0xFFFF)
	{
		if (coffMachine != 0)
		{
			fprintf(stderr, "Import header expected, but machine is specified\n");
			return 1;
		}

		uint16_t importHeaderVer = s.Le16();
		uint16_t coffMachine = s.Le16();
		const coff_target *coffTarget = FindCoffTarget(coffMachine);
		dst.Target.ISA = coffTarget->ISA;
		dst.Target.Flags = coffTarget->TargetFlags;
		dst.TimeStamp = s.Le32();
		uint32_t sizeOfData = s.Le32();
		uint16_t ordinalOrHint = s.Le16();
		uint16_t flags = s.Le16();

		const char *symbolName = s.GetZeroTerminatedStr();
		const char *dllName = s.GetZeroTerminatedStr();

		obj_dll_import *di = tm.Push(dst.DllImports);
		di->DllName = MakeIdent(dllName);
		di->SymbolName = MakeIdent(symbolName);
		di->ImportHint = ordinalOrHint;
		di->Type = flags & 0x3;

		uint32_t nameType = (flags >> 2) & 0x7;
		switch (nameType)
		{
		case IMPORT_ORDINAL:
			di->ImportName = NullIdent();
			break;
		case IMPORT_NAME:
			dst->ImportName = dst->SymbolName;
			break;
		case IMPORT_NAME_NOPREFIX:
			{
				char c = symbolName[0];
				if (c == '@' || c == '?' || c == '_')
					dst->ImportName = MakeIdent(symbolName + 1);
				else
					dst->ImportName = dst->SymbolName;
			}
			break;
		case IMPORT_NAME_UNDECORATE:
			{
				char c = symbolName[0];
				const char *start = symbolName;
				if (c == '@' || c == '?' || c == '_')
					start = symbolName + 1;
				const char *end = strchr(start, '@');
				if (start == symbolName && end == NULL)
					dst->ImportName = dst->SymbolName;
				else if (end == NULL)
					dst->ImportName = MakeIdent(start);
				else
					dst->ImportName = MakeIdent(start, (uint32_t)(end - start));
			}
		}

		return 0;
	}

	const coff_target *coffTarget = FindCoffTarget(coffMachine);
	dst.Target.ISA = coffTarget->ISA;
	dst.Target.Flags = coffTarget->TargetFlags;

	dst.TimeStamp = s.Le32();

	uint32_t ptrToSyms = s.Le32();
	uint32_t numSyms = s.Le32();

	uint32_t ptrToStrings = ptrToSyms + numSyms * 18;

	slice<ident> stringTable;
	slice<uint32_t> stringTableOff;

	{
		in_stream ss = s.GetSubStreamAtAbsolute(ptrToStrings);

		const uint32_t batchSize = 64;
		const char *strs[batchSize];
		uint32_t lens[batchSize];
		uint32_t batchPos = 0;

		uint32_t strTabSz = ss.Le32();
		while (ss.GetPos() <= strTabSz)
		{
			const char *str = (const char*)ss.GetCurrentPtr();
			size_t len = strlen(str);

			uint32_t *off = tm.Push(stringTableOff);
			*off = ss.GetPos();

			ss.Skip((uint32_t)len + 1);

			if (batchPos >= batchSize)
			{
				ident *ip = tm.Push(stringTable, batchPos);
				MakeIdents(ip, strs, lens, batchPos);
				batchPos = 0;
			}

			strs[batchPos] = str;
			lens[batchPos] = (uint32_t)len;
			batchPos++;
		}

		if (batchPos > 0)
		{
			ident *ip = tm.Push(stringTable, batchPos);
			MakeIdents(ip, strs, lens, batchPos);
		}
	}

	uint32_t optionalHeaderSize = s.Le16();
	uint16_t flags = s.Le16();
	s.Skip(optionalHeaderSize);

	dst.Sections = tm.NewSlice<obj_section>(numSections);
	for (uint32_t secI = 0; secI < numSections; secI++)
	{
		obj_section& sec = dst.Sections[secI];

		const char *secName = (const char*)s.GetCurrentPtr();
		s.Skip(8);

		if (secName[0] == '/')
		{
			uint32_t index = 0;
			uint32_t pos = 1;
			char c = secName[pos];
			while (c >= '0' && c <= '9')
			{
				index = index * 10 + (c - '0');

				pos++;
				c = secName[pos];
			}

			uint32_t *offP = BinarySearch(stringTableOff, index);
			sec.Name = stringTable[offP - stringTableOff.Data];
		}
		else
		{
			uint32_t len = 0;
			while (len < 8 && secName[len])
				len++;
			sec.Name = MakeIdent(secName, len);
		}

		sec.VirtualSize = s.Le32();
		sec.VirtualBase = s.Le32();
		sec.SharedFlags = 0;

		uint32_t dataSize = s.Le32();
		sec.DataSize = dataSize;
		sec.Data = tm.Alloc(dataSize);
		uint32_t ptrToData = s.Le32();
		memcpy(sec.Data, s.GetAbsolutePtr(ptrToData), dataSize);

		uint32_t ptrToReloc = s.Le32();
		s.Skip(4); // Linenumbers
		uint32_t numReloc = s.Le16();
		s.Skip(2); // Linenumbers
		uint32_t flags = s.Le32();

		// TODO: Flags

		sec.Reloactions = tm.NewSlice<obj_relocation>(numReloc);

		{
			in_stream rs = s.GetSubStreamAtAbsolute(ptrToReloc);
			for (uint32_t i = 0; i < numReloc; i++)
			{
				obj_relocation &reloc = sec.Reloactions[i];
				reloc.Address = rs.Le32();
				reloc.SymbolIndex = rs.Le32();

				uint16_t type = rs.Le16();
				if (type >= coffTarget->RelocTypeCount)
				{
					fprintf(stderr, "Unknown relocation type: %d\n", type);
					return 3;
				}

				coff_reloc_type &coffRelocType = coffTarget->RelocTypes[type];
				reloc.Type = coffRelocType.RelocType;
				reloc.Offset = coffRelocType.Offset;
			}
		}
	}

	{
		in_stream ss = s.GetSubStreamAtAbsolute(ptrToSyms);
		for (uint32_t i = 0; i < numSyms; i++)
		{
			const char *symName = (const char*)ss.GetCurrentPtr();
			ident name;

			if (symName[0])
			{
				ss.Skip(8);

				uint32_t length = 1;
				while (length < 8 && symName[length])
					length++;
				name = MakeIdent(symName, length);
			}
			else
			{
				uint32_t zeros = ss.Le32();
				assert(zeros == 0);
				uint32_t strIndex = ss.Le32();

				uint32_t *offP = BinarySearch(stringTableOff, strIndex);
				name = stringTable[offP - stringTableOff.Data];
			}

			uint32_t value = ss.Le32();
			int sectionIndex = (int)(int16_t)ss.Le16();
			uint32_t type = ss.Le16();

			uint8_t storage = ss.Le8();
			unsigned aux = ss.Le8();

			// TODO: aux records
			ss.Skip(aux * 18);
			i += aux;

			obj_symbol *sym = tm.Push(dst.Symbols);
			sym->Name = name;
			sym->Address = value;
			if (sectionIndex > 0)
			{
				sym->DefinedInSection = sectionIndex - 1;
				sym->Type = ObjSymbolDefine;
			}
			else
			{
				sym->DefinedInSection = 0;
				switch (sectionIndex)
				{
				case 0:
					sym->Type = ObjSymbolExternal;
					break;
				case -1:
					sym->Type = ObjSymbolAbsolute;
					break;
				case -2:
					sym->Type = ObjSymbolMiscDebug;
					break;
				default:
					sym->Type = ObjSymbolUnknown;
					break;
				}
			}

			for (unsigned i = 0; i < aux; i++)
			{
				obj_symbol *sym = tm.Push(dst.Symbols);
				sym->Name = NullIdent();
				sym->Address = 0;
				sym->DefinedInSection = 0;
				sym->Type = ObjSymbolUnknown;
			}
		}
	}

	tm.Free(stringTable.Data);

	return 0;
}

int ReadCoffArchive(temp_mem &tm, slice<obj_file>& dst, in_stream &s)
{
	const char *header = (const char*)s.GetCurrentPtr();
	if (memcmp(header, "!<arch>\n", 8) != 0)
		return 1;
	s.Skip(8);

	int linkerEntryIndex = -1;

	const char *longnames = NULL;

	while (!s.IsAtEnd())
	{
		const char *nameS = (const char*)s.GetSkip(16);
		const char *dateS = (const char*)s.GetSkip(12);
		s.Skip(6); // User ID
		s.Skip(6); // Group ID
		uint64_t mode = s.Le64();
		const char *sizeS = (const char*)s.GetSkip(10);

		uint32_t size = 0;
		for (uint32_t i = 0; i < 10; i++)
		{
			char c = sizeS[i];
			if (c < '0' || c > '9')
				break;
			size = size * 10 + (c - '0');
		}

		const char *endS = (const char*)s.GetSkip(2);
		if (memcmp(endS, "`\n", 2) != 0)
		{
			return 2;
		}

		ident name = NullIdent();

		if (nameS[0] == '/')
		{
			char next = nameS[1];
			if (next == '/')
			{
				longnames = (const char*)s.GetCurrentPtr();
			}
			else if (next >= '0' && next <= '9')
			{
				if (longnames == NULL)
				{
					fprintf(stderr, "Trying to reference a longname before the longnames section\n");
					return 3;
				}

				uint32_t strIndex = next - '0';
				for (int i = 2; i < 16; i++)
				{
					char c = nameS[i];
					if (c <= '0' || c >= '9')
						break;

					strIndex = strIndex * 10 + (c - '0');
				}

				name = MakeIdent(longnames + strIndex);
			}
			else
			{
				linkerEntryIndex++;
				if (linkerEntryIndex == 0)
				{
				}
				else if (linkerEntryIndex == 1)
				{
				}
				else
				{
					fprintf(stderr, "Unexpected linker entry\n");
					return 2;
				}
			}
		}
		else
		{
			unsigned length = 1;
			while (length < 16 && nameS[length] != '/')
				length++;

			name = MakeIdent(nameS, length);
		}

		if (!name.IsNull())
		{
			obj_file *obj = tm.Push(dst);
			obj->Name = name;

			in_stream os = s.GetSubStreamAtCurrent(size);
			if (ReadCoffObject(tm, *obj, os))
			{
				fprintf(stderr, "Failed to read OBJ entry %s\n", name.Str());
				return 3;
			}
		}

		if (size % 2 != 0)
			size++;
		s.Skip(size);
	}

	return 0;
}

