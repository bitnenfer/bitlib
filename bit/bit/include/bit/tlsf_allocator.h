#include <bit/allocator.h>

namespace bit
{
	/* Two-Level Segregated Fit Allocator */
	/* Source: http://www.gii.upv.es/tlsf/files/ecrts04_tlsf.pdf */
	struct CTLSFAllocator : public bit::IAllocator
	{
		void* Allocate(size_t Size, size_t Alignment) override;
		void* Reallocate(void* Pointer, size_t Size, size_t Alignment) override;
		void Free(void* Pointer) override;
		size_t GetSize(void* Pointer) override;
		CMemoryUsageInfo GetMemoryUsageInfo() override;
	};
}
