#pragma once

#include <bit/array.h>

namespace bit
{
	struct BITLIB_API CCommandArgEntry
	{
		const char* Name;
		const char* Value;
		size_t NameLen;
		size_t ValueLen;
	};

	BITLIB_API_TEMPLATE_STRUCT bit::TArray<bit::CCommandArgEntry>;

	struct BITLIB_API CCommandArgs
	{
		CCommandArgs(const char* Args[], uint32_t ArgCount, IAllocator& Allocator = bit::GetDefaultAllocator());
		bool Contains(const char* Arg);
		const char* GetValue(const char* Arg);
		int32_t GetArgCount();

	private:
		TArray<CCommandArgEntry> Entries;
	};


}
