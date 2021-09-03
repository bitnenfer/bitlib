#pragma once

#include <bit/allocator.h>
#include <bit/utils.h>

namespace bit
{
	enum class EVirtualMemoryProtection : uint8_t
	{
		PROTECTION_NONE			= 0b01,
		PROTECTION_READONLY		= 0b10,
		PROTECTION_READWRITE	= 0b11
	};

	struct CVirtualMemoryBlock
	{
		friend struct CVirtualMemoryAllocator;

		void* GetStartAddress() const { return StartAddress; }
		void* GetEndAddress() const { return EndAddress; }
		size_t GetSize() const { return bit::PtrDiff(StartAddress, EndAddress); }
		bool IsCommited() const { return (Flags & 0b1) > 0; }
		EVirtualMemoryProtection GetProtection() const { return (EVirtualMemoryProtection)((Flags >> 1) & 0b11); }

	private:
		void* StartAddress = nullptr;
		void* EndAddress = nullptr;
		uint8_t Flags = 0; // bit 0 indicates if range is commited. Bits 1 and 2 are for protection
	};

	struct CVirtualMemoryAllocator : public bit::IAllocator
	{
		// Inherited via IAllocator
		virtual void* Allocate(size_t Size, size_t Alignment) override;
		virtual void* Reallocate(void* Pointer, size_t Size, size_t Alignment) override;
		virtual void Free(void* Pointer) override;
		virtual size_t GetSize(void* Pointer) override;
		virtual CMemoryInfo GetMemoryInfo() override;

		size_t GetPageSize() const;
		size_t GetAllocationGranularity() const;

		CVirtualMemoryBlock AllocateVirtualMemory(void* Address, size_t Size, bool bCommited);
		bool CommitVirtualMemory(CVirtualMemoryBlock& VirtualMemoryBlock);
		bool DecommitVirtualMemory(CVirtualMemoryBlock& VirtualMemoryBlock);
		bool ProtectVirtualMemory(CVirtualMemoryBlock& VirtualMemoryBlock, EVirtualMemoryProtection Protection);
	};
}
