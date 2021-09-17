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

bit::CVirtualAddressSpace::CVirtualAddressSpace() :
	BaseAddress(nullptr),
	ReservedSize(0),
	CommittedSize(0)
{
}

bit::CVirtualAddressSpace::CVirtualAddressSpace(void* BaseAddress, size_t ReservedSize) :
	BaseAddress(BaseAddress),
	ReservedSize(ReservedSize),
	CommittedSize(0)
{}

bit::CVirtualAddressSpace::CVirtualAddressSpace(CVirtualAddressSpace&& Move)
{
	BaseAddress = Move.BaseAddress;
	ReservedSize = Move.ReservedSize;
	CommittedSize = Move.CommittedSize;
	Move.Invalidate();
}

bit::CVirtualAddressSpace& bit::CVirtualAddressSpace::operator=(CVirtualAddressSpace&& Move)
{
	BaseAddress = Move.BaseAddress;
	ReservedSize = Move.ReservedSize;
	CommittedSize = Move.CommittedSize;
	Move.Invalidate();
	return *this;
}

void* bit::CVirtualAddressSpace::CommitAll()
{
	void* Pages = VirtualAlloc(BaseAddress, ReservedSize, MEM_COMMIT, PAGE_READWRITE);
	if (Pages != nullptr) CommittedSize = ReservedSize;
	return Pages;
}

bool bit::CVirtualAddressSpace::DecommitAll()
{
	if (VirtualFree(BaseAddress, ReservedSize, MEM_DECOMMIT))
	{
		CommittedSize = 0;
		return true;
	}
	return false;
}

void* bit::CVirtualAddressSpace::CommitPagesByAddress(void* Address, size_t Size)
{
	if (bit::PtrInRange(Address, BaseAddress, GetEndAddress()) && Size <= ReservedSize)
	{
		void* Pages = VirtualAlloc(Address, Size, MEM_COMMIT, PAGE_READWRITE);
		if (Pages != nullptr) CommittedSize += Size;
		return Pages;
	}
	return nullptr;
}

bool bit::CVirtualAddressSpace::DecommitPagesByAddress(void* Address, size_t Size)
{
	if (bit::PtrInRange(Address, BaseAddress, GetEndAddress()) && Size <= ReservedSize)
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

bool bit::CVirtualAddressSpace::ProtectPagesByAddress(void* Address, size_t Size, EPageProtectionType Protection)
{
	if (bit::PtrInRange(Address, BaseAddress, GetEndAddress()) && Size <= ReservedSize)
	{
		DWORD OldProtection = 0;
		return VirtualProtect(Address, Size, BitGetProtection(Protection), &OldProtection) == TRUE;
	}
	return false;
}

void* bit::CVirtualAddressSpace::CommitPagesByOffset(size_t Offset, size_t Size)
{
	void* Address = GetAddress(Offset);
	if (bit::PtrInRange(Address, BaseAddress, GetEndAddress()) && Size <= ReservedSize)
	{
		void* Pages = VirtualAlloc(Address, Size, MEM_COMMIT, PAGE_READWRITE);
		if (Pages != nullptr) CommittedSize += Size;
		return Pages;
	}
	return nullptr;
}

bool bit::CVirtualAddressSpace::DecommitPagesByOffset(size_t Offset, size_t Size)
{
	void* Address = GetAddress(Offset);
	if (bit::PtrInRange(Address, BaseAddress, GetEndAddress()) && Size <= ReservedSize)
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

bool bit::CVirtualAddressSpace::ProtectPagesByOffset(size_t Offset, size_t Size, EPageProtectionType Protection)
{
	void* Address = GetAddress(Offset);
	if (bit::PtrInRange(Address, BaseAddress, GetEndAddress()) && Size <= ReservedSize)
	{
		DWORD OldProtection = 0;
		return VirtualProtect(Address, Size, BitGetProtection(Protection), &OldProtection) == TRUE;
	}
	return false;
}

void* bit::CVirtualAddressSpace::GetBaseAddress() const
{
	return BaseAddress;
}

void* bit::CVirtualAddressSpace::GetAddress(size_t Offset) const
{
	return bit::ForwardPtr(BaseAddress, Offset);
}

void* bit::CVirtualAddressSpace::GetEndAddress() const
{
	return bit::ForwardPtr(BaseAddress, ReservedSize);
}

size_t bit::CVirtualAddressSpace::GetRegionSize() const
{
	return ReservedSize;
}

size_t bit::CVirtualAddressSpace::GetCommittedSize() const
{
	return CommittedSize;
}

size_t bit::CVirtualAddressSpace::GetPageCount() const
{
	return ReservedSize / bit::GetOSPageSize();
}

bool bit::CVirtualAddressSpace::IsValid() const
{ 
	return BaseAddress != nullptr; 
}

void bit::CVirtualAddressSpace::Invalidate()
{
	BaseAddress = nullptr;
	ReservedSize = 0;
	CommittedSize = 0;
}

bool bit::VirtualReserveSpace(void* Address, size_t Size, CVirtualAddressSpace& OutVirtualMemorySpace)
{
	void* Ptr = VirtualAlloc(Address, Size, MEM_RESERVE, PAGE_READWRITE);
	if (Ptr == nullptr) return false;
	MEMORY_BASIC_INFORMATION MemInfo = {};
	VirtualQuery(Ptr, &MemInfo, sizeof(MEMORY_BASIC_INFORMATION));
	BitPlacementNew(&OutVirtualMemorySpace) CVirtualAddressSpace(Ptr, MemInfo.RegionSize);
	return true;
}

void bit::VirtualReleaseSpace(bit::CVirtualAddressSpace& VirtualMemoryRegion)
{
	if (VirtualMemoryRegion.IsValid())
	{
		void* Base = VirtualMemoryRegion.GetBaseAddress();
		size_t Size = VirtualMemoryRegion.GetRegionSize();
		VirtualFree(Base, Size, MEM_DECOMMIT);
		VirtualFree(Base, Size, MEM_RELEASE);
	}
}

void* bit::VirtualDefaultAddress()
{
	return nullptr;
}
