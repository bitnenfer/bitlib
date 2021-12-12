#pragma once

#include <bit/core/memory/allocator.h>
#include <bit/utility/utility.h>

namespace bit
{
	enum class PageProtectionType
	{
		PROTECTION_TYPE_READ_WRITE,
		PROTECTION_TYPE_READ_ONLY
	};

	struct BITLIB_API VirtualMemoryBlock
	{
		VirtualMemoryBlock();
		VirtualMemoryBlock(void* BaseAddress, size_t ReservedSize);
		VirtualMemoryBlock(VirtualMemoryBlock&& Move);
		VirtualMemoryBlock& operator=(VirtualMemoryBlock&& Move);
		void* CommitAll();
		bool DecommitAll();
		void* CommitPagesByAddress(void* Address, size_t Size);
		bool DecommitPagesByAddress(void* Address, size_t Size);
		bool ProtectPagesByAddress(void* Address, size_t Size, PageProtectionType Protection);
		void* CommitPagesByOffset(size_t Offset, size_t Size);
		bool DecommitPagesByOffset(size_t Offset, size_t Size);
		bool ProtectPagesByOffset(size_t Offset, size_t Size, PageProtectionType Protection);
		void* GetBaseAddress() const;
		void* GetAddress(size_t Offset) const;
		void* GetEndAddress() const;
		size_t GetReservedSize() const;
		size_t GetCommittedSize() const;
		size_t GetPageCount() const;
		bool IsValid() const;
		void Invalidate();
		bool OwnsAddress(const void* Ptr) const;

	private:
		VirtualMemoryBlock(const VirtualMemoryBlock&) = delete;
		VirtualMemoryBlock& operator=(const VirtualMemoryBlock&) = delete;

		void* BaseAddress = nullptr;
		size_t ReservedSize = 0;
		int64_t CommittedSize = 0;
	};

	BITLIB_API bool VirtualAllocateBlock(void* Address, size_t Size, VirtualMemoryBlock& OutVirtualMemorySpace);
	BITLIB_API bool VirtualAllocateBlock(size_t Size, VirtualMemoryBlock& OutVirtualMemorySpace);
	BITLIB_API void VirtualFreeBlock(VirtualMemoryBlock& VirtualMemorySpace);
}
