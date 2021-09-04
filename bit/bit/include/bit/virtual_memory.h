#pragma once

#include <bit/allocator.h>
#include <bit/utils.h>

namespace bit
{
	enum class EPageProtectionType
	{
		PROTECTION_TYPE_READ_WRITE,
		PROTECTION_TYPE_READ_ONLY
	};

	struct BITLIB_API CVirtualMemory
	{
		struct BITLIB_API CMemoryRegion
		{
			friend struct CVirtualMemory;
			void* CommitPagesByAddress(void* Address, size_t Size);
			bool DecommitPagesByAddress(void* Address, size_t Size);
			bool ProtectPagesByAddress(void* Address, size_t Size, EPageProtectionType Protection);
			void* CommitPagesByOffset(size_t Offset, size_t Size);
			bool DecommitPagesByOffset(size_t Offset, size_t Size);
			bool ProtectPagesByOffset(size_t Offset, size_t Size, EPageProtectionType Protection);
			void* GetBaseAddress() const;
			void* GetAddress(size_t Offset) const;
			void* GetEndAddress() const;
			size_t GetRegionSize() const;
			size_t GetCommitedSize() const;
			size_t GetPageCount() const;
			bool IsValid() const;

		private:
			void* BaseAddress = nullptr;
			size_t RegionSize = 0;
			size_t CommitedSize = 0;
		};

		static size_t GetPageSize();
		static size_t GetAllocationGranularity();
		static CMemoryRegion AllocateRegion(void* Address, size_t Size);
		static void ReleaseRegion(CMemoryRegion& VirtualMemoryRegion);
	};
}
