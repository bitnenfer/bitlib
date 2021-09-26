#include <bit/command_args.h>

#define BIT_STRLEN(Str, Len) while (Str[Len++] != 0); Len--;

bit::CommandArgs::CommandArgs(const char* Args[], uint32_t ArgCount, Allocator& InAllocator) :
	Entries(ArgCount)
{
	for (uint32_t Index = 1; Index < ArgCount; ++Index)
	{
		const char* Arg = Args[Index];
		const char* CurrArg = Arg;
		if (*(CurrArg++) == '-')
		{
			size_t Len = 0;
			BIT_STRLEN(CurrArg, Len);
			size_t StartIndex = 1;
			size_t EndIndex = 0;
			while (EndIndex < Len)
			{
				if (*(CurrArg++) == '=')
				{
					CommandArgEntry Entry = {};
					Entry.Name = &Arg[StartIndex];
					Entry.NameLen = EndIndex - StartIndex + 1;
					StartIndex = EndIndex + 2;
					while (EndIndex < Len)
					{
						EndIndex++;
					}
					Entry.Value = &Arg[StartIndex];
					Entry.ValueLen = EndIndex - StartIndex + 1;
					Entries.Add(Entry);
					goto next;
				}
				EndIndex += 1;
			}
			CommandArgEntry Entry = {};
			Entry.Name = &Arg[StartIndex];
			Entry.NameLen = EndIndex - StartIndex + 1;
			Entry.Value = "";
			Entry.ValueLen = 0;
			Entries.Add(Entry);
		}
	next:
		continue;
	}
}

bool bit::CommandArgs::Contains(const char* Arg)
{
	size_t Len = 0;
	BIT_STRLEN(Arg, Len);
	CommandArgEntry Output;
	return Entries.FindFirst([&](const CommandArgEntry& Entry)->bool 
	{
		return Entry.NameLen == Len && bit::Memcmp(Arg, Entry.Name, Len) == 0;
	}, Output);
}

const char* bit::CommandArgs::GetValue(const char* Arg)
{
	size_t Len = 0;
	BIT_STRLEN(Arg, Len);
	CommandArgEntry Output;
	Entries.FindFirst([&](const CommandArgEntry& Entry)->bool
	{
		return Entry.NameLen == Len && bit::Memcmp(Arg, Entry.Name, Len) == 0;
	}, Output);
	return Output.Value;
}

int64_t bit::CommandArgs::GetArgCount()
{
	return Entries.GetCount();
}

bool bit::operator==(const CommandArgEntry& LHS, const CommandArgEntry& RHS)
{
	return bit::Memcmp(&LHS, &RHS, sizeof(CommandArgEntry));
}
