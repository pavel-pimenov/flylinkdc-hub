/*
 * Unit tests for utility.h pure functions.
 * Uses GoogleTest. Build with: cmake --build out/build/default --target test_utility
 */
#include <gtest/gtest.h>
#include "utility.h"

// --- HaveOnlyNumbers ---

TEST(HaveOnlyNumbers, EmptyReturnsTrue)
{
	EXPECT_TRUE(HaveOnlyNumbers("", 0));
}

TEST(HaveOnlyNumbers, ValidDigits)
{
	const char data[] = "1234567890";
	EXPECT_TRUE(HaveOnlyNumbers(data, static_cast<uint16_t>(strlen(data))));
}

TEST(HaveOnlyNumbers, InvalidChars)
{
	const char data[] = "123abc";
	EXPECT_FALSE(HaveOnlyNumbers(data, static_cast<uint16_t>(strlen(data))));
}

TEST(HaveOnlyNumbers, SingleDigit)
{
	const char data[] = "5";
	EXPECT_TRUE(HaveOnlyNumbers(data, 1));
}

// --- safe_stoi ---

TEST(SafeStoi, ValidNumber)
{
	int result = 0;
	EXPECT_TRUE(safe_stoi("42", result));
 EXPECT_EQ(result, 42);
}

TEST(SafeStoi, NegativeNumber)
{
	int result = 0;
	EXPECT_TRUE(safe_stoi("-7", result));
	EXPECT_EQ(result, -7);
}

TEST(SafeStoi, InvalidString)
{
	int result = 0;
	EXPECT_FALSE(safe_stoi("abc", result));
}

TEST(SafeStoi, EmptyString)
{
	int result = 0;
	EXPECT_FALSE(safe_stoi("", result));
}

TEST(SafeStoi, LargeNumber)
{
	int result = 0;
	EXPECT_TRUE(safe_stoi("2147483647", result));
	EXPECT_EQ(result, 2147483647);
}

// --- safe_stoul ---

TEST(SafeStoul, ValidNumber)
{
	unsigned long result = 0;
	EXPECT_TRUE(safe_stoul("12345", result));
	EXPECT_EQ(result, 12345UL);
}

TEST(SafeStoul, InvalidString)
{
	unsigned long result = 0;
	EXPECT_FALSE(safe_stoul("xyz", result));
}

// --- IsV4Mapped ---

TEST(IsV4Mapped, V4MappedAddress)
{
	uint8_t hash[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 192, 168, 1, 1 };
	EXPECT_TRUE(IsV4Mapped(hash));
}

