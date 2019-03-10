# ISAAC-engine

### A C++ random number engine based on the ISAAC pseudorandom number generator

ISAAC is a pseudorandom number generator created by Bob Jenkins. An in-depth discussion can
be found [here](http://burtleburtle.net/bob/rand/isaac.html), and the reference implementation
in C can be found [here](http://burtleburtle.net/bob/rand/isaacafa.html).

This project is a C++ implementation, based on the reference implementation, that takes
the form of a *random number engine* consistent with the random number generation
facility described in section 26.5 of the language standard. Specifically, it meets the
requirements for random number engines found in paragraph 26.5.1.4. The practical value of this
effort is that it provides an implementation of ISAAC that can be used in conjunction with
other components of the facility, such as engine adaptors and distributions.

## Using the engine

The implementation consists of a set of templates contained in a single header file, isaac.h.
The templates are defined in the namespace utils. A trivial use case might look something like this:

```` cpp
#include <isaac.h>
#include <random>

int main(int argc, const char * argv[])
{
	using namespace utils;

	isaac<> engine;

	// gen can be used 'raw' (not recommended) by
	// invoking operator()():

	auto rand_num = engine();

	// normally, a distribution object would invoke 
	// the engine:

	std::normal_distribution<double> normal_dist;
	auto rand_double = normal_dist(engine);
}
````
The header file includes two distinct engines, isaac and isaac64. The result_type of
isaac is an unsigned 32-bit integer, and the result_type of isaac64
is an unsigned 64-bit integer.

The templates take an optional parameter, Alpha, that determines the number of words
(of result_type) used for internal state, and the number of words used to seed the engine.
The state size is 2<sup>Alpha</sup>. For example:

```` cpp
isaac<8> engine; 	// state size is 2^8 = 256 words
isaac64<4> engine64; 	// state size is 2^4 = 16 words
````
Larger values for alpha result in higher quality output (in terms of measures of randomness).
Smaller values result in faster execution. The algorithm's author suggests using a value
of 8 for cryptographic applications, and 4 for non-cryptographic applications.

### Seeding the engine

The language standard requires certain methods for seeding an engine. Seeding can
occur at the time of construction, or by invoking the method seed() in one of its 
overloaded forms. The following constuctor and seed signatures are supported as per
the standard:

```` cpp
isaac<> engine1(); 	// null constructor uses a default seed (0)
engine1.seed();		// state after seed is the same as state after null constructor

isaac<> engine2(1234);	// seed with a single value of result_type
engine2(1234);

std::seed_seq seq;
isaac<> engine3(seq);	// seed with a seed sequence object
engine3.seed(seq);
````
Unfortunately, all of these seeding mechanisms have shortcomings with respect to predictability,
some of which are discussed [here](http://www.pcg-random.org/posts/cpp-seeding-surprises.html).
As an alternative, this implementation provides a method for seeding the the complete internal state
from any source that supports forward iterators, to wit:

```` cpp
std::vector<seed_type> seed_vec = { ... }; // fill from source of your choice
isaac<> engine(seed_vec.begin(), seed_vec.end());
// or
engine.seed(seed_vec.begin(), seed_vec.end());
````
If the number of elements available between the beginning and ending iterators is less than the the
state size, the sequence is repeated util the entire state is initialized.

#### [Edit: 3/10/2019]

A stronger seeding mechanism has been added that uses system entropy sources (such as /dev/urandom)
if available:

```` cpp
std::random_device rdev;
isaac<> gen(rdev);

// or, alternatively:

std::random_device rdev;
isaac<> gen;
gen.seed(rdev);
````

See your library's documentation for std::random_device for details. This constructor and overload of seed()
will initialize the internal state of the generator from the argument.

### Other required methods

Additional methods required by the standard include equality and inequality comparison operators.
Equality is determined by internal state. If two instances of an engine are are constructed with identical
parameter values, or seeded with identical values, they have the same internal state:

```` cpp
isaac<> engine1(1234);
isaac<> engine2(1234);
assert(engine1 == engine2);
````
Following an invocation of seed(), an engine will have the same state as one just contructed with
the same parameters as those used for seed():

```` cpp
isaac<> engine1(4321);
isaac<> engine2;
engine2.seed(4321);
assert(engine1 == engine2);
````
The invocation operator()(), discard(), and seed() methods all modify the internal state. For example, 
continuing the previous code snippet:

```` cpp
auto rnum = engine1();
assert(engine1 != engine2);
````
State can be saved to a stream and restored:

```` cpp
std::ostringstream os;
isaac<> engine1(5678);
os << engine1;

std::istringstream(os.str());
isaac<> engine2;
is >> engine2;

assert(engine1 == engine2);
````
The copy constructor initializes the constructed engine to the state of the constructor parameter:

```` cpp
isaac<> engine1(34567);
isaac<> engine2(engine1);
assert(engine1 == engine2);
````
The discard() method skips a specified number of values in the sequence generated by the engine:

```` cpp
isaac<> engine1;
isaac<> engine2;

engine1.discard(3);

engine2(); engine2(); engine2();

assert(engine1 == engine2);
````

### Performance

No performance tuning has been done on this implementation, although it follows the general structure of
the reference implementation closely. In its current state, it is slightly faster (about 10% to 20%) than the 
standard implementation of the Mersenne Twister engine of the same result_type (mt19937/isaac and mt19937_64/isaac64).
Bob Jenkins discusses the quality of the output at some length on his 
[web site](http://burtleburtle.net/bob/rand/isaac.html). The following
are output sets from the **ent** pseudorandom number sequence test program for mt19937_64 and isaac64, respectively:
(**ent** can be found [here](http://www.fourmilab.ch/random/), including discussion of how to interpret the output).

From mt19937_64, file size is 33554432 bytes:

````
Entropy = 7.999995 bits per byte.

Optimum compression would reduce the size
of this 33554432 byte file by 0 percent.

Chi square distribution for 33554432 samples is 241.77, and randomly
would exceed this value 71.45 percent of the times.

Arithmetic mean value of data bytes is 127.4949 (127.5 = random).
Monte Carlo value for Pi is 3.140223213 (error 0.04 percent).
Serial correlation coefficient is -0.000255 (totally uncorrelated = 0.0).
````
From isaac64, same file size:

````
Entropy = 7.999995 bits per byte.

Optimum compression would reduce the size
of this 33554432 byte file by 0 percent.

Chi square distribution for 33554432 samples is 240.22, and randomly
would exceed this value 73.82 percent of the times.

Arithmetic mean value of data bytes is 127.5154 (127.5 = random).
Monte Carlo value for Pi is 3.141199538 (error 0.01 percent).
Serial correlation coefficient is 0.000025 (totally uncorrelated = 0.0).
````






