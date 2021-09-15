#include <bit/utils.h>
#include <bit/array.h>
#include <bit/command_args.h>
#include <bit/atomics.h>
#include <bit/critical_section.h>
#include <bit/thread.h>
#include <bit/os.h>
#include <bit/scope_timer.h>
#include <bit/hash_table.h>
#include <bit/linked_list.h>
#include <bit/intrusive_linked_list.h>
#include <bit/linear_allocator.h>
#include <bit/virtual_memory.h>
#include <bit/page_allocator.h>
#include <bit/pointers.h>
#include <bit/string.h>
#include <bit/scope_lock.h>
#include <bit/mutex.h>
#include <bit/rw_lock.h>
#include <bit/thread_local_storage.h>

struct SLBlock
{

	uint8_t* Blocks;
	uint8_t Bitmap;
};

int main(int32_t Argc, const char* Argv[])
{
	bit::TArray<bit::CThread> Threads;
	bit::TArray<int32_t> Data;

	bit::THashTable<bit::CString, int32_t> Table;
	Table.Insert("Hello", 99);
	int32_t MyValue = Table["Hello"];

	for (int32_t Index = 0; Index < 10; ++Index)
	{
		Data.Add(Index);
	}

	if (Data.RemoveAll([](int32_t& Value) { return Value % 2 == 0; }))
	{
		BIT_LOG("Remove was successful!");
	}

	Data.Compact();

	for (int32_t Index = 0; Index < bit::GetOSProcessorCount(); ++Index)
	{
		Threads.Allocate().Start([](void* UserData) 
		{
			bit::CTLSHandle TLS = bit::CTLSAllocator::Get().Allocate(bit::Malloc(128));
			bit::CThread::SleepThread(100);
			bit::Free(TLS->GetData());
			bit::CTLSAllocator::Get().Free(TLS);
			return 0;
		}, 4096, nullptr);
	}

	for (bit::CThread& Thread : Threads)
	{
		Thread.Join();
	}

	return 0;
}