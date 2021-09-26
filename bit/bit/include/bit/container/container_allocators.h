#pragma once

#include <bit/core/memory.h>
#include <bit/core/os/os.h>

namespace bit
{
	typedef int32_t SizeType_t;
	static_assert(bit::IsSigned<SizeType_t>::Value, "Size type must be signed");

	struct BITLIB_API DefaultHeapAllocator
	{
		void* Allocate(void* Original, SizeType_t Size, SizeType_t Count)
		{
			return bit::Realloc(Original, (size_t)(Size * Count), DEFAULT_ALIGNMENT);
		}

		void Free(void* Ptr)
		{
			bit::Free(Ptr);
		}
	};

	struct BITLIB_API DefaultBlockAllocator
	{
		DefaultBlockAllocator(DefaultBlockAllocator&& Allocator) :
			Block(Allocator.Block),
			AllocationSize(Allocator.AllocationSize)
		{
			Allocator.Block = nullptr;
			Allocator.AllocationSize = 0;
		}
		DefaultBlockAllocator& operator=(DefaultBlockAllocator&& Allocator)
		{
			Block = Allocator.Block;
			AllocationSize = Allocator.AllocationSize;
			Allocator.Block = nullptr;
			Allocator.AllocationSize = 0;
			return *this; 
		}

		DefaultBlockAllocator() :
			Block(nullptr),
			AllocationSize(0)
		{}

		~DefaultBlockAllocator()
		{
			Free();
		}

		void Allocate(SizeType_t Size, SizeType_t Count)
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

		SizeType_t GetAllocationSize() const { return AllocationSize; }
		void* GetAllocation() const { return Block; }
		bool IsValid() const { return Block != nullptr; }

	private:
		DefaultBlockAllocator(const DefaultBlockAllocator&) = delete;
		void* Block;
		SizeType_t AllocationSize;
		DefaultHeapAllocator HeapAllocator;
	};

	template<typename T, size_t Capacity>
	struct FixedBlockAllocator
	{
		void Allocate(SizeType_t Size, SizeType_t Count)
		{
			BIT_ASSERT_MSG(Size * Count <= Capacity * sizeof(T), "Block is too small to fit requested allocation.");
		}

		void Free() {}
		SizeType_t GetAllocationSize() const { return Capacity * sizeof(T); }
		void* GetAllocation() const { return (void*)&Block[0]; }
		bool IsValid() const { return true; }
	
	private:
		T Block[Capacity];
	};

	template<typename T, size_t InlineCount, typename TFallbackAllocator = DefaultBlockAllocator>
	struct SmallBufferBlockAllocator
	{
		SmallBufferBlockAllocator() :
			Block((void*)&InlineBlock[0]),
			AllocationSize(InlineCount * sizeof(T))
		{}
		SmallBufferBlockAllocator(SmallBufferBlockAllocator&& Allocator) :
			Block((void*)&InlineBlock[0]),
			AllocationSize(InlineCount * sizeof(T))
		{
			if (Allocator.IsUsingInlineBlock())
			{
				bit::Memcpy(InlineBlock, Allocator.InlineBlock, InlineCount * sizeof(T));
			}
			else
			{
				Block = Allocator.Block;
				AllocationSize = Allocator.AllocationSize;
			}
			Allocator.Block = nullptr;
			Allocator.AllocationSize = 0;
		}
		SmallBufferBlockAllocator& operator=(SmallBufferBlockAllocator&& Allocator)
		{
			if (Allocator.IsUsingInlineBlock())
			{
				bit::Memcpy(InlineBlock, Allocator.InlineBlock, InlineCount * sizeof(T));
			}
			else
			{
				Block = Allocator.Block;
				AllocationSize = Allocator.AllocationSize;
			}
			Allocator.Block = nullptr;
			Allocator.AllocationSize = 0;
			return *this;
		}

		bool IsUsingInlineBlock() const { return Block == &InlineBlock[0]; }

		void Allocate(SizeType_t Size, SizeType_t Count)
		{
			if (Size * Count > InlineCount * sizeof(T))
			{
				FallbackAllocator.Allocate(Size, Count);
				if (IsUsingInlineBlock())
				{
					bit::Memcpy(FallbackAllocator.GetAllocation(), Block, InlineCount * sizeof(T));
				}
				Block = FallbackAllocator.GetAllocation();
				AllocationSize = Size * Count;
			}
			else
			{
				Block = (void*)&InlineBlock[0];
				AllocationSize = InlineCount * sizeof(T);
			}
		}

		void Free() 
		{
			if (!IsUsingInlineBlock())
			{
				FallbackAllocator.Free();
				Block = nullptr;
				AllocationSize = 0;
			}
		}
		SizeType_t GetAllocationSize() const { return AllocationSize; }
		void* GetAllocation() const { return Block; }
		bool IsValid() const { return Block != nullptr; }

	private:
		SmallBufferBlockAllocator(const SmallBufferBlockAllocator&) = delete;

		T InlineBlock[InlineCount];
		TFallbackAllocator FallbackAllocator;
		void* Block;
		SizeType_t AllocationSize;
	};

	struct BITLIB_API DefaultLinkedListAllocator
	{
		template<typename TLinkType>
		struct LinkAllocator
		{
			TLinkType* AllocateLink(SizeType_t Count)
			{
				return (TLinkType*)HeapAllocator.Allocate(nullptr, sizeof(TLinkType), 1);
			}

			void FreeLink(TLinkType* Link)
			{
				HeapAllocator.Free(Link);
			}

			DefaultHeapAllocator HeapAllocator;
		};
	};

	struct BITLIB_API DefaultHashTableAllocator
	{
		typedef DefaultLinkedListAllocator BucketEntryAllocatorType_t;
		typedef DefaultHeapAllocator BucketAllocatorType_t;
	};
}