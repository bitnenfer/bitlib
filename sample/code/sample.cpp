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
#include <bit/tuple.h>

namespace bit
{
	struct TLSFBlockHeader
	{
		bool IsFree() const { return (Size & 0b01) > 0; }
		bool IsLastOnThePool() const { return (Size & 0b10) > 0; }
		uint32_t GetSize() const { return Size & (~0b11);  }
		uint32_t Size;
		TLSFBlockHeader* PrevPhysicalBlock;
	};

	struct TLSF
	{
		static constexpr size_t SLI = 4;
		static constexpr size_t MAX_POOL_SIZE = bit::TToGiB<4>::Value;
		static constexpr size_t FL_COUNT = bit::ConstLog2(MAX_POOL_SIZE);
		static constexpr size_t SL_COUNT = 1 << SLI;
		static constexpr size_t MIN_BLOCK_SIZE = sizeof(TLSFBlockHeader);
		static constexpr size_t NUM_LIST = (1 << SLI) * (FL_COUNT - bit::ConstLog2(MIN_BLOCK_SIZE));
	};

	struct TLSFMemoryBlock
	{
		bit::CVirtualAddressSpace Memory;
		TLSFMemoryBlock* Next;
	};

	struct TLSFBlockFree : public TLSFBlockHeader
	{
		TLSFBlockHeader* NextFree;
		TLSFBlockHeader* PrevFree;
	};

	struct TLSFBlockMap
	{
		size_t FL;
		size_t SL;
	};

	struct TLSFStructure
	{
		TLSFBlockMap Mapping(size_t Size)
		{
			// First level index is the last set bit
			size_t FL = bit::Log2(Size);
			// Second level index is the next rightmost SLI bits
			// For example if SLI is 4, then 460 would result in 
			// 0000000111001100
			//       / |..|	
			//    FL    SL
			// FL = 8, SL = 12
			size_t SL = (Size >> (FL - TLSF::SLI)) ^ (TLSF::SL_COUNT);
			return { FL, SL };
		}

		void* Malloc(size_t Size)
		{
			TLSFBlockMap Map = Mapping(Size);
			void* Block = FindSuitableBlock(Size, Map);
			RemoveBlock(Block);
			if (GetBlockSize(Block) > Size)
			{
				void* RemainingBlock = Split(Block, Size);
				Map = Mapping(GetBlockSize(RemainingBlock));
				InsertBlock(RemainingBlock, Map);
			}
			RemoveBlock(Block);
			return Block;
		}

		void Free(void* Block)
		{
			void* FreeBlock = Merge(Block);
			TLSFBlockMap Map = Mapping(GetBlockSize(FreeBlock));
			InsertBlock(FreeBlock, Map);
		}

		void* Merge(void* Block)
		{
			return Block;
		}

		void* FindSuitableBlock(size_t Size, TLSFBlockMap& Map)
		{
			return FreeBlocks[Map.FL][Map.SL];
		}

		void RemoveBlock(void* Block)
		{

		}

		void* Split(void* Block, size_t Size)
		{
			return nullptr;
		}

		size_t GetBlockSize(void* Block)
		{
			return ((TLSFBlockHeader*)Block)->GetSize();
		}

		void InsertBlock(void* Block, TLSFBlockMap& Map)
		{

		}

		void CoalesceBlocks(TLSFBlockFree* FreeBlock)
		{

		}

		uint32_t FLBitmap;
		uint32_t SLBitmap[TLSF::FL_COUNT];
		TLSFBlockFree* FreeBlocks[TLSF::FL_COUNT][TLSF::SL_COUNT];
		TLSFMemoryBlock* MemoryBlockList;
	};

	TLSFStructure* CreateTLSF(size_t InitialPoolSize)
	{
		CVirtualAddressSpace InitialPool = {};
		if (VirtualReserveSpace(nullptr, InitialPoolSize, InitialPool))
		{
			TLSFStructure* Tlsf = (TLSFStructure*)InitialPool.CommitAll();
			bit::Memset(Tlsf, 0, sizeof(TLSFStructure));
			TLSFMemoryBlock* InitialBlock = (TLSFMemoryBlock*)bit::ForwardPtr(Tlsf, sizeof(TLSFStructure));
			InitialBlock->Memory = bit::Move(InitialPool);
			InitialBlock->Next = nullptr;
			Tlsf->MemoryBlockList = InitialBlock;
			return Tlsf;
		}
		return nullptr;
	}

	void DestroyTLSF(TLSFStructure* Tlsf)
	{
		TLSFMemoryBlock* Block = Tlsf->MemoryBlockList;
		while (Block != nullptr)
		{
			TLSFMemoryBlock* Next = Block->Next;
			VirtualReleaseSpace(Block->Memory);
			Block = Next;
		}
	}
}

int main(int32_t Argc, const char* Argv[])
{
	bit::TLSFStructure* Tlsf = bit::CreateTLSF(bit::ToMiB(20));
	bit::DestroyTLSF(Tlsf);

	return 0;
}