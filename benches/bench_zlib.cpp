#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <zlib.h>

#if defined(ZLIBNG_VERSION)
static const char * g_sBenchName = "zlib-ng (SIMD)";
#else
static const char * g_sBenchName = "system zlib";
#endif

// Repeat pattern to build larger dataset
static void RepeatAppend(std::vector<char> & data, const std::vector<char> & src, size_t targetBytes)
{
	while (data.size() < targetBytes)
	{
		data.insert(data.end(), src.begin(), src.end());
	}
	data.resize(targetBytes);
}

static std::vector<char> PrepareHubData()
{
	const size_t TARGET_SIZE = 2 * 1024 * 1024; // 2 MB

	std::vector<char> data;
	data.reserve(TARGET_SIZE);

	// Simulate realistic hub traffic: MyINFO, chat messages, search requests
	const char * nicks[] = {
		"User_Alpha", "User_Beta", "User_Gamma", "User_Delta", "User_Epsilon",
		"[fly]TestUser_1", "[fly]TestUser_2", "fantom.ul", "winterman745",
		"RJ-45", "Shemeleff-1-", "Kosta_2022", "krasnotal__R231",
		"DCpinger_6dtP0E", "Hawaj", "Cow_lD5"
	};

	const char * tags[] = {
		"<FlylinkDC++ V:504.22782-x64,M:P,H:19/0/1,S:20>",
		"<FlylinkDC++ V:601.23416-x64,M:P,H:5/0/5,S:10>",
		"<StrgDC++ V:2.42,M:A,H:5/0/1,S:6>",
		"<ApexDC++ V:1.0.0,M:A,H:3/0/0,S:3>",
		"<DC++ V:0.882,M:A,H:1/0/0,S:1>",
	};

	const char * shareInfo[] = {
		"$ $100\x01$$",
		"$ $10.5\x01$$",
		"$ $1.2\x01$$",
		"$ $500\x01$$",
		"$ $0.01\x01$$",
	};

	const char * chatMessages[] = {
		"Hello everyone! Can someone help me with a rare file?",
		"Looking for 1990s documentary about space exploration",
		"Welcome to the hub! Please read the rules before downloading.",
		"Sharing my collection of classic movies - over 500 titles!",
		"Does anyone have the latest Ubuntu ISO? Looking for 24.04.",
		"Slot ratio is 1:3, please share files to keep the hub alive!",
		"Hub supports ZPipe compression for faster download speeds.",
		"Remember to use TTH search for better and faster results.",
		"Good night everyone! See you tomorrow with more shares.",
	};

	const char * searchPatterns[] = {
		"TTH:ABCDEF1234567890ABCDEF1234567890ABCDEF12",
		"ubuntu-24.04-desktop-amd64.iso",
		"TTH:0987654321FEDCBA0987654321FEDCBA09876543",
		"Rick Astley - Never Gonna Give You Up.mp3",
		"Star Wars Episode IV - A New Hope (1977).mkv",
	};

	// Build a representative sample of hub traffic
	std::vector<char> sample;
	sample.reserve(128 * 1024);

	// ~50% MyINFO messages (most common traffic)
	for (const char * nick : nicks)
	{
		for (const char * tag : tags)
		{
			for (const char * share : shareInfo)
			{
				std::string myinfo = std::string("$MyINFO $ALL ") + nick + " " + tag + " " + share + "|";
				sample.insert(sample.end(), myinfo.begin(), myinfo.end());
			}
		}
	}

	// ~30% chat messages
	for (const char * nick : nicks)
	{
		for (const char * msg : chatMessages)
		{
			std::string chat = std::string("<") + nick + "> " + msg + "|";
			sample.insert(sample.end(), chat.begin(), chat.end());
		}
	}

	// ~20% search requests
	for (const char * nick : nicks)
	{
		for (const char * pattern : searchPatterns)
		{
			std::string search = std::string("$Search ") + nick + " " + pattern + "|";
			sample.insert(sample.end(), search.begin(), search.end());
		}
	}

	// Repeat sample to reach target size
	RepeatAppend(data, sample, TARGET_SIZE);
	return data;
}

static bool DeflateData(const char * inData, size_t inLen, std::vector<char> & outBuf, size_t & outLen)
{
	z_stream stream;
	memset(&stream, 0, sizeof(stream));
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.data_type = Z_TEXT;

	int ret = deflateInit(&stream, 8);
	if (ret != Z_OK)
		return false;

	stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(inData));
	stream.avail_in = static_cast<uInt>(inLen);
	stream.next_out = reinterpret_cast<Bytef *>(outBuf.data());
	stream.avail_out = static_cast<uInt>(outBuf.size());

	ret = deflate(&stream, Z_FINISH);

	outLen = stream.total_out;

	deflateEnd(&stream);

	return ret == Z_STREAM_END;
}

static void RunBenchmark(const std::vector<char> & hubData, size_t iterations)
{
	std::vector<char> outBuf(hubData.size() + 1024, '\0');
	size_t compressedLen = 0;

	// Warmup
	DeflateData(hubData.data(), hubData.size(), outBuf, compressedLen);

	auto start = std::chrono::high_resolution_clock::now();

	for (size_t i = 0; i < iterations; i++)
	{
		DeflateData(hubData.data(), hubData.size(), outBuf, compressedLen);
	}

	auto end = std::chrono::high_resolution_clock::now();

	double elapsedSec = std::chrono::duration<double>(end - start).count();
	double totalMB = (hubData.size() * iterations) / (1024.0 * 1024.0);
	double throughput = totalMB / elapsedSec;
	double avgMicro = (elapsedSec * 1000000.0) / iterations;

	std::printf("=== Zlib Benchmark Results (%s) ===\n", g_sBenchName);
	std::printf("  Library:       %s\n", zlibVersion());
	std::printf("  Data size:     %.2f MB\n", hubData.size() / (1024.0 * 1024.0));
	std::printf("  Iterations:    %zu\n", iterations);
	std::printf("  Compressed:    %.2f MB (ratio %.1f%%)\n",
		compressedLen / (1024.0 * 1024.0),
		100.0 * compressedLen / hubData.size());
	std::printf("  Total processed: %.2f MB\n", totalMB);
	std::printf("  Elapsed:       %.3f s\n", elapsedSec);
	std::printf("  Throughput:    %.1f MB/s\n", throughput);
	std::printf("  Avg time:      %.1f us/op\n", avgMicro);
	std::printf("====================================\n\n");
}

int main()
{
	std::printf("\nPreparing hub traffic data...\n");

	auto hubData = PrepareHubData();
	std::printf("Generated %.2f MB of hub traffic data.\n\n", hubData.size() / (1024.0 * 1024.0));

	// Quick benchmark for CI
	RunBenchmark(hubData, 100);

	// Full benchmark for detailed comparison
	RunBenchmark(hubData, 1000);

	return 0;
}
