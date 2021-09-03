#pragma once

#include <bit/memory.h>

namespace bit
{
	template<typename TSizeType>
	struct TDefaultHeapAllocator
	{
		void* Allocate(void* Original, TSizeType Size, TSizeType Count)
		{
			return bit::Realloc(Original, Size * Count, DEFAULT_ALIGNMENT);
		}

		void Free(void* Ptr)
		{
			bit::Free(Ptr);
		}
	};

	template<typename TSizeType>
	struct TDefaultBlockAllocator
	{
		TDefaultBlockAllocator(TDefaultBlockAllocator&& Allocator) :
			Block(Allocator.Block),
			AllocationSize(Allocator.AllocationSize)
		{
			Allocator.Block = nullptr;
			Allocator.AllocationSize = 0;
		}
		TDefaultBlockAllocator& operator=(TDefaultBlockAllocator&& Allocator)
		{
			Block = Allocator.Block;
			AllocationSize = Allocator.AllocationSize;
			Allocator.Block = nullptr;
			Allocator.AllocationSize = 0;
			return *this; 
		}

		TDefaultBlockAllocator() :
			Block(nullptr),
			AllocationSize(0)
		{}

		~TDefaultBlockAllocator()
		{
			Free();
		}

		void Allocate(TSizeType Size, TSizeType Count)
		{
			Block = HeapAllocator.Allocate(Block, Size, Count);
			AllocationSize = Size * Count;
		}

		void Free()
		{
			if (IsValid())
			{
				HeapAllocator.Free(Block);
				Block = nullptr;
				AllocationSize = 0;
			}
		}

		TSizeType GetAllocationSize() const { return AllocationSize; }
		void* GetAllocation() const { return Block; }
		bool IsValid() const { return Block != nullptr; }

	private:
		TDefaultBlockAllocator(const TDefaultBlockAllocator&) = delete;
		void* Block;
		TSizeType AllocationSize;
		TDefaultHeapAllocator<TSizeType> HeapAllocator;
	};

	template<typename TSizeType>
	struct TDefaultLinkedListAllocator
	{
		template<typename TLinkType>
		struct TLinkAllocator
		{
			TLinkType* AllocateLink(TSizeType Count)
			{
				return (TLinkType*)HeapAllocator.Allocate(nullptr, sizeof(TLinkType), 1);
			}

			void FreeLink(TLinkType* Link)
			{
				HeapAllocator.Free(Link);
			}

			TDefaultHeapAllocator<TSizeType> HeapAllocator;
		};
	};

	template<typename TSizeType>
	struct TDefaultHashTableAllocator
	{
		typedef TDefaultLinkedListAllocator<TSizeType> BucketEntryAllocatorType_t;
		typedef TDefaultHeapAllocator<TSizeType> BucketAllocatorType_t;
	};
}