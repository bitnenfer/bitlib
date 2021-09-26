#include <bit/utility/murmur_hash.h>

// Source by Bitsquid
// http://bitsquid.blogspot.com/2011/08/code-snippet-murmur-hash-inverse-pre.html

size_t bit::MurmurHash(const void* Key, size_t Len, size_t Seed)
{
#if BIT_PLATFORM_X64
	const uint64_t m = 0xc6a4a7935bd1e995ULL;
	const uint32_t r = 47;

	uint64_t h = Seed ^ (Len * m);

	const uint64_t* data = (const uint64_t*)Key;
	const uint64_t* end = data + (Len / 8);

	while (data != end)
	{
		uint64_t k = *data++;
		k *= m;
		k ^= k >> r;
		k *= m;
		h ^= k;
		h *= m;
	}

	const unsigned char* data2 = (const unsigned char*)data;

	switch (Len & 7)
	{
	case 7: h ^= uint64_t(data2[6]) << 48;
	case 6: h ^= uint64_t(data2[5]) << 40;
	case 5: h ^= uint64_t(data2[4]) << 32;
	case 4: h ^= uint64_t(data2[3]) << 24;
	case 3: h ^= uint64_t(data2[2]) << 16;
	case 2: h ^= uint64_t(data2[1]) << 8;
	case 1: h ^= uint64_t(data2[0]);
		h *= m;
	};

	h ^= h >> r;
	h *= m;
	h ^= h >> r;

	return h;
#elif BIT_PLATFORM_X86
	const unsigned int m = 0x5bd1e995;
	const int r = 24;
	unsigned int h = Seed ^ Len;
	const unsigned char* data = (const unsigned char*)Key;
	while (Len >= 4)
	{
		unsigned int k = *(unsigned int*)data;

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		data += 4;
		Len -= 4;
	}
	switch (Len)
	{
	case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0];
		h *= m;
	};
	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;
	return h;
#endif
}
