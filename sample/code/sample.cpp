#include <bit/utility/utility.h>
#include <bit/container/array.h>
#include <bit/utility/command_args.h>
#include <bit/core/os/atomics.h>
#include <bit/core/os/critical_section.h>
#include <bit/core/os/thread.h>
#include <bit/core/os/os.h>
#include <bit/core/os/debug.h>
#include <bit/utility/prof_timer.h>
#include <bit/container/hash_table.h>
#include <bit/container/linked_list.h>
#include <bit/container/intrusive_linked_list.h>
#include <bit/core/memory/linear_allocator.h>
#include <bit/core/os/virtual_memory.h>
#include <bit/core/memory/page_allocator.h>
#include <bit/utility/pointers.h>
#include <bit/container/string.h>
#include <bit/utility/scope_lock.h>
#include <bit/core/os/mutex.h>
#include <bit/core/os/rw_lock.h>

struct MyValue : public bit::IntrusiveLinkedList<MyValue>
{
	MyValue(int32_t Value) :
		bit::IntrusiveLinkedList<MyValue>(this),
		Value(Value)
	{}
	int32_t Value;
};

struct TestData
{
	TestData(uint32_t Value) : Value(Value) {};
	~TestData()
	{
		BIT_LOG("Destroy TestData %u\n", Value);
	}
	uint32_t Value;
};

template<typename T>
struct CustomDeleter
{
	void operator()(T* const Ptr)
	{
		bit::Delete(Ptr);
		BIT_LOG("Custom deleter!!\n");
	}
};

#include <stdlib.h>

