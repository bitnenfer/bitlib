#include <bit/core/os/virtual_memory.h>
#include <bit/core/memory.h>
#include <bit/core/os/debug.h>
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

bit::VirtualMemoryBlock::VirtualMemoryBlock() :
	BaseAddress(nullptr),
	ReservedSize(0),
	CommittedSize(0)
{
}

bit::VirtualMemoryBlock::VirtualMemoryBlock(void* BaseAddress, size_t ReservedSize) :
	BaseAddress(BaseAddress),
	ReservedSize(ReservedSize),
	CommittedSize(0)
{}

bit::VirtualMemoryBlock::VirtualMemoryBlock(VirtualMemoryBlock&& Move)
{
	BaseAddress = Move.BaseAddress;
	ReservedSize = Move.ReservedSize;
	CommittedSize = Move.CommittedSize;
	Move.Invalidate();
}

bit::VirtualMemoryBlock& bit::VirtualMemoryBlock::operator=(VirtualMemoryBlock&& Move)
{
	BaseAddress = Move.BaseAddress;
	ReservedSize = Move.ReservedSize;
	CommittedSize = Move.CommittedSize;
	Move.Invalidate();
	return *this;
}

void* bit::VirtualMemoryBlock::CommitAll()
{
	void* Pages = VirtualAlloc(BaseAddress, ReservedSize, MEM_COMMIT, PAGE_READWRITE);
	if (Pages != nullptr) CommittedSize = ReservedSize;
	return Pages;
}

bool bit::VirtualMemoryBlock::DecommitAll()
{
	if (VirtualFree(BaseAddress, ReservedSize, MEM_DECOMMIT))
	{
		CommittedSize = 0;
		return true;
	}
	return false;
}

void* bit::VirtualMemoryBlock::CommitPagesByAddress(void* Address, size_t Size)
{
	if (bit::PtrInRange(Address, BaseAddress, GetEndAddress()) && Size <= ReservedSize)
	{
		void* Pages = VirtualAlloc(Address, Size, MEM_COMMIT, PAGE_READWRITE);
		if (Pages != nullptr) CommittedSize += Size;
		return Pages;
	}
	return nullptr;
}

bool bit::VirtualMemoryBlock::DecommitPagesByAddress(void* Address, size_t Size)
{
	if (bit::PtrInRange(Address, BaseAddress, GetEndAddress()) && Size <= ReservedSize)
	{
		if (VirtualFree(Address, Size, MEM_DECOMMIT))
		{
			CommittedSize -= Size;
			BIT_ASSERT(CommittedSize >= 0);
			return true;
		}
		return false;
	}
	return false;
}

bool bit::VirtualMemoryBlock::ProtectPagesByAddress(void* Address, size_t Size, PageProtectionType Protection)
{
	if (bit::PtrInRange(Address, BaseAddress, GetEndAddress()) && Size <= ReservedSize)
	{
		DWORD OldProtection = 0;
		return VirtualProtect(Address, Size, BitGetProtection(Protection), &OldProtection) == TRUE;
	}
	return false;
}

void* bit::VirtualMemoryBlock::CommitPagesByOffset(size_t Offset, size_t Size)
{
	return CommitPagesByAddress(GetAddress(Offset), Size);
}

bool bit::VirtualMemoryBlock::DecommitPagesByOffset(size_t Offset, size_t Size)
{
	return DecommitPagesByAddress(GetAddress(Offset), Size);
}

bool bit::VirtualMemoryBlock::ProtectPagesByOffset(size_t Offset, size_t Size, PageProtectionType Protection)
{
	return ProtectPagesByAddress(GetAddress(Offset), Size, Protection);
}

void* bit::VirtualMemoryBlock::GetBaseAddress() const
{
	return BaseAddress;
}

void* bit::VirtualMemoryBlock::GetAddress(size_t Offset) const
{
	return bit::OffsetPtr(BaseAddress, Offset);
}

void* bit::VirtualMemoryBlock::GetEndAddress() const
{
	return bit::OffsetPtr(BaseAddress, ReservedSize);
}

size_t bit::VirtualMemoryBlock::GetReservedSize() const
{
	return ReservedSize;
}

size_t bit::VirtualMemoryBlock::GetCommittedSize() const
{
	return CommittedSize;
}

size_t bit::VirtualMemoryBlock::GetPageCount() const
{
	return ReservedSize / bit::GetOSPageSize();
}

bool bit::VirtualMemoryBlock::IsValid() const
{ 
	return BaseAddress != nullptr; 
}

void bit::VirtualMemoryBlock::Invalidate()
{
	BaseAddress = nullptr;
	ReservedSize = 0;
	CommittedSize = 0;
}

bool bit::VirtualMemoryBlock::OwnsAddress(const void* Ptr) const
{
	return bit::PtrInRange(Ptr, GetBaseAddress(), GetEndAddress());
}

bool bit::VirtualAllocateBlock(void* Address, size_t Size, VirtualMemoryBlock& OutVirtualMemoryBlock)
{
	void* Ptr = VirtualAlloc(Address, Size, MEM_RESERVE, PAGE_READWRITE);
	if (Ptr == nullptr) return false;
	MEMORY_BASIC_INFORMATION MemInfo = {};
	VirtualQuery(Ptr, &MemInfo, sizeof(MEMORY_BASIC_INFORMATION));
	BitPlacementNew(&OutVirtualMemoryBlock) VirtualMemoryBlock(Ptr, MemInfo.RegionSize);
	return true;
}

bool bit::VirtualAllocateBlock(size_t Size, VirtualMemoryBlock& OutVirtualMemoryBlock)
{
	return VirtualAllocateBlock(nullptr, Size, OutVirtualMemoryBlock);
}

void bit::VirtualFreeBlock(bit::VirtualMemoryBlock& VirtualMemoryRegion)
{
	if (VirtualMemoryRegion.IsValid())
	{
		void* Base = VirtualMemoryRegion.GetBaseAddress();
		size_t Size = VirtualMemoryRegion.GetReservedSize();
		VirtualFree(Base, Size, MEM_DECOMMIT);
		VirtualFree(Base, Size, MEM_RELEASE);
	}
}
