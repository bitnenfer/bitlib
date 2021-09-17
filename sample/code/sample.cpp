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


namespace tlsf
{
	struct TLSFBlockStatusBits
	{
		enum : uint8_t
		{
			STATUS_BUSY_OR_FREE_BIT = 0b01,
			STATUS_LAST_ON_POOL_BIT = 0b10
		};
	};

	struct TLSFBlockHeader
	{
		bool IsFree() const { return (Size & TLSFBlockStatusBits::STATUS_BUSY_OR_FREE_BIT) > 0; }
		bool IsLastOnThePool() const { return (Size & TLSFBlockStatusBits::STATUS_LAST_ON_POOL_BIT) > 0; }
		uint32_t Size;
		TLSFBlockHeader* PrevPhysicalBlock;
	};

	struct TLSFBlockFree : public TLSFBlockHeader
	{
		TLSFBlockHeader* NextFree;
		TLSFBlockHeader* PrevFree;
	};

	struct TLSFParams
	{
		static constexpr size_t SLI = 4;
		static constexpr size_t MAX_POOL_SIZE = bit::TToGiB<2>::Value;
		static constexpr size_t MAX_FL_COUNT = 31;
		static constexpr size_t FL_COUNT = bit::ConstMin(bit::ConstLog2(MAX_POOL_SIZE), MAX_FL_COUNT);
		static constexpr size_t SL_COUNT = 1 << SLI;
		static constexpr size_t MIN_BLOCK_SIZE = sizeof(TLSFBlockHeader);
		static constexpr size_t NUM_LIST = 0;
	};

	struct TLSFStructure
	{
		uint32_t FLBitmap;
		uint32_t SLBitmap[TLSFParams::FL_COUNT];
		TLSFBlockFree* Blocks[TLSFParams::FL_COUNT][TLSFParams::SL_COUNT];
	};

	bit::TTuple<size_t, size_t> Mapping(size_t Size)
	{
		// First level index is the last set bit
		size_t FL = bit::Log2(Size);
		// Second level index is the next rightmost SLI bits
		// For example if SLI is 4, then 460 would result in 
		// 0000000111001100
		//       / |..|	
		//    FL    SL
		// FL = 8, SL = 12
		size_t SL = (Size >> (FL - TLSFParams::SLI)) ^ (TLSFParams::SL_COUNT);
		return { FL, SL };
	}
}

int main(int32_t Argc, const char* Argv[])
{
	bit::TTuple<size_t, size_t> Map = tlsf::Mapping(460);
	size_t FL = Map.Get<0>();
	size_t SL = Map.Get<1>();
	return 0;
}