#include <bit/command_args.h>

#define BIT_STRLEN(Str, Len) while (Str[Len++] != 0); Len--;

bit::CCommandArgs::CCommandArgs(const char* Args[], uint32_t ArgCount, IAllocator& InAllocator) :
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
					CCommandArgEntry Entry = {};
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
			CCommandArgEntry Entry = {};
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

bool bit::CCommandArgs::Contains(const char* Arg)
{
	size_t Len = 0;
	BIT_STRLEN(Arg, Len);
	CCommandArgEntry Output;
	return Entries.FindFirst([&](const CCommandArgEntry& Entry)->bool 
	{
		return Entry.NameLen == Len && bit::Memcmp(Arg, Entry.Name, Len) == 0;
	}, Output);
}

const char* bit::CCommandArgs::GetValue(const char* Arg)
{
	size_t Len = 0;
	BIT_STRLEN(Arg, Len);
	CCommandArgEntry Output;
	Entries.FindFirst([&](const CCommandArgEntry& Entry)->bool
	{
		return Entry.NameLen == Len && bit::Memcmp(Arg, Entry.Name, Len) == 0;
	}, Output);
	return Output.Value;
}

int64_t bit::CCommandArgs::GetArgCount()
{
	return Entries.GetCount();
}
