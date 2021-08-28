#include <bit/command_args.h>

#define BIT_STRLEN(Str, Len) while (Str[Len++] != 0); Len--;

bit::CommandArgs::CommandArgs(const char* Args[], uint32_t ArgCount, IAllocator& InAllocator) :
	Allocator(&InAllocator),
	Entries(nullptr)
{
	Entries = BitPlacementNew(Allocator->Alloc(sizeof(*Entries), bit::DEAFULT_ALIGNMENT)) Array<ArgEntry>(InAllocator);

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
					ArgEntry Entry = {};
					Entry.Name = &Arg[StartIndex];
					Entry.NameLen = EndIndex - StartIndex + 1;
					StartIndex = EndIndex + 2;
					while (EndIndex < Len)
					{
						EndIndex++;
					}
					Entry.Value = &Arg[StartIndex];
					Entry.ValueLen = EndIndex - StartIndex + 1;
					Entries->Add(Entry);
					goto next;
				}
				EndIndex += 1;
			}
			ArgEntry Entry = {};
			Entry.Name = &Arg[StartIndex];
			Entry.NameLen = EndIndex - StartIndex + 1;
			Entry.Value = "";
			Entry.ValueLen = 0;
			Entries->Add(Entry);
		}
	next:
		continue;
	}
}

bit::CommandArgs::~CommandArgs()
{
	bit::Destroy(Entries, 1);
	Allocator->Free(Entries);
}

bool bit::CommandArgs::Contains(const char* Arg)
{
	size_t Len = 0;
	BIT_STRLEN(Arg, Len);
	ArgEntry Output;
	return Entries->FindFirst([&](const ArgEntry& Entry)->bool 
	{
		return Entry.NameLen == Len && bit::Memcmp(Arg, Entry.Name, Len) == 0;
	}, Output);
}

const char* bit::CommandArgs::GetValue(const char* Arg)
{
	size_t Len = 0;
	BIT_STRLEN(Arg, Len);
	ArgEntry Output;
	Entries->FindFirst([&](const ArgEntry& Entry)->bool
	{
		return Entry.NameLen == Len && bit::Memcmp(Arg, Entry.Name, Len) == 0;
	}, Output);
	return Output.Value;
}

int32_t bit::CommandArgs::GetArgCount()
{
	return Entries->GetCount();
}
