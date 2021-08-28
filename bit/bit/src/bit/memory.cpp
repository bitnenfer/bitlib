#include <bit/memory.h>

size_t bit::ToKiB(size_t Bytes) { return Bytes * 1024; }
size_t bit::ToMiB(size_t Bytes) { return ToKiB(Bytes) * 1024; }
size_t bit::ToGiB(size_t Bytes) { return ToMiB(Bytes) * 1024; }
size_t bit::ToTiB(size_t Bytes) { return ToGiB(Bytes) * 1024; }