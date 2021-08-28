#include <bit/utils.h>
#include <bit/array.h>
#include <bit/fixed_allocator.h>
#include <bit/command_args.h>

int main(int32_t Argc, const char* Argv[])
{
	bit::FixedAllocator<int32_t, 1000> Allocator;
	bit::Array<int32_t> MyArray;
	bit::Array<int32_t> CopyArray(Allocator);

	for (uint32_t Index = 0; Index < 1025; ++Index)
	{
		MyArray.Add(Index);
	}

	MyArray.FindAll([](int32_t& Value) 
	{
		return bit::IsPow2(Value);
	}, CopyArray);

	int32_t Total = 0;
	CopyArray.ForEach([&Total](int32_t& Pow2Value) 
	{
		Total += Pow2Value;
	});

	size_t Value = bit::Log2(0x1234);

	bit::CommandArgs Cmds(Argv, Argc);
	bool bFoo = Cmds.Contains("foo");
	bool bBar = Cmds.Contains("bar");
	bool bWat = Cmds.Contains("wat");
	const char* WatValue = Cmds.GetValue("wat");
	return 0;
}