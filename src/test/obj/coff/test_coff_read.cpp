#include "prelude.h"
#include "test/main/test_support.h"
#include "obj/obj_file.h"
#include "obj/coff/coff.h"
#include "base/temp_mem.h"
#include "base/in_stream.h"
#include <stdio.h>
#include <stdlib.h>

TestCase(coff_read_basic)
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

	obj.Name = MakeIdent("freestanding.obj");
	ReadCoffObject(tm, obj, s);

	free(data);
}

TestCase(coff_read_archive)
{
	FILE *objFile = fopen("data/extdll.lib", "rb");
	fseek(objFile, 0, SEEK_END);
	size_t size = ftell(objFile);
	fseek(objFile, 0, SEEK_SET);

	char *data = (char*)malloc(size);
	fread(data, 1, size, objFile);
	fclose(objFile);

	in_stream s;
	s.Buffer = data;
	s.Pos = data;
	s.End = data + size;

	temp_mem tm = { 0 };

	slice<obj_file> objs;

	ReadCoffArchive(tm, objs, s);

	for (const obj_file &obj : objs)
	{
		printf("Object: %s\n", obj.Name.Str());
		for (const obj_symbol &sym : obj.Symbols)
		{
			switch (sym.Type)
			{
			case ObjSymbolExternal:
				printf("  extern %s\n", sym.Name.Str());
				break;
			case ObjSymbolDefine:
				printf("     def %s\n", sym.Name.Str());
				break;
			}
		}
	}

	free(data);
}
