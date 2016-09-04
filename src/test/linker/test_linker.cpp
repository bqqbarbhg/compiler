#include "prelude.h"
#include "test/main/test_support.h"
#include "linker/linker.h"
#include "base/in_stream.h"
#include "obj/obj_file.h"
#include "base/temp_mem.h"
#include "obj/coff/coff.h"
#include <string.h>

TestCase(linker_basic)
{
	temp_mem tm = { 0 };

	obj_file fileA;
	obj_file fileB;

	{
		obj_section *sec = tm.Push(fileA.Sections);
		sec->Name = MakeIdent(".text$B");
		sec->SharedFlags = ObjSharedFlagHasData;
		sec->VirtualAlignment = 16;
		sec->VirtualSize = 100;
		sec->DataSize = 0;
	}

	{
		obj_section *sec = tm.Push(fileA.Sections);
		sec->Name = MakeIdent(".text$A");
		sec->SharedFlags = ObjSharedFlagHasData;
		sec->VirtualAlignment = 16;
		sec->VirtualSize = 100;
		sec->DataSize = 0;
	}

	{
		obj_section *sec = tm.Push(fileA.Sections);
		sec->Name = MakeIdent(".bss");
		sec->SharedFlags = 0;
		sec->VirtualAlignment = 16;
		sec->VirtualSize = 100;
		sec->DataSize = 0;
	}

	{
		obj_section *sec = tm.Push(fileB.Sections);
		sec->Name = MakeIdent(".text");
		sec->SharedFlags = ObjSharedFlagHasData;
		sec->VirtualAlignment = 16;
		sec->VirtualSize = 100;
		sec->DataSize = 0;
	}

	{
		obj_section *sec = tm.Push(fileB.Sections);
		sec->Name = MakeIdent(".bss");
		sec->SharedFlags = 0;
		sec->VirtualAlignment = 16;
		sec->VirtualSize = 100;
		sec->DataSize = 0;
	}

	link_options opt;
	opt.memory = &tm;

	const obj_file *files[] = { &fileA, &fileB };

	obj_file dst;
	LinkObjects(dst, Slice(files), opt);

	TestAssert(dst.Sections[0].Name == MakeIdent(".text"));
	TestAssert(dst.Sections[1].Name == MakeIdent(".bss"));

	TestAssert(dst.Sections[0].VirtualSize > 300);
	TestAssert(dst.Sections[1].VirtualSize > 200);

	TestAssert(dst.Sections[0].VirtualAlignment >= 16);
	TestAssert(dst.Sections[1].VirtualAlignment >= 16);

	TestAssert(dst.Sections[0].VirtualBase < dst.Sections[1].VirtualBase);

	tm.Destroy();
}

TestCase(linker_freestanding)
{
	FILE *objFile = fopen("data/freestanding.obj", "rb");
	fseek(objFile, 0, SEEK_END);
	size_t size = ftell(objFile);
	fseek(objFile, 0, SEEK_SET);

	char *data = (char*)malloc(size);
	fread(data, 1, size, objFile);
	fclose(objFile);

	in_stream s;
	s.Buffer = data;
	s.Pos = data;

	temp_mem tm = { 0 };

	obj_file obj;
	ReadCoffObject(tm, obj, s);

	obj_file hack;
	hack.Target.ISA = InstructionSetX86_64;

	{
		obj_section *sec = tm.Push(hack.Sections);
		sec->Name = MakeIdent(".text");
		sec->Data = "............................";
		sec->DataSize = strlen((char*)sec->Data);
		sec->VirtualSize = sec->DataSize;
		sec->VirtualAlignment = 16;
		sec->SharedFlags = 0;
	}

	{
		obj_symbol *sym = tm.Push(hack.Symbols);
		sym->Name = MakeIdent("MessageBoxA");
		sym->Address = 0;
		sym->DefinedInSection = 0;
		sym->Type = ObjSymbolDefine;
	}

	{
		obj_symbol *sym = tm.Push(hack.Symbols);
		sym->Name = MakeIdent("ExitProcess");
		sym->Address = 16;
		sym->DefinedInSection = 0;
		sym->Type = ObjSymbolDefine;
	}

	const obj_file *objs[] = { &obj, &hack };

	link_options opt;
	opt.memory = &tm;

	obj_file dst;
	LinkObjects(dst, Slice(objs), opt);

	free(data);
}
