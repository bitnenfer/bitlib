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

	struct BITLIB_API CVirtualAddressSpace
	{
		CVirtualAddressSpace();
		CVirtualAddressSpace(void* BaseAddress, size_t ReservedSize);
		CVirtualAddressSpace(CVirtualAddressSpace&& Move);
		CVirtualAddressSpace& operator=(CVirtualAddressSpace&& Move);
		void* CommitAll();
		bool DecommitAll();
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
		size_t GetCommittedSize() const;
		size_t GetPageCount() const;
		bool IsValid() const;
		void Invalidate();

	private:
		CVirtualAddressSpace(const CVirtualAddressSpace&) = delete;
		CVirtualAddressSpace& operator=(const CVirtualAddressSpace&) = delete;

		void* BaseAddress = nullptr;
		size_t ReservedSize = 0;
		size_t CommittedSize = 0;
	};

	BITLIB_API bool VirtualReserveSpace(void* Address, size_t Size, CVirtualAddressSpace& OutVirtualMemorySpace);
	BITLIB_API void VirtualReleaseSpace(CVirtualAddressSpace& VirtualMemorySpace);
	BITLIB_API void* VirtualDefaultAddress();
}
