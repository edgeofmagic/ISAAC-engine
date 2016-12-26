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

int main(int argc, const char * argv[])
{

	// Compares execution times of std::mt19937_64 and isaac64<>

	auto mtseed = static_cast<std::mt19937_64::result_type>(std::chrono::system_clock::now().time_since_epoch().count());
	std::mt19937_64 mtgen(mtseed);

	// Alpha should (generally) either be 8 for crypto use, or 4 for non-crypto use.
	// If not provided, it defaults to 8
	
	static constexpr std::size_t alpha = 8;
	
	// Alpha determines the size (in elements of result_type) of the
	// internal state, and the size of the initial state for seeding.
	
	static constexpr std::size_t seed_blk_size = 1 << alpha;

	using isaac_type = utils::isaac64<>;
	isaac_type igen;

	// a simple seeding strategy, using the engine to seed itself
	
	auto small_seed = static_cast<isaac_type::result_type>(std::chrono::system_clock::now().time_since_epoch().count());
	igen.seed(small_seed);
	
	// use the time-seeded engine to create a full block of seed values
	
	std::vector<isaac_type::result_type> seedvec(seed_blk_size);
	for (std::size_t i = 0; i < seed_blk_size; ++i)
	{
		seedvec[i] = igen();
	}

	igen.seed(seedvec.begin(), seedvec.end());
	
	std::uint64_t value = 0; // hack to prevent optimizer from removing inner timing loop
	
	auto isaac_ms = time_rand(igen, 1 << 30, value);
	auto mt_ms = time_rand(mtgen, 1 << 30, value);
	
	std::cout << "generating 2^30 bytes, isaac64 = " << isaac_ms << " ms, mt19937_64 = " << mt_ms << " ms" << std::endl;

	return 0;
}