TEST(IsV4Mapped, PureIPv6)
{
	uint8_t hash[16] = { 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
	EXPECT_FALSE(IsV4Mapped(hash));
}

TEST(IsV4Mapped, AllZeros)
{
	uint8_t hash[16] = {};
	EXPECT_FALSE(IsV4Mapped(hash));
}

// --- Allign ---

TEST(Allign, SmallValue)
{
	EXPECT_EQ(Allign(10), 16);   // 10 + 5 + 1
}

TEST(Allign, Zero)
{
	EXPECT_EQ(Allign(0), 1);     // 0 + 0 + 1
}

TEST(Allign, LargeValue)
{
	EXPECT_EQ(Allign(1000), 1501); // 1000 + 500 + 1
}

TEST(Allign, One)
{
	EXPECT_EQ(Allign(1), 2);     // 1 + 0 + 1
}

// --- MatchBytes ---

TEST(MatchBytes, MatchesLiteral)
{
	const char buf[] = "Hello World";
	EXPECT_TRUE(MatchBytes(buf, "Hello"));
}

TEST(MatchBytes, NoMatch)
{
	const char buf[] = "Hello World";
	EXPECT_FALSE(MatchBytes(buf, "World"));
}

TEST(MatchBytes, EmptyLiteral)
{
	const char buf[] = "Hello";
	EXPECT_TRUE(MatchBytes(buf, ""));
}

// --- MatchU16 ---

TEST(MatchU16, MatchesValue)
{
	const char buf[] = "\x39\x05";  // 0x0539 = 1337 in little-endian
	EXPECT_TRUE(MatchU16(buf, 0, 0x0539));
}

TEST(MatchU16, NoMatch)
{
	const char buf[] = "\x39\x05";
	EXPECT_FALSE(MatchU16(buf, 0, 0x1234));
}

// --- MatchU32 ---

TEST(MatchU32, MatchesValue)
{
	const char buf[] = "\x01\x02\x03\x04";
	EXPECT_TRUE(MatchU32(buf, 0, 0x04030201));
}

TEST(MatchU32, NoMatch)
{
	const char buf[] = "\x01\x02\x03\x04";
	EXPECT_FALSE(MatchU32(buf, 0, 0x00000001));
}

// --- MatchU64 ---

TEST(MatchU64, MatchesValue)
{
	const char buf[] = "\x01\x02\x03\x04\x05\x06\x07\x08";
	EXPECT_TRUE(MatchU64(buf, 0, 0x0807060504030201ULL));
}

TEST(MatchU64, NoMatch)
{
	const char buf[] = "\x01\x02\x03\x04\x05\x06\x07\x08";
	EXPECT_FALSE(MatchU64(buf, 0, 0x0000000000000001ULL));
}

// --- px_str ---

TEST(PxStr, FromString)
{
	std::string s = "hello";
	EXPECT_EQ(px_str(s), "hello");
}

TEST(PxStr, FromCString)
{
	EXPECT_EQ(px_str("world"), "world");
}

TEST(PxStr, FromNullCString)
{
	const char* p = nullptr;
	EXPECT_EQ(px_str(p), "");
}

TEST(PxStr, FromCStringWithLen)
{
	EXPECT_EQ(px_str("hello", 3), "hel");
}

TEST(PxStr, FromNullCStringWithLen)
{
	const char* p = nullptr;
	EXPECT_EQ(px_str(p, 5), "");
}

// --- CalcHash ---

TEST(CalcHash, ReturnsLower16Bits)
{
	uint32_t val = 0xAABBCCDD;
	uint16_t result = CalcHash(val);
	EXPECT_EQ(result, 0xCCDD);
}

TEST(CalcHash, Zero)
{
	EXPECT_EQ(CalcHash(0), 0);
}

TEST(CalcHash, MaxUint32)
{
	EXPECT_EQ(CalcHash(0xFFFFFFFF), 0xFFFF);
}

// --- SnprintfAppend ---

TEST(SnprintfAppend, BasicAppend)
{
	char buf[64] = {};
	int offset = 0;
	EXPECT_TRUE(SnprintfAppend(buf, offset, sizeof(buf), "Hello %s", "World"));
	EXPECT_STREQ(buf, "Hello World");
	EXPECT_EQ(offset, 11);
}

TEST(SnprintfAppend, MultipleAppends)
{
	char buf[64] = {};
	int offset = 0;
	EXPECT_TRUE(SnprintfAppend(buf, offset, sizeof(buf), "Hello "));
	EXPECT_TRUE(SnprintfAppend(buf, offset, sizeof(buf), "World"));
	EXPECT_STREQ(buf, "Hello World");
	EXPECT_EQ(offset, 11);
}

TEST(SnprintfAppend, BufferOverflowTruncates)
{
	char buf[8] = {};
	int offset = 0;
	// vsnprintf truncates to fit, returns chars that WOULD have been written
	EXPECT_TRUE(SnprintfAppend(buf, offset, sizeof(buf), "This is a very long string that exceeds buffer"));
	EXPECT_STREQ(buf, "This is");  // truncated to 7 chars + null
}

// --- Time constants ---

TEST(TimeConstants, Consistency)
{
	EXPECT_EQ(SECONDS_PER_MINUTE, 60U);
	EXPECT_EQ(SECONDS_PER_HOUR, 3600U);
	EXPECT_EQ(SECONDS_PER_DAY, 86400U);
	EXPECT_EQ(SECONDS_PER_MONTH, 2592000U);
	EXPECT_EQ(SECONDS_PER_YEAR, 31536000U);
	EXPECT_EQ(MINUTES_PER_HOUR, 60U);
	EXPECT_EQ(MINUTES_PER_DAY, 1440U);
	EXPECT_EQ(MINUTES_PER_MONTH, 43200U);
	EXPECT_EQ(MINUTES_PER_YEAR, 525600U);
}

TEST(TimeConstants, HourEquals60Minutes)
{
	EXPECT_EQ(SECONDS_PER_HOUR, MINUTES_PER_HOUR * SECONDS_PER_MINUTE);
}

TEST(TimeConstants, DayEquals24Hours)
{
	EXPECT_EQ(SECONDS_PER_DAY, 24 * SECONDS_PER_HOUR);
}

// --- HaveOnlyNumbers edge cases ---

TEST(HaveOnlyNumbers, AllSameDigit)
{
	const char data[] = "11111";
	EXPECT_TRUE(HaveOnlyNumbers(data, 5));
}

TEST(HaveOnlyNumbers, SpaceIsInvalid)
{
	const char data[] = "12 34";
	EXPECT_FALSE(HaveOnlyNumbers(data, 5));
}

TEST(HaveOnlyNumbers, NullCharIsInvalid)
{
	// Note: isdigit('\0') returns 0
	const char data[] = "12\034";
	EXPECT_FALSE(HaveOnlyNumbers(data, 5));
}

// --- safe_stoi edge cases ---

TEST(SafeStoi, LeadingZeros)
{
	int result = 0;
	EXPECT_TRUE(safe_stoi("007", result));
	EXPECT_EQ(result, 7);
}

TEST(SafeStoi, MaxInt)
{
	int result = 0;
	EXPECT_TRUE(safe_stoi("2147483647", result));
	EXPECT_EQ(result, 2147483647);
}

TEST(SafeStoi, OverflowFails)
{
	int result = 0;
	EXPECT_FALSE(safe_stoi("99999999999999", result));
}

// --- MatchBytes edge cases ---

TEST(MatchBytes, ExactMatch)
{
	const char buf[] = "AB";
	EXPECT_TRUE(MatchBytes(buf, "AB"));
}

TEST(MatchBytes, LongerLiteral)
{
	const char buf[] = "AB";
	EXPECT_FALSE(MatchBytes(buf, "ABC"));
}

// --- MatchU16 edge cases ---

TEST(MatchU16, ZeroValue)
{
	const char buf[] = "\x00\x00";
	EXPECT_TRUE(MatchU16(buf, 0, 0));
}

TEST(MatchU16, MaxValue)
{
	const char buf[] = "\xff\xff";
	EXPECT_TRUE(MatchU16(buf, 0, 0xFFFF));
}

// --- MatchU32 edge cases ---

TEST(MatchU32, ZeroValue)
{
	const char buf[] = "\x00\x00\x00\x00";
	EXPECT_TRUE(MatchU32(buf, 0, 0));
}

TEST(MatchU32, MaxValue)
{
	const char buf[] = "\xff\xff\xff\xff";
	EXPECT_TRUE(MatchU32(buf, 0, 0xFFFFFFFF));
}

// --- Allign growth factor ---

TEST(Allign, GrowthFactor)
{
	// Allign should grow by ~50%
	size_t prev = Allign(100);
	EXPECT_GT(prev, static_cast<size_t>(150));  // 100 + 50 + 1 = 151
	EXPECT_LT(prev, static_cast<size_t>(160));

	prev = Allign(1000);
	EXPECT_GT(prev, static_cast<size_t>(1500));
	EXPECT_LT(prev, static_cast<size_t>(1600));
}

TEST(Allign, MonotonicallyIncreasing)
{
	for (size_t i = 1; i < 1000; i++)
	{
		EXPECT_GT(Allign(i), i) << "Allign(" << i << ") should be > " << i;
	}
}
