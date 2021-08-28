#pragma once

#if defined(_WIN32) || defined(_WIN64)
#include <bit/platform_windows.h>
#else
#include <bit/platform_null.h>
#endif
