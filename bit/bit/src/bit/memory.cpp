#include <bit/memory.h>

size_t bit::ToKiB(size_t Value) { return Value * 1024; }
size_t bit::ToMiB(size_t Value) { return ToKiB(Value) * 1024; }
size_t bit::ToGiB(size_t Value) { return ToMiB(Value) * 1024; }
size_t bit::ToTiB(size_t Value) { return ToGiB(Value) * 1024; }