#pragma once

#include <bit/core/types.h>

namespace bit
{
	enum class ProcessorArch
	{
		PROC_ARCH_UNKNOWN,
		PROC_ARCH_X64,
		PROC_ARCH_ARM,
		PROC_ARCH_ARM64,
		PROC_ARCH_IA64,
		PROC_ARCH_X86
	};

	BITLIB_API void ExitProgram(int32_t ExitCode);
	BITLIB_API double GetSeconds();
	BITLIB_API int32_t GetOSErrorCode();
	BITLIB_API size_t GetOSPageSize();
	BITLIB_API size_t GetOSAllocationGranularity();
	BITLIB_API void* GetOSMinAddress();
	BITLIB_API void* GetOSMaxAddress();
	BITLIB_API int32_t GetOSProcessorCount();
	BITLIB_API ProcessorArch GetOSProcessorArch();
}
