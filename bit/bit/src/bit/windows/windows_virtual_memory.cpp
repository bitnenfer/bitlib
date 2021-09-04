#include <bit/virtual_memory.h>
#include <bit/memory.h>
#include <bit/os.h>
#include "windows_common.h"

static DWORD BitGetProtection(bit::EPageProtectionType ProtectionType)
{
	switch (ProtectionType)
	{
	case bit::EPageProtectionType::PROTECTION_TYPE_READ_WRITE: return PAGE_READWRITE;
	case bit::EPageProtectionType::PROTECTION_TYPE_READ_ONLY: return PAGE_READONLY;
	}
	return PAGE_READONLY;
}

void* bit::CVirtualMemory::CMemoryRegion::CommitPagesByAddress(void* Address, size_t Size)
{
	if (bit::PtrInRange(Address, BaseAddress, GetEndAddress()) && Size <= RegionSize)
	{
		void* Pages = VirtualAlloc(Address, Size, MEM_COMMIT, PAGE_READWRITE);
		if (Pages != nullptr) CommittedSize += Size;
		return Pages;
	}
	return nullptr;
}

bool bit::CVirtualMemory::CMemoryRegion::DecommitPagesByAddress(void* Address, size_t Size)
{
	if (bit::PtrInRange(Address, BaseAddress, GetEndAddress()) && Size <= RegionSize)
	{
		if (VirtualFree(Address, Size, MEM_DECOMMIT))
		{
			CommittedSize -= Size;
			return true;
		}
		return false;
	}
	return false;
}

bool bit::CVirtualMemory::CMemoryRegion::ProtectPagesByAddress(void* Address, size_t Size, EPageProtectionType Protection)
{
	if (bit::PtrInRange(Address, BaseAddress, GetEndAddress()) && Size <= RegionSize)
	{
		DWORD OldProtection = 0;
		return VirtualProtect(Address, Size, BitGetProtection(Protection), &OldProtection) == TRUE;
	}
	return false;
}

void* bit::CVirtualMemory::CMemoryRegion::CommitPagesByOffset(size_t Offset, size_t Size)
{
	void* Address = GetAddress(Offset);
	if (bit::PtrInRange(Address, BaseAddress, GetEndAddress()) && Size <= RegionSize)
	{
		void* Pages = VirtualAlloc(Address, Size, MEM_COMMIT, PAGE_READWRITE);
		if (Pages != nullptr) CommittedSize += Size;
		return Pages;
	}
	return nullptr;
}

bool bit::CVirtualMemory::CMemoryRegion::DecommitPagesByOffset(size_t Offset, size_t Size)
{
	void* Address = GetAddress(Offset);
	if (bit::PtrInRange(Address, BaseAddress, GetEndAddress()) && Size <= RegionSize)
	{
		if (VirtualFree(Address, Size, MEM_DECOMMIT))
		{
			CommittedSize -= Size;
			return true;
		}
		return false;
	}
	return false;
}

bool bit::CVirtualMemory::CMemoryRegion::ProtectPagesByOffset(size_t Offset, size_t Size, EPageProtectionType Protection)
{
	void* Address = GetAddress(Offset);
	if (bit::PtrInRange(Address, BaseAddress, GetEndAddress()) && Size <= RegionSize)
	{
		DWORD OldProtection = 0;
		return VirtualProtect(Address, Size, BitGetProtection(Protection), &OldProtection) == TRUE;
	}
	return false;
}

void* bit::CVirtualMemory::CMemoryRegion::GetBaseAddress() const
{
	return BaseAddress;
}

void* bit::CVirtualMemory::CMemoryRegion::GetAddress(size_t Offset) const
{
	return bit::ForwardPtr(BaseAddress, Offset);
}

void* bit::CVirtualMemory::CMemoryRegion::GetEndAddress() const
{
	return bit::ForwardPtr(BaseAddress, RegionSize);
}

size_t bit::CVirtualMemory::CMemoryRegion::GetRegionSize() const
{
	return RegionSize;
}

size_t bit::CVirtualMemory::CMemoryRegion::GetCommitedSize() const
{
	return CommittedSize;
}

size_t bit::CVirtualMemory::CMemoryRegion::GetPageCount() const
{
	return RegionSize / bit::GetOSPageSize();
}

bool bit::CVirtualMemory::CMemoryRegion::IsValid() const
{ 
	return BaseAddress != nullptr; 
}

bit::CVirtualMemory::CMemoryRegion bit::CVirtualMemory::AllocateRegion(void* Address, size_t Size)
{
	bit::CVirtualMemory::CMemoryRegion VirtualMemoryRegion;
	bit::Memset(&VirtualMemoryRegion, 0, sizeof(bit::CVirtualMemory::CMemoryRegion));
	void* Ptr = VirtualAlloc(Address, Size, MEM_RESERVE, PAGE_READWRITE);

	MEMORY_BASIC_INFORMATION MemInfo = {};
	VirtualQuery(Ptr, &MemInfo, sizeof(MEMORY_BASIC_INFORMATION));

	VirtualMemoryRegion.BaseAddress = Ptr;
	VirtualMemoryRegion.RegionSize = MemInfo.RegionSize;
	return VirtualMemoryRegion;
}

void bit::CVirtualMemory::ReleaseRegion(bit::CVirtualMemory::CMemoryRegion& VirtualMemoryRegion)
{
	if (VirtualMemoryRegion.IsValid())
	{
		VirtualFree(VirtualMemoryRegion.GetBaseAddress(), VirtualMemoryRegion.GetRegionSize(), MEM_RELEASE);
		bit::Memset(&VirtualMemoryRegion, 0, sizeof(bit::CVirtualMemory::CMemoryRegion));
	}
}
