/*
 * Fuzz harness for utility.h pure functions.
 * Build: clang++-21 -fsanitize=fuzzer,address -std=c++23 -I core tests/fuzz_utility.cpp -o fuzz_utility
 * Run:   ./fuzz_utility -max_len=256 -timeout=5
 */
#include <cstdint>
#include <cstring>
#include <array>
#include "utility.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size == 0 || size > 256)
		return 0;

	const char *buf = reinterpret_cast<const char *>(data);

	(void)HaveOnlyNumbers(buf, static_cast<uint16_t>(size));

	if (size < 32)
	{
		char tmp[32] = {};
		memcpy(tmp, data, size);
		int iResult = 0;
		(void)safe_stoi(tmp, iResult);
		unsigned long ulResult = 0;
		(void)safe_stoul(tmp, ulResult);
	}

	if (size >= 16)
	{
		(void)IsV4Mapped(data);
	}

	if (size >= sizeof(size_t))
	{
		size_t n;
		memcpy(&n, data, sizeof(n));
		(void)Allign(n);
	}

	if (size >= 2)
	{
		(void)MatchU16(buf, 0, *reinterpret_cast<const uint16_t *>(data));
	}
	if (size >= 4)
	{
		(void)MatchU32(buf, 0, *reinterpret_cast<const uint32_t *>(data));
	}
	if (size >= 8)
	{
		(void)MatchU64(buf, 0, *reinterpret_cast<const uint64_t *>(data));
	}

	return 0;
}
