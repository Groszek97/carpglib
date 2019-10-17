#pragma once

#include "File.h"

//-----------------------------------------------------------------------------
// Check tools/pak/pak.txt for specification
struct Pak
{
	enum Flags
	{
		Encrypted = 0x01,
		FullEncrypted = 0x02
	};

	struct Header
	{
		char sign[3];
		byte version;
		uint flags;
		uint files_count;
		uint file_entry_table_size;
	};

	struct File
	{
		union
		{
			cstring filename;
			uint filename_offset;
		};
		uint size;
		uint compressed_size;
		uint offset;
	};

	string path, key;
	FileReader file;
	File* files;
	Buffer* filename_buf;
	bool encrypted;
};
