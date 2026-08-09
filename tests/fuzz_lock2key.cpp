/*
 * Standalone fuzz harness for Lock2Key.
 * The function is self-contained (pure bit manipulation, no project deps).
 *
 * Build: clang++-21 -fsanitize=fuzzer,address -std=c++23 -O2 tests/fuzz_lock2key.cpp -o fuzz_lock2key
 * Run:   ./fuzz_lock2key -max_len=128 -timeout=5
 *
 * Lock format: "$Lock <46 bytes of arbitrary data> PPK|"
 */
#include <cstdint>
#include <cstring>
#include <string>

// Inline copy of Lock2Key from core/utility.cpp to avoid pulling in the entire project
static std::string Lock2KeyInline(const char * sLock)
{
	static constexpr uint8_t ui8LockSize = 46;
	std::string result;
	result.reserve(461);

	sLock = sLock + 6; // skip "$Lock "

	uint8_t v;

	for (uint8_t ui8i = 0; ui8i < ui8LockSize; ui8i++)
	{
		if (ui8i == 0)
		{
			v = static_cast<uint8_t>(sLock[0] ^ sLock[45] ^ sLock[44] ^ 5);
		}
		else
		{
			v = sLock[ui8i] ^ sLock[ui8i - 1];
		}

		v = static_cast<uint8_t>(((v << 4) & 0xF0) | ((v >> 4) & 0x0F));

		if (result.size() >= 460)
		{
			break;
		}

		switch (v)
		{
		case   0: result += "/%DCN000%/"; break;
		case   5: result += "/%DCN005%/"; break;
		case  36: result += "/%DCN036%/"; break;
		case  96: result += "/%DCN096%/"; break;
		case 124: result += "/%DCN124%/"; break;
		case 126: result += "/%DCN126%/"; break;
		default:  result += static_cast<char>(v); break;
		}
	}
	return result;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size < 6 || size > 128)
		return 0;

	// Construct a "$Lock " prefixed string
	char lockBuf[134] = {};
	memcpy(lockBuf, "$Lock ", 6);
	size_t copyLen = (size < 126) ? size : 126;
	memcpy(lockBuf + 6, data, copyLen);

	std::string key = Lock2KeyInline(lockBuf);
	(void)key;

	return 0;
}
