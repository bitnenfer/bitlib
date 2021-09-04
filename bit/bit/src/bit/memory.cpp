#include <bit/memory.h>

size_t bit::ToKiB(size_t Value) { return Value * 1024; }
size_t bit::ToMiB(size_t Value) { return ToKiB(Value) * 1024; }
size_t bit::ToGiB(size_t Value) { return ToMiB(Value) * 1024; }
size_t bit::ToTiB(size_t Value) { return ToGiB(Value) * 1024; }

double bit::FromKiB(size_t Value) { return (double)Value / 1024.0; }
double bit::FromMiB(size_t Value) { return FromKiB(Value) / 1024.0; }
double bit::FromGiB(size_t Value) { return FromMiB(Value) / 1024.0; }
double bit::FromTiB(size_t Value) { return FromGiB(Value) / 1024.0; }
