# ISAAC-engine
A C++ random number engine based on the ISAAC pseudorandom number generator

ISAAC is a pseudorandom number generator created by Bob Jenkins. An in-depth discussion can
be found [here](http://burtleburtle.net/bob/rand/isaac.html), and the reference implementation
(in C) can be found [here](http://burtleburtle.net/bob/rand/isaacafa.html).

This project is a C++ implementation (based on the reference implementation) that takes
the form of a random number engine consistent with the C++ standard random number generation
facility described in section 26.5.1.4 of the language standard. The practical value of this
effort is that it provides an implementation of ISAAC that can be used in conjunction with
other elements of the facility, such as engine adaptors and distributions.

##Using the engine

The implementation consists of a set of templates contained in a single header file, isaac.h.
The templates are defined in the namespace utils. The simplest use case looks something like this:
````C++
#include <isaac.h>

int main(int argc, const char * argv[])
{
	utils::isaac<> gen;

	// gen can be used 'raw' (not recommended) by
	// invoking operator()():

	auto rand_num = gen();

	// normally, a distribution object would invoke 
	// the engine:

	std::normal_distribution<double> normal_dist(5.0,2.0);
	auto rand_double = normal_dist(gen);
}
````
The header file includes two distinct engines, isaac and isaac64. The result_type of
isaac is std::uint32_t (an unsigned 32-bit integer), and the result_type of isaac64
is std::uint64_t (an unsigned 64-bit integer).

The templates take an optional parameter, Alpha, that determines the number of words
(of result_type) used for internal state, and the number of words used to seed the engine.
The state size is 2<sup>Alpha</sup>. For example:
````
	utils::isaac<8> gen; 		// state size is 2^8 = 256 words
	utils::isaac64<4> gen64; 	// state size is 2^4 = 16 words
````
Larger values for alpha result in higher quality output (in terms of measures of randomness).
Smaller values result in faster execution. The algorithm's author suggests using a value
of 8 for cryptographic applications, and 4 for non-cryptographic applications.

###Seeding the engine

The language standard requires certain methods for seeding engines. Seeding can
occur at the time of construction, or by invoking the method seed() in one of its 
overloaded forms. The following constuctor and seed signatures are supported as per
the standard:
````
	using seed_type = utils::isaac<>::result_type;

	utils::isaac<> gen1(); 	// null constructor uses a default seed (0)
	gen1.seed();		// state after seed is the same as state after null constructor

	seed_type seed = 1234;
	utils::isaac<> gen2(seed); // seed with a single value of result_type
	gen2(seed);

	std::seed_seq seq( { 1, 127, 43 } );
	utils::isaac<> gen3(seq); // seed with a seed sequence object
	gen3.seed(seq);
````
Unfortunately, all of these seeding mechanisms have shortcomings with respect to predictability,
some of which are discussed [here](http://www.pcg-random.org/posts/cpp-seeding-surprises.html).
As an alternative, this implementation provides a method for seeding the the complete internal state
from any source that supports forward iterators, to wit:
````
	std::vector<seed_type> seed_vec = { ... }; // fill from source of your choice
	utils::isaac<> gen(seed_vec.begin(), seed_vec.end());
	// or
	gen.seed(seed_vec.begin(), seed_vec.end());
````
If the number of elements available between the beginning and ending iterators is less than the the
state size, the sequence is repeated util the state is initialized.


