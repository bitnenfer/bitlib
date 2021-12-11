#pragma once

#include <bit/container/array.h>

namespace bit
{
	struct BITLIB_API CommandArgEntry
	{
		const char* Name;
		const char* Value;
		size_t NameLen;
		size_t ValueLen;

		friend bool operator==(const CommandArgEntry& LHS, const CommandArgEntry& RHS);
	};

	BITLIB_API_TEMPLATE_STRUCT bit::Array<bit::CommandArgEntry>;

	struct BITLIB_API CommandArgs
	{
		CommandArgs(const char* Args[], uint32_t ArgCount, IAllocator& Allocator = bit::GetGlobalAllocator());
		bool Contains(const char* Arg);
		const char* GetValue(const char* Arg);
		int64_t GetArgCount();

	private:
		Array<CommandArgEntry> Entries;
	};


}
