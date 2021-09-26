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

struct MyValue : public bit::IntrusiveLinkedList<MyValue>
{
	MyValue(int32_t Value) :
		bit::IntrusiveLinkedList<MyValue>(*this),
		Value(Value)
	{}
	int32_t Value;
};

struct MemoryBlock : public bit::IntrusiveLinkedList<MemoryBlock>
{
	MemoryBlock() : bit::IntrusiveLinkedList<MemoryBlock>(*this) {}
	size_t Size;
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

int main(int32_t Argc, const char* Argv[])
{
#if 0
	void* B = bit::Malloc(4);
	bit::Free(B);

#else
	bit::TempFmtString("Hello %s", "World");

	bit::Allocator& DefaultAllocator = bit::GetDefaultAllocator();
	bit::ScopeTimer Timer("Sample");
	bit::PageAllocator PageAllocator("PageAllocator", bit::VirtualDefaultAddress(), 1 GiB);
	bit::String MyString = "Testing";

	MyString += bit::String::Format("Hello wtf %.2f", 3.14f);

	BIT_LOG("My Str says = %s\n", *(MyString + "\nWOoo"));

	MyValue* NM = PageAllocator.New<MyValue>(99);
	size_t Alignment = bit::GetAddressAlignment(NM);
	size_t NMSize = PageAllocator.GetSize(NM);
	size_t Wastage = NMSize - sizeof(MyValue);
	PageAllocator.Delete(NM);

	auto MX = bit::Forward<size_t>(bit::Move(Alignment));

	bit::LinearAllocator LinearAllocator("TestLinearAllocator", PageAllocator.AllocateArena(10 MiB));
	bit::FixedMemoryArena<1 KiB> FixedMemoryArena;
	bit::LinearAllocator FixedAllocator("FixedLinearAllocator", FixedMemoryArena);

	bit::TSharedPtr<TestData> Outside;
	{
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

	}
	{
		bit::pmr::Array<int32_t> MyArray{ LinearAllocator };
		bit::pmr::Array<int32_t> CopyArray{ FixedAllocator };
		bit::HashTable<int32_t, int32_t> Table{};
		bit::LinkedList<int32_t> List{};
		MyValue MyRoot{ 0 };

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

		bit::Array<Payload> PayloadData;

		for (int32_t Value : MyArray)
		{
			Threads.Add(bit::Move(bit::Thread()));
			PayloadData.Add({ &Table, &RWLock, Value });
			List.Insert(Value);
		}

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
	}
	bit::MemoryUsageInfo MemInfo = bit::GetDefaultAllocator().GetMemoryUsageInfo();
	BIT_LOG("Memory Usage Info:\n\tAllocated %.4lf KiB\n\tCommitted %.4lfKiB\n\tReserved %.4lfKiB\n", 
			bit::FromKiB(MemInfo.AllocatedBytes),
			bit::FromKiB(MemInfo.CommittedBytes),
			bit::FromKiB(MemInfo.ReservedBytes)
	);
#endif
	return 0;
}