int main(int32_t Argc, const char* Argv[])
{
#if 0
	void* B = bit::Malloc(4);
	bit::Free(B);

#else
	bit::IAllocator& DefaultAllocator = bit::GetDefaultAllocator();
	bit::ProfTimer Timer;
	int32_t IterCount = 100;
	double TotalTime = 0.0;
	double PeakTime = 0.0;

	for (int32_t TestIndex = 0; TestIndex < IterCount; ++TestIndex)
	{
		BIT_ALWAYS_LOG("Iteration %d / %d", TestIndex, IterCount);
		Timer.Begin();
		{
			{
				bit::Array<void*> Ptrs;
				Ptrs.Resize(20000);

				auto RandSize = [](size_t MaxSize)
				{
					double Rand = (double)rand() / (double)RAND_MAX;
					return bit::Max((size_t)((double)MaxSize * Rand), 8ULL);
				};

				for (int32_t Index = 0; Index < 10000; ++Index)
				{
					size_t Size = RandSize(32 KiB);
					void* P = bit::Malloc(Size);
					Ptrs.Add(P);
				}

				for (void* P : Ptrs)
				{
					bit::Free(P);
				}

				Ptrs.Clear();

				for (int32_t Index = 0; Index < 10000; ++Index)
				{
					size_t Size = RandSize(32 KiB);
					void* P = bit::Malloc(Size);
					Ptrs.Add(P);
				}

				for (void* P : Ptrs)
				{
					bit::Free(P);
				}
			}

			bit::TempFmtString("Hello %s", "World");

			bit::String MyString = "Testing";

			MyString += bit::String::Format("Hello wtf %.2f", 3.14f);

			BIT_LOG("My Str says = %s\n", *(MyString + "\nWOoo"));

			bit::LinearAllocator LinearAllocator("TestLinearAllocator", DefaultAllocator.AllocateArena(2 MiB));
			bit::FixedMemoryArena<1 KiB> FixedMemoryArena;
			bit::LinearAllocator FixedAllocator("FixedLinearAllocator", FixedMemoryArena);

			bit::TSharedPtr<TestData> Outside;
			bit::TWeakPtr<TestData> Weak;
			{
				bit::TSharedPtr<TestData> SharedValue = bit::MakeSharedWithAllocator<TestData>(FixedAllocator, 100);
				bit::UniquePtr<TestData> PassAround;
				{
					bit::UniquePtr<TestData> TEST = bit::MakeUnique<TestData>(69);
					TEST.Get()->Value = 9999;
					PassAround.Swap(TEST);
				}


				bit::TSharedPtr<TestData> Copy = SharedValue;
				SharedValue = bit::Move(PassAround);
				Weak = SharedValue;
				Copy.Reset();

				bit::TSharedPtr<MyValue> SharedTest;
				SharedTest.Reset();
				Outside = Weak.Lock();
			}

			bit::Array<int32_t> MyArray{ LinearAllocator };
			bit::Array<int32_t> CopyArray{ FixedAllocator };
			bit::HashTable<int32_t, int32_t> Table{ LinearAllocator };
			bit::LinkedList<int32_t> List{ LinearAllocator };

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

			int32_t Value = bit::BitScanReverse(0x1234);

			bit::AtomicExchange(&Value, 0xFFFF);

			bit::CommandArgs Cmds(Argv, Argc);
			bool bFoo = Cmds.Contains("foo");
			bool bBar = Cmds.Contains("bar");
			bool bWat = Cmds.Contains("wat");
			const char* WatValue = Cmds.GetValue("wat");

			bit::CriticalSection CS;
			bit::Mutex Mtx;
			bit::RWLock RWLock;
			bit::Array<bit::Thread> Threads;

			struct Payload
			{
				bit::HashTable<int32_t, int32_t>* HashTable;
				bit::RWLock* Lock;
				int32_t Value;
			};

			bit::Array<Payload, bit::FixedBlockStorage<Payload, 1025>> PayloadData{};

			for (int32_t Value : MyArray)
			{
				Threads.Add(bit::Move(bit::Thread()));
				PayloadData.Add({ &Table, &RWLock, Value });
				List.Insert(Value);
			}

			bit::Array<int32_t> SimpleCopy = MyArray;

			BIT_ASSERT(SimpleCopy[123] == MyArray[123]);

			int32_t Value100 = List[100];

			for (int32_t Index = 0; Index < PayloadData.GetCount(); ++Index)
			{
				Threads[Index].Start([](void* UserData) -> int32_t
				{
					Payload& Data = *(Payload*)UserData;
					//bit::TScopedLock<bit::CMutex> Lock(Data.Lock);
					bit::ScopedRWLock Lock(Data.Lock, bit::RWLockType::LOCK_READ_WRITE);
					BIT_LOG("Value = %d\n", Data.Value);
					Data.HashTable->Insert(Data.Value, Data.Value);
					return 0;
				}, 4096, &PayloadData[Index]);
			}

			for (bit::Thread& Thread : Threads)
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

			if (TestIndex == IterCount - 1)
			{
				bit::AllocatorMemoryInfo MemInfo = bit::GetDefaultAllocator().GetMemoryUsageInfo();
				BIT_ALWAYS_LOG("%s", bit::TempFmtString(
					"Memory Usage Info:\n\t"
					"Total Allocated: %s\n\t"
					"Total Committed: %s\n\t"
					"Total Reserved: %s\n\t"
					,
					*bit::FormatSize(MemInfo.AllocatedBytes),
					*bit::FormatSize(MemInfo.CommittedBytes),
					*bit::FormatSize(MemInfo.ReservedBytes)
				));
			}
		}
		double Time = Timer.End();
		TotalTime += Time;
		PeakTime = bit::Max(PeakTime, Time);

	}

	bit::AllocatorMemoryInfo MemInfo = bit::GetDefaultAllocator().GetMemoryUsageInfo();
	BIT_ALWAYS_LOG("%s", bit::TempFmtString(
		"Avg. Exec. Time: %.4lf s\n\t"
		"Peak Exec. Time: %.4lf s\n\t"
		"Iter. Count: %d\n\t"
		,
		TotalTime / (double)IterCount,
		PeakTime,
		IterCount
	));

#endif
	return 0;
}