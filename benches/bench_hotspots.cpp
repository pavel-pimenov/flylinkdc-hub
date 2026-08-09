// Benchmark: IpP2Country::Find (linear vs binary) and isFlooder (list vs hashmap)
// Builds standalone: g++ -O2 -std=c++20 -o bench_hotspots bench_hotspots.cpp -I../core

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <random>
#include <string>
#include <vector>

// ─── Shared constants ──────────────────────────────────────────────────────

static constexpr uint32_t IP_BYTE3_SHIFT = 16777216;
static constexpr uint32_t IP_BYTE2_SHIFT = 65536;
static constexpr uint32_t IP_BYTE1_SHIFT = 256;
static constexpr size_t COUNTRY_COUNT = 254;

// ─── IP2Country data (loaded from CSV) ─────────────────────────────────────

static std::vector<uint32_t> g_RangeFrom, g_RangeTo;
static std::vector<uint8_t> g_RangeCI;
static uint32_t g_Count = 0;

static void LoadCSV(const char * path)
{
	std::ifstream f(path);
	if (!f.is_open()) { fprintf(stderr, "Cannot open %s\n", path); return; }

	g_RangeFrom.resize(210000);
	g_RangeTo.resize(210000);
	g_RangeCI.resize(210000);

	std::string line;
	while (std::getline(f, line))
	{
		if (line.empty() || line[0] != '\"') continue;

		const char * s = line.data() + 1;
		uint8_t field = 0;
		for (size_t i = 1; i < line.size(); i++)
		{
			if (line[i] == '\"')
			{
				line[i] = '\0';
				if (field == 0) g_RangeFrom[g_Count] = strtoul(s, nullptr, 10);
				else if (field == 1) g_RangeTo[g_Count] = strtoul(s, nullptr, 10);
				else if (field == 4)
				{
					g_RangeCI[g_Count] = 0;
					g_Count++;
					break;
				}
				field++;
				i += 2;
				s = line.data() + i + 1;
			}
		}
	}
	g_RangeFrom.resize(g_Count);
	g_RangeTo.resize(g_Count);
	g_RangeCI.resize(g_Count);
	fprintf(stderr, "Loaded %u IPv4 ranges\n", g_Count);
}

// ─── Generate random IPs for benchmarking ──────────────────────────────────

static std::vector<uint32_t> GenerateRandomIPs(size_t n)
{
	std::mt19937 rng(42);
	std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);
	std::vector<uint32_t> ips(n);
	for (auto & ip : ips) ip = dist(rng);
	return ips;
}

// ─── Old: Linear search (original code) ────────────────────────────────────

static uint32_t FindLinear(uint32_t ip)
{
	for (uint32_t i = 0; i < g_Count; i++)
	{
		if (g_RangeFrom[i] <= ip && g_RangeTo[i] >= ip)
			return g_RangeCI[i];
	}
	return COUNTRY_COUNT - 1;
}

// ─── New: Binary search (optimized code) ───────────────────────────────────

static uint32_t FindBinary(uint32_t ip)
{
	auto it = std::upper_bound(g_RangeFrom.begin(), g_RangeFrom.begin() + g_Count, ip);
	if (it != g_RangeFrom.begin())
	{
		auto idx = static_cast<uint32_t>(std::distance(g_RangeFrom.begin(), it) - 1);
		if (g_RangeTo[idx] >= ip)
			return g_RangeCI[idx];
	}
	return COUNTRY_COUNT - 1;
}

// ─── AntiFlood benchmark ───────────────────────────────────────────────────

#include <list>
#include <memory>
#include <unordered_map>

struct FakeFloodEntry
{
	uint64_t m_ui64Time = 0;
	int16_t m_ui16Hits = 0;
	uint8_t m_ui128IpHash[16];

	FakeFloodEntry(const uint8_t * h) : m_ui64Time(1000), m_ui16Hits(1)
	{
		memcpy(m_ui128IpHash, h, 16);
	}
};

struct FakeFloodKey
{
	uint8_t m_ui128IpHash[16];
	FakeFloodKey(const uint8_t * h) { memcpy(m_ui128IpHash, h, 16); }
};

struct FakeFloodKeyHash
{
	size_t operator()(const FakeFloodKey & k) const noexcept
	{
		size_t h = 14695981039346656037ULL;
		for (int i = 0; i < 16; i++)
		{
			h ^= k.m_ui128IpHash[i];
			h *= 1099511628211ULL;
		}
		return h;
	}
};

struct FakeFloodKeyEqual
{
	bool operator()(const FakeFloodKey & a, const FakeFloodKey & b) const noexcept
	{
		return memcmp(a.m_ui128IpHash, b.m_ui128IpHash, 16) == 0;
	}
};

// Old: linear list search
static bool FloodLookupList(std::list<std::unique_ptr<FakeFloodEntry>> & lst, const uint8_t * ip)
{
	for (auto & e : lst)
	{
		if (memcmp(ip, e->m_ui128IpHash, 16) == 0)
			return true;
	}
	return false;
}

// New: hash map lookup
static bool FloodLookupMap(std::unordered_map<FakeFloodKey, FakeFloodEntry, FakeFloodKeyHash, FakeFloodKeyEqual> & mp, const uint8_t * ip)
{
	return mp.find(FakeFloodKey(ip)) != mp.end();
}

// ─── Main ──────────────────────────────────────────────────────────────────

