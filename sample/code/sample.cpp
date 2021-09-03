#include <bit/utils.h>
#include <bit/array.h>
#include <bit/fixed_allocator.h>
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

struct MyValue : public bit::TIntrusiveLinkedList<MyValue>
{
	MyValue(int32_t Value) :
		bit::TIntrusiveLinkedList<MyValue>(*this),
		Value(Value)
	{}
	int32_t Value;
};

int main(int32_t Argc, const char* Argv[])
{
	bit::CScopeTimer Timer("Sample");
	bit::CLinearAllocator LinearAllocator("TestLinearAllocator");
	LinearAllocator.Initialize(bit::Malloc(bit::ToMiB(100), bit::CLinearAllocator::GetRequiredAlignment()), bit::ToMiB(100));
	{
		bit::TArray<int32_t> MyArray;
		bit::TArray<int32_t> CopyArray;
		bit::pmr::THashTable<int32_t, int32_t> Table(LinearAllocator);
		bit::TLinkedList<int32_t> List;
		MyValue MyRoot(0);

		Table.Insert(69, 0);
		Table.Insert(169, 1);
		Table.Insert(269, 2);

		Table.Erase(69);

		int32_t X = Table[69];
		int32_t Y = Table[169];
		int32_t Z = Table[269];
		int32_t W = Table[999];

		for (auto& Elem : Table)
		{
			BIT_LOG("%d = %d\n", Elem.Key, Elem.Value);
		}

		for (uint32_t Index = 0; Index < 1025; ++Index)
		{
			MyArray.Add(Index);
			MyValue* NewNode = bit::New<MyValue>(Index);
			NewNode->InsertAtTail(MyRoot);
		}
		int64_t Count = MyRoot.GetCount();

		for (MyValue& Value : MyRoot)
		{
			BIT_LOG("IntrusiveLinkedList = %u\n", Value.Value);
		}

		MyValue& Value1002 = MyRoot[100];

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

		bit::AtomicExchange64((int64_t*)&Value, 0xFFFF);

		bit::CCommandArgs Cmds(Argv, Argc);
		bool bFoo = Cmds.Contains("foo");
		bool bBar = Cmds.Contains("bar");
		bool bWat = Cmds.Contains("wat");
		const char* WatValue = Cmds.GetValue("wat");

		bit::CCriticalSection CS;
		bit::TArray<bit::CThread> Threads;

		struct Payload
		{
			bit::pmr::THashTable<int32_t, int32_t>* HashTable;
			bit::CCriticalSection* CS;
			int32_t Value;
		};

		bit::TArray<Payload> PayloadData;

		for (int32_t Value : MyArray)
		{
			Threads.Add(bit::Move(bit::CThread()));
			PayloadData.Add({ &Table, &CS, Value });
			List.Insert(Value);
		}

		int32_t Value100 = List[100];

		for (int32_t Index = 0; Index < PayloadData.GetCount(); ++Index)
		{
			Threads[Index].Start([](void* UserData) -> int32_t
			{
				Payload& Data = *(Payload*)UserData;
				bit::CScopeLock Lock(Data.CS);
				BIT_LOG("Value = %d\n", Data.Value);
				Data.HashTable->Insert(Data.Value, Data.Value);
				return 0;
			}, 4096, &PayloadData[Index]);
		}

		for (bit::CThread& Thread : Threads)
		{
			Thread.Join();
		}

		int32_t idx = 0;
		int32_t LastKey = 0;
		int32_t LastValue = 0;
		for (auto Iter = Table.cbegin(); Iter != Table.cend(); ++Iter)
		{
			BIT_LOG("%d = %d\n", Iter->Key, Iter->Value);
			LastKey = Iter->Key;
			LastValue = Iter->Value;
			idx++;
		}
	}
	bit::Free(LinearAllocator.GetBuffer(nullptr));
	bit::CMemoryInfo MemInfo = bit::GetDefaultAllocator().GetMemoryInfo();

	return 0;
}