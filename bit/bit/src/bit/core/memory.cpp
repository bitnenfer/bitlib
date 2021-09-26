#include <bit/core/memory.h>

double bit::FromKiB(size_t Value) { return (double)Value / 1024.0; }
double bit::FromMiB(size_t Value) { return FromKiB(Value) / 1024.0; }
double bit::FromGiB(size_t Value) { return FromMiB(Value) / 1024.0; }
double bit::FromTiB(size_t Value) { return FromGiB(Value) / 1024.0; }