int main(int argc, char ** argv)
{
	const char * csvPath = (argc > 1) ? argv[1] : "../cfg/IpToCountry.csv";
	const size_t NUM_IPS = 100000;
	const size_t NUM_LOOKUPS = 1000000;

	LoadCSV(csvPath);
	if (g_Count == 0) { fprintf(stderr, "No data loaded, exiting\n"); return 1; }

	auto ips = GenerateRandomIPs(NUM_IPS);

	// ─── Benchmark 1: IpP2Country Find ────────────────────────────────

	printf("=== IpP2Country::Find (%u ranges, %zu lookups) ===\n", g_Count, NUM_LOOKUPS);

	// Linear
	volatile uint32_t dummy1 = 0;
	auto t1_start = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < NUM_LOOKUPS; i++)
	{
		dummy1 += FindLinear(ips[i % NUM_IPS]);
	}
	auto t1_end = std::chrono::high_resolution_clock::now();
	double ms_linear = std::chrono::duration<double, std::milli>(t1_end - t1_start).count();

	// Binary
	volatile uint32_t dummy2 = 0;
	auto t2_start = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < NUM_LOOKUPS; i++)
	{
		dummy2 += FindBinary(ips[i % NUM_IPS]);
	}
	auto t2_end = std::chrono::high_resolution_clock::now();
	double ms_binary = std::chrono::duration<double, std::milli>(t2_end - t2_start).count();

	printf("  Linear:  %8.2f ms  (%7.1f ns/lookup)\n", ms_linear, ms_linear * 1e6 / NUM_LOOKUPS);
	printf("  Binary:  %8.2f ms  (%7.1f ns/lookup)\n", ms_binary, ms_binary * 1e6 / NUM_LOOKUPS);
	printf("  Speedup: %.1fx\n\n", ms_linear / ms_binary);

	// Verify correctness
	uint32_t mismatch = 0;
	for (auto ip : ips)
	{
		if (FindLinear(ip) != FindBinary(ip)) mismatch++;
	}
	printf("  Correctness: %zu IPs tested, %u mismatches\n\n", ips.size(), mismatch);

	// ─── Benchmark 2: isFlooder lookup ────────────────────────────────

	const size_t NUM_FLOOD_ENTRIES = 1000;
	const size_t NUM_FLOOD_LOOKUPS = 100000;

	printf("=== isFlooder lookup (%zu entries, %zu lookups) ===\n", NUM_FLOOD_ENTRIES, NUM_FLOOD_LOOKUPS);

	std::mt19937 rng(123);
	std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);

	// Generate lookup IPs: 50% existing, 50% new
	std::vector<uint8_t> existingIPs(NUM_FLOOD_ENTRIES * 16);
	std::vector<uint8_t> lookupIPs(NUM_FLOOD_LOOKUPS * 16);

	for (size_t i = 0; i < NUM_FLOOD_ENTRIES; i++)
	{
		uint32_t ip = dist(rng);
		memset(&existingIPs[i * 16], 0, 10);
		existingIPs[i * 16 + 10] = 255;
		existingIPs[i * 16 + 11] = 255;
		memcpy(&existingIPs[i * 16 + 12], &ip, 4);
	}

	for (size_t i = 0; i < NUM_FLOOD_LOOKUPS; i++)
	{
		if (i % 2 == 0)
		{
			// Existing IP
			size_t idx = rng() % NUM_FLOOD_ENTRIES;
			memcpy(&lookupIPs[i * 16], &existingIPs[idx * 16], 16);
		}
		else
		{
			// New IP
			uint32_t ip = dist(rng);
			memset(&lookupIPs[i * 16], 0, 10);
			lookupIPs[i * 16 + 10] = 255;
			lookupIPs[i * 16 + 11] = 255;
			memcpy(&lookupIPs[i * 16 + 12], &ip, 4);
		}
	}

	// Old: list
	{
		std::list<std::unique_ptr<FakeFloodEntry>> lst;
		for (size_t i = 0; i < NUM_FLOOD_ENTRIES; i++)
			lst.push_back(std::make_unique<FakeFloodEntry>(&existingIPs[i * 16]));

		volatile uint32_t hits = 0;
		auto t_start = std::chrono::high_resolution_clock::now();
		for (size_t i = 0; i < NUM_FLOOD_LOOKUPS; i++)
			hits += FloodLookupList(lst, &lookupIPs[i * 16]) ? 1 : 0;
		auto t_end = std::chrono::high_resolution_clock::now();
		double ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
		printf("  List (O(n)):     %8.2f ms  (%7.1f ns/lookup)  hits=%u\n",
		       ms, ms * 1e6 / NUM_FLOOD_LOOKUPS, hits);
	}

	// New: hashmap
	{
		std::unordered_map<FakeFloodKey, FakeFloodEntry, FakeFloodKeyHash, FakeFloodKeyEqual> mp;
		for (size_t i = 0; i < NUM_FLOOD_ENTRIES; i++)
			mp.emplace(FakeFloodKey(&existingIPs[i * 16]), FakeFloodEntry(&existingIPs[i * 16]));

		volatile uint32_t hits = 0;
		auto t_start = std::chrono::high_resolution_clock::now();
		for (size_t i = 0; i < NUM_FLOOD_LOOKUPS; i++)
			hits += FloodLookupMap(mp, &lookupIPs[i * 16]) ? 1 : 0;
		auto t_end = std::chrono::high_resolution_clock::now();
		double ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
		printf("  HashMap (O(1)):  %8.2f ms  (%7.1f ns/lookup)  hits=%u\n\n",
		       ms, ms * 1e6 / NUM_FLOOD_LOOKUPS, hits);
	}

	return 0;
}
