#pragma once

#include "base/ident.h"
#include "base/slice.h"
#include "base/definitions.h"

enum obj_reloc_type
{
	RelocUnknown,
	RelocIgnore,

	RelocAbsoluteLE32,
	RelocAbsoluteLE64,
	RelocImageBaseLE32,
	RelocRelativeLE32,

	RelocTypeCount,
};

enum obj_symbol_type
{
	ObjSymbolUnknown,
	ObjSymbolDefine,
	ObjSymbolExternal,
	ObjSymbolAbsolute,
	ObjSymbolMiscDebug,
};

enum obj_symbol_storage
{
	ObjSymbolStorageUnknown,
	ObjSymbolStorageExternal,
	ObjSymbolStorageStatic,
};

struct obj_symbol
{
	ident Name;
	uint32_t Address;
	uint32_t DefinedInSection;
	obj_symbol_type Type;
	obj_symbol_storage Storage;
};

struct obj_relocation
{
	uint32_t SymbolIndex;
	uint32_t Address;
	obj_reloc_type Type;
	uint32_t Offset;
};

enum
{
	ObjSharedFlagHasData = 1 << 0,
};

enum obj_dll_import_type
{
	ObjDllImportTypeCode,
	ObjDllImportTypeData,
	ObjDllImportTypeConst,
};

struct obj_dll_import
{
	ident DllName;
	ident ImportName;
	ident SymbolName;
	uint32_t ImportHint;
	obj_dll_import_type Type;
};

struct obj_section
{
	ident Name;
	slice<obj_relocation> Reloactions;
	void *Data;
	uint32_t DataSize;

	uint32_t VirtualBase;
	uint32_t VirtualSize;
	uint32_t VirtualAlignment;

	uint32_t SharedFlags;
};

struct obj_file
{
	ident Name;

	target_info Target;
	uint32_t TimeStamp;
	uint64_t ImageBase;

	slice<obj_symbol> Symbols;
	slice<obj_section> Sections;
	slice<obj_dll_import> DllImports;
};

