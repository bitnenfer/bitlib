#include <bit/core/os/virtual_memory.h>
#include <bit/core/memory.h>
#include <bit/core/os/os.h>
#include "../../windows_common.h"

static DWORD BitGetProtection(bit::PageProtectionType ProtectionType)
{
	switch (ProtectionType)
	{
	case bit::PageProtectionType::PROTECTION_TYPE_READ_WRITE: return PAGE_READWRITE;
	case bit::PageProtectionType::PROTECTION_TYPE_READ_ONLY: return PAGE_READONLY;
	}
	return PAGE_READONLY;
}

bit::VirtualAddressSpace::VirtualAddressSpace() :
	BaseAddress(nullptr),
	ReservedSize(0),
	CommittedSize(0)
{
}

bit::VirtualAddressSpace::VirtualAddressSpace(void* BaseAddress, size_t ReservedSize) :
	BaseAddress(BaseAddress),
	ReservedSize(ReservedSize),
	CommittedSize(0)
{}

bit::VirtualAddressSpace::VirtualAddressSpace(VirtualAddressSpace&& Move)
{
	BaseAddress = Move.BaseAddress;
	ReservedSize = Move.ReservedSize;
	CommittedSize = Move.CommittedSize;
	Move.Invalidate();
}

bit::VirtualAddressSpace& bit::VirtualAddressSpace::operator=(VirtualAddressSpace&& Move)
{
	BaseAddress = Move.BaseAddress;
	ReservedSize = Move.ReservedSize;
	CommittedSize = Move.CommittedSize;
	Move.Invalidate();
	return *this;
}

void* bit::VirtualAddressSpace::CommitAll()
{
	void* Pages = VirtualAlloc(BaseAddress, ReservedSize, MEM_COMMIT, PAGE_READWRITE);
	if (Pages != nullptr) CommittedSize = ReservedSize;
	return Pages;
}

bool bit::VirtualAddressSpace::DecommitAll()
{
	if (VirtualFree(BaseAddress, ReservedSize, MEM_DECOMMIT))
	{
		CommittedSize = 0;
		return true;
	}
	return false;
}

void* bit::VirtualAddressSpace::CommitPagesByAddress(void* Address, size_t Size)
{
	if (bit::PtrInRange(Address, BaseAddress, GetEndAddress()) && Size <= ReservedSize)
	{
		void* Pages = VirtualAlloc(Address, Size, MEM_COMMIT, PAGE_READWRITE);
		if (Pages != nullptr) CommittedSize += Size;
		return Pages;
	}
	return nullptr;
}

bool bit::VirtualAddressSpace::DecommitPagesByAddress(void* Address, size_t Size)
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

bool bit::VirtualAddressSpace::ProtectPagesByAddress(void* Address, size_t Size, PageProtectionType Protection)
{
	if (bit::PtrInRange(Address, BaseAddress, GetEndAddress()) && Size <= ReservedSize)
	{
		DWORD OldProtection = 0;
		return VirtualProtect(Address, Size, BitGetProtection(Protection), &OldProtection) == TRUE;
	}
	return false;
}

void* bit::VirtualAddressSpace::CommitPagesByOffset(size_t Offset, size_t Size)
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

bool bit::VirtualAddressSpace::DecommitPagesByOffset(size_t Offset, size_t Size)
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

bool bit::VirtualAddressSpace::ProtectPagesByOffset(size_t Offset, size_t Size, PageProtectionType Protection)
{
	void* Address = GetAddress(Offset);
	if (bit::PtrInRange(Address, BaseAddress, GetEndAddress()) && Size <= ReservedSize)
	{
		DWORD OldProtection = 0;
		return VirtualProtect(Address, Size, BitGetProtection(Protection), &OldProtection) == TRUE;
	}
	return false;
}

void* bit::VirtualAddressSpace::GetBaseAddress() const
{
	return BaseAddress;
}

void* bit::VirtualAddressSpace::GetAddress(size_t Offset) const
{
	return bit::OffsetPtr(BaseAddress, Offset);
}

void* bit::VirtualAddressSpace::GetEndAddress() const
{
	return bit::OffsetPtr(BaseAddress, ReservedSize);
}

size_t bit::VirtualAddressSpace::GetRegionSize() const
{
	return ReservedSize;
}

size_t bit::VirtualAddressSpace::GetCommittedSize() const
{
	return CommittedSize;
}

size_t bit::VirtualAddressSpace::GetPageCount() const
{
	return ReservedSize / bit::GetOSPageSize();
}

bool bit::VirtualAddressSpace::IsValid() const
{ 
	return BaseAddress != nullptr; 
}

void bit::VirtualAddressSpace::Invalidate()
{
	BaseAddress = nullptr;
	ReservedSize = 0;
	CommittedSize = 0;
}

bool bit::VirtualReserveSpace(void* Address, size_t Size, VirtualAddressSpace& OutVirtualMemorySpace)
{
	void* Ptr = VirtualAlloc(Address, Size, MEM_RESERVE, PAGE_READWRITE);
	if (Ptr == nullptr) return false;
	MEMORY_BASIC_INFORMATION MemInfo = {};
	VirtualQuery(Ptr, &MemInfo, sizeof(MEMORY_BASIC_INFORMATION));
	BitPlacementNew(&OutVirtualMemorySpace) VirtualAddressSpace(Ptr, MemInfo.RegionSize);
	return true;
}

void bit::VirtualReleaseSpace(bit::VirtualAddressSpace& VirtualMemoryRegion)
{
	if (VirtualMemoryRegion.IsValid())
	{
		void* Base = VirtualMemoryRegion.GetBaseAddress();
		size_t Size = VirtualMemoryRegion.GetRegionSize();
		VirtualFree(Base, Size, MEM_DECOMMIT);
		VirtualFree(Base, Size, MEM_RELEASE);
	}
}

void* bit::VirtualRandomAddress()
{
	return nullptr;
}