#pragma once
#include <stdint.h>

#define CRC32_NEGL 0xffffffff

class Crc32
{
protected:

	static const uint32_t crc32tab[256];

protected:

	uint32_t crc;

public:

	Crc32(void)
	{
		Reset();
	}

	void Set(uint32_t u)
	{
		crc = u ^ CRC32_NEGL;
	}

	void Reset(void)
	{
		crc = CRC32_NEGL;
	}

	void Update(const void* s, uint64_t n)
	{
		uint32_t c = crc;
		unsigned char* d = (unsigned char*)s;
		while (n--)
			c = crc32tab[unsigned char(c ^ *d++)] ^ (c >> 8);
		crc = c;
	}

	uint32_t GetCrc(void)
	{
		return crc ^ CRC32_NEGL;
	}
};

class xorshift64
{
public:

	explicit xorshift64(uint64_t seed1 = 0)
	{
		seed(seed1);
	}

	void seed(uint64_t seed1 = 0)
	{
		u = uint64_t(15191868757011070976) ^ seed1;
		w = uint64_t(0x61C8864680B583EB) ^ seed1;
	}

	// Xorshift + Weyl Generator + some extra magic for low bits
	uint64_t get_raw()
	{
		u ^= (u << 5); u ^= (u >> 15); u ^= (u << 27);
		w += uint64_t(0x61C8864680B583EB);
		return u + (w ^ (w >> 27));
	}

	uint64_t get_uint64()
	{
		return get_raw();
	}

	// Uniform [0,n) with 64-bits of precision
	// If n is a power of two, this only uses not-well-distributed low-bits
	uint64_t get_uint64(uint64_t n) {
		return get_uint64() % n;
	}

	// Uniform [0,n) exactly
	// This uses low bits which are not very random
	uint64_t get_uint64x(uint64_t n)
	{
		// Find a mask
		uint64_t v = n;
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v |= v >> 32;
		uint64_t u;
		do {
			u = get_uint64() & v;
		} while (u >= n);
		return u;
	}

	// Uniform [0,n) exactly
	// This uses high bits, but might not be as fast as get_uint64x
	uint64_t get_uint64h(uint64_t n)
	{
		// Find a shift
		uint64_t v = n - 1;
		int r = 64;
		while (v >>= 1) {
			r--;
		}
		uint64_t u;
		do {
			u = get_uint64() >> r;
		} while (u >= n);
		return u;
	}

	double get_double53()
	{
		uint64_t u = get_raw();
		int64_t n = static_cast<int64_t>(u >> 11);
		return double(n) / 9007199254740992.0;
	}

	double get_double52()
	{
		uint64_t u = get_raw();
		int64_t n = static_cast<int64_t>(u >> 11) | 0x1;
		return double(n) / 9007199254740992.0;
	}

	// Uniform [0,max)
	uint64_t operator()() {
		return get_uint64();
	}

	// Uniform [0,n) with 64-bits of precision
	uint64_t operator()(uint64_t n) {
		return get_uint64(n);
	}

	uint64_t u, w;
};

class Random64
{
public:

	Random64()
	{
	}

	void Init(uint64_t s)
	{
		iset = false;
		_xorRnd.seed(s);
	}

	/// @brief Generates a random integer in the range [from, to)
	/// @param from 
	/// @param to 
	/// @return 
	int64_t GetRandInt(int64_t from, int64_t to)
	{
		const double randFlt = Get01();
		const uint64_t randInt = *(uint64_t*)(&randFlt);
		return from + randInt % (to - from);
	}

	double Get11()
	{
		return (Get01() * 2.0) - 1.0;
	}

	double Get01()
	{
		return _xorRnd.get_double53();
	}

	uint32_t GetSeed()
	{
		Crc32 crc;

		crc.Update(&_xorRnd.u, sizeof(_xorRnd.u));
		crc.Update(&_xorRnd.w, sizeof(_xorRnd.w));

		return crc.GetCrc();
	}

private:
	bool iset;
	xorshift64 _xorRnd;
};