#include <iostream>
#include <chrono>
#include "isaac.h"

template<class Gen>
std::uint64_t time_rand(Gen gen, std::size_t num_bytes, std::uint64_t& val)
{
	std::size_t count = num_bytes / sizeof(typename Gen::result_type);
	auto start = std::chrono::system_clock::now();
	for (std::size_t i = 0; i < count; ++i)
	{
		// assigning the result to val prevents the optimizer
		// from eliminating the loop entirely, which it is prone to do
		val += gen();
	}
	auto finish = std::chrono::system_clock::now();
	auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start);
	return elapsed_ms.count();
}

template<class Generator>
inline void
random_fill(Generator& gen, unsigned char* buf, std::size_t count)
{
	static constexpr std::size_t word_size = sizeof(typename Generator::result_type);
	unsigned char* p = buf;
	unsigned char* limit = p + count;
	while (p < limit)
	{
		auto word = gen();
		auto n = std::min(static_cast<std::size_t>(limit - p), word_size);
		::memcpy(p, &word, n);
		p += n;
	}
}

int main(int argc, const char * argv[])
{

	// Compares execution times of std::mt19937_64 and isaac64<>

	std::random_device rdev;

	std::mt19937_64::result_type mtseed = rdev();
	mtseed <<= 32;
	mtseed |= rdev();
	std::mt19937_64 mtgen(mtseed);

	// Alpha should (generally) either be 8 for crypto use, or 4 for non-crypto use.
	// If not provided, it defaults to 8
	
	static constexpr std::size_t alpha = 8;
	
	// Alpha determines the size (in elements of result_type) of the
	// internal state, and the size of the initial state for seeding.

	utils::isaac64<alpha> igen{rdev};
	
	std::uint64_t value = 0; // hack to prevent optimizer from removing inner timing loop
	
	auto isaac_ms = time_rand(igen, 1 << 30, value);
	auto mt_ms = time_rand(mtgen, 1 << 30, value);
	
	std::cout << "generating 2^30 bytes, isaac64 = " << isaac_ms << " ms, mt19937_64 = " << mt_ms << " ms" << std::endl;


	// Added as example for LeMoussel (see issue https://github.com/edgeofmagic/ISAAC-engine/issues/1):

	auto start = std::chrono::system_clock::now();

	unsigned char nonce96[12];

	for (uint32_t j = 0; j < 1000000; j++) {
		random_fill(igen, nonce96, sizeof(nonce96));
	}

	auto finish = std::chrono::system_clock::now();
	auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start);

	std::cout << "elapsed time for random_fill(1000000 iterations): " << elapsed_ms.count() << " milliseconds." << std::endl;
	return 0;
}

