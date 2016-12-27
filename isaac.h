/*
	This work is derived from the ISAAC random number generator, created by Bob Jenkins,
	which he has generously put in the public domain. All design credit goes to Bob Jenkins.

	Details of the algorithm, and the original C source can be found at 
	http://burtleburtle.net/bob/rand/isaacafa.html.

	This work is a C++ translation and re-packaging of the original C code to make it 
	meet the requirements for a random number engine, as specified in paragraph 26.5.1.4
	of the C++ language standard. As such, it can be used in conjunction with other elements
	in the random number generation facility, such as distributions and engine adaptors.

	Created by David Curtis, 2016. Public Domain.
*/

#ifndef guard_utils_isaac_h
#define guard_utils_isaac_h

#include <limits>
#include <type_traits>
#include <random>
#include <array>

namespace utils
{

/************************************************************
_isaac contains code common to isaac and isaac64.
It uses CRTP (a.k.a. 'static polymorphism') to invoke
specialized methods in the derived class templates,
avoiding the cost of virtual method invocations and
allowing those methods to be placed inline by the compiler.
Applications should not specialize or instantiate this 
template directly.
*************************************************************/

template<class Derived, std::size_t Alpha, class T>
class _isaac
{
public:
	using result_type = T;

protected:
	static constexpr std::size_t state_size = 1 << Alpha;

	static constexpr result_type default_seed = 0;

	explicit _isaac(result_type s)
	{
		seed(s);
	}
	
	template<class Sseq>
	explicit _isaac(Sseq& q, typename std::enable_if<std::__is_seed_sequence<Sseq, _isaac>::value>::type* = 0)
	{
		seed(q);
	}
	
	_isaac(const std::vector<result_type>& sv)
	{
		seed(sv);
	}
	
	template<class Iter>
	_isaac(Iter begin, Iter end, typename std::enable_if<
		   std::is_integral<typename std::iterator_traits<Iter>::value_type>::value &&
		   std::is_unsigned<typename std::iterator_traits<Iter>::value_type>::value>::type * = nullptr)
	{
		seed(begin, end);
	}
	
	_isaac(const _isaac& rhs)
	{
		for (std::size_t i = 0; i < state_size; ++i)
		{
			result_[i] = rhs.result_[i];
			memory_[i] = rhs.memory_[i];
		}
		a_ = rhs.a_;
		b_ = rhs.b_;
		c_ = rhs.c_;
		count_ = rhs.count_;
	}

public:

	static constexpr result_type min()
	{
		return std::numeric_limits<result_type>::min();
	}
	static constexpr result_type max()
	{
		return std::numeric_limits<result_type>::max();
	}
	
	inline void
	seed(result_type s = default_seed)
	{
		for (std::size_t i = 0; i < state_size; ++i)
		{
			result_[i] = s;
		}
		init();
	}
	
	template<class Sseq>
	inline typename std::enable_if <std::__is_seed_sequence<Sseq, Derived>::value, void>::type
	seed(Sseq& q)
	{
		std::array<result_type, state_size> seed_array;
		q.generate(seed_array.begin(), seed_array.end());
		for (std::size_t i = 0; i < state_size; ++i)
		{
			result_[i] = seed_array[i];
		}
		init();
	}

	template<class Iter>
	inline typename std::enable_if<
		std::is_integral<typename std::iterator_traits<Iter>::value_type>::value &&
		std::is_unsigned<typename std::iterator_traits<Iter>::value_type>::value, void>::type
	seed(Iter begin, Iter end)
	{
		Iter it = begin;
		for (std::size_t i = 0; i < state_size; ++i)
		{
			if (it == end)
			{
				it = begin;
			}
			result_[i] = *it;
			++it;
		}
		init();
	}
	
	inline result_type
	operator()()
	{
		return (!count_--) ? (do_isaac(), count_ = state_size - 1, result_[count_]) : result_[count_];
	}
	
	inline void
	discard(unsigned long long z)
	{
		for (; z; --z) operator()();
	}

	friend bool
	operator==(const _isaac& x, const _isaac& y)
	{
		bool equal = true;
		if (x.a_ != y.a_ || x.b_ != y.b_ || x.c_ != y.c_ || x.count_ != y.count_)
		{
			equal = false;
		}
		else
		{
			for (std::size_t i = 0; i < state_size; ++i)
			{
				if (x.result_[i] != y.result_[i] || x.memory_[i] != y.memory_[i])
				{
					equal = false;
					break;
				}
			}
		}
		return equal;
	}

	friend bool
	operator!=(const _isaac& x, const _isaac& y)
	{
		return !(x == y);
	}

	template <class CharT, class Traits>
	friend std::basic_ostream<CharT, Traits>&
	operator<<(std::basic_ostream<CharT, Traits>& os, const _isaac& x)
	{
		std::__save_flags<CharT, Traits> sflags(os);
		os.flags(std::ios_base::dec | std::ios_base::left);
		CharT sp = os.widen(' ');
		os.fill(sp);
		os << x.count_;
		for (std::size_t i = 0; i < state_size; ++i)
		{
			os << sp << x.result_[i];
		}
		for (std::size_t i = 0; i < state_size; ++i)
		{
			os << sp << x.memory_[i];
		}
		os << sp << x.a_ << sp << x.b_ << sp << x.c_;
		return os;
	}
	
	template <class CharT, class Traits>
	friend std::basic_istream<CharT, Traits>&
	operator>>(std::basic_istream<CharT, Traits>& is, _isaac& x)
	{
		bool failed = false;
		result_type tmp_result[state_size];
		result_type tmp_memory[state_size];
		result_type tmp_a = 0;
		result_type tmp_b = 0;
		result_type tmp_c = 0;
		std::size_t tmp_count = 0;
		
		std::__save_flags<CharT, Traits> sflags(is);
		is.flags(std::ios_base::dec | std::ios_base::skipws);
		
		is >> tmp_count;
		if (is.fail())
		{
			failed = true;
		}
		
		if (!failed)
		{
			for (std::size_t i = 0; i < state_size; ++i)
			{
				is >> tmp_result[i];
				if (is.fail())
				{
					failed = true;
					break;
				}
			}
		}
		if (!failed)
		{
			for (std::size_t i = 0; i < state_size; ++i)
			{
				is >> tmp_memory[i];
				if (is.fail())
				{
					failed = true;
					break;
				}
			}
		}
		if (!failed)
		{
			is >> tmp_a;
			if (is.fail())
			{
				failed = true;
			}
		}
		if (!failed)
		{
			is >> tmp_b;
			if (is.fail())
			{
				failed = true;
			}
		}
		if (!failed)
		{
			is >> tmp_c;
			if (is.fail())
			{
				failed = true;
			}
		}
		
		if (!failed)
		{
			for (std::size_t i = 0; i < state_size; ++i)
			{
				x.result_[i] = tmp_result[i];
				x.memory_[i] = tmp_memory[i];
			}
			x.a_ = tmp_a;
			x.b_ = tmp_b;
			x.c_ = tmp_c;
			x.count_ = tmp_count;
		}
		else
		{
			is.setstate(std::ios::failbit); // should already be set, just making certain
		}
		return is;
	}

protected:

	void
	init()
	{
		result_type a = golden();
		result_type b = golden();
		result_type c = golden();
		result_type d = golden();
		result_type e = golden();
		result_type f = golden();
		result_type g = golden();
		result_type h = golden();
		
		a_ = 0;
		b_ = 0;
		c_ = 0;
		
		for (std::size_t i = 0; i < 4; ++i)          /* scramble it */
		{
			mix(a,b,c,d,e,f,g,h);
		}
		
		/* initialize using the contents of result_[] as the seed */
		for (std::size_t i = 0; i < state_size; i += 8)
		{
			a += result_[i];
			b += result_[i+1];
			c += result_[i+2];
			d += result_[i+3];
			e += result_[i+4];
			f += result_[i+5];
			g += result_[i+6];
			h += result_[i+7];
			
			mix(a,b,c,d,e,f,g,h);
			
			memory_[i] = a;
			memory_[i+1] = b;
			memory_[i+2] = c;
			memory_[i+3] = d;
			memory_[i+4] = e;
			memory_[i+5] = f;
			memory_[i+6] = g;
			memory_[i+7] = h;
		}
		
		/* do a second pass to make all of the seed affect all of memory_ */
		for (std::size_t i = 0; i < state_size; i += 8)
		{
			a += memory_[i];
			b += memory_[i+1];
			c += memory_[i+2];
			d += memory_[i+3];
			e += memory_[i+4];
			f += memory_[i+5];
			g += memory_[i+6];
			h += memory_[i+7];
			
			mix(a,b,c,d,e,f,g,h);
			
			memory_[i] = a;
			memory_[i+1] = b;
			memory_[i+2] = c;
			memory_[i+3] = d;
			memory_[i+4] = e;
			memory_[i+5] = f;
			memory_[i+6] = g;
			memory_[i+7] = h;
		}

		do_isaac();				/* fill in the first set of results */
		count_ = state_size;	/* prepare to use the first set of results */
	}
	
	inline void
	do_isaac()
	{
		static_cast<Derived*>(this)->_do_isaac();
	}
	
	inline result_type
	golden()
	{
		return static_cast<Derived*>(this)->_golden();
	}
	
	inline void
	mix(result_type& a, result_type& b, result_type& c, result_type& d, result_type& e, result_type& f, result_type& g, result_type& h)
	{
		static_cast<Derived*>(this)->_mix(a, b, c, d, e, f, g, h);
	}
	
	result_type result_[state_size];
	result_type memory_[state_size];
	result_type a_;
	result_type b_;
	result_type c_;
	std::size_t count_;
};


template<std::size_t Alpha = 8>
class isaac : public _isaac<isaac<Alpha>, Alpha, std::uint32_t>
{
public:

	using base = _isaac<isaac, Alpha, std::uint32_t>;
	
	friend class _isaac<isaac, Alpha, std::uint32_t>;
	
	using result_type = std::uint32_t;
	
	explicit isaac(result_type s = base::default_seed)
	:
	base::_isaac(s)
	{}
	
	template<class Sseq>
	explicit isaac(Sseq& q, typename std::enable_if<std::__is_seed_sequence<Sseq, isaac>::value>::type* = 0)
	:
	base::_isaac(q)
	{}
	
	template<class Iter>
	isaac(Iter begin, Iter end, typename std::enable_if <
		  std::is_integral<typename std::iterator_traits<Iter>::value_type>::value &&
		  std::is_unsigned<typename std::iterator_traits<Iter>::value_type>::value>::type * = nullptr)
	:
	base::_isaac(begin, end)
	{}

	isaac(const isaac& rhs)
	:
	base::_isaac(static_cast<const base&>(rhs))
	{}

private:
	
	static constexpr result_type _golden()
	{
		return 0x9e3779b9; /* the golden ratio */
	}

	inline void
	_mix(result_type& a, result_type& b, result_type& c, result_type& d, result_type& e, result_type& f, result_type& g, result_type& h)
	{
	   a ^= b << 11; d += a; b += c;
	   b ^= c >> 2;  e += b; c += d;
	   c ^= d << 8;  f += c; d += e;
	   d ^= e >> 16; g += d; e += f;
	   e ^= f << 10; h += e; f += g;
	   f ^= g >> 4;  a += f; g += h;
	   g ^= h << 8;  b += g; h += a;
	   h ^= a >> 9;  c += h; a += b;
	}

	inline result_type
	ind(result_type* mm, result_type x)
	{
		return *(result_type *)((std::uint8_t *)(mm) + ((x) & ((base::state_size - 1) << 2)));
	}

	inline void
	rngstep(const result_type mix, result_type& a, result_type& b, result_type*& mm,
			result_type*& m, result_type*& m2, result_type*& r, result_type& x, result_type& y)
	{
	  x = *m;
	  a = (a^(mix)) + *(m2++);
	  *(m++) = y = ind(mm, x) + a + b;
	  *(r++) = b = ind(mm, y >> Alpha) + x;
	}

	void
	_do_isaac()
	{
		result_type a;
		result_type b;
		result_type x;
		result_type y;
		result_type * m;
		result_type * mm;
		result_type * m2;
		result_type * r;
		result_type * mend;
		
		mm = base::memory_;
		r = base::result_;
		a = base::a_;
		b = base::b_ + (++base::c_);
		for (m = mm, mend = m2 = m + (base::state_size/2); m < mend; )
		{
			rngstep( a << 13, a, b, mm, m, m2, r, x, y);
			rngstep( a >> 6 , a, b, mm, m, m2, r, x, y);
			rngstep( a << 2 , a, b, mm, m, m2, r, x, y);
			rngstep( a >> 16, a, b, mm, m, m2, r, x, y);
		}
		for (m2 = mm; m2 < mend; )
		{
			rngstep( a << 13, a, b, mm, m, m2, r, x, y);
			rngstep( a >> 6 , a, b, mm, m, m2, r, x, y);
			rngstep( a << 2 , a, b, mm, m, m2, r, x, y);
			rngstep( a >> 16, a, b, mm, m, m2, r, x, y);
		}
		base::b_ = b; base::a_ = a;
	}

};

template<std::size_t Alpha = 8>
class isaac64 : public _isaac<isaac64<Alpha>, Alpha, std::uint64_t>
{
public:
	
	using result_type = std::uint64_t;

	using base = _isaac<isaac64, Alpha, std::uint64_t>;

	friend class _isaac<isaac64, Alpha, std::uint64_t>;
	
	explicit isaac64(result_type s = base::default_seed)
	:
	base::_isaac(s)
	{}
	
	template<class Sseq>
	explicit isaac64(Sseq& q, typename std::enable_if<std::__is_seed_sequence<Sseq, isaac64>::value>::type* = 0)
	:
	base::_isaac(q)
	{}
	
	template<class Iter>
	isaac64(Iter begin, Iter end,
			typename std::enable_if
			<
					std::is_integral<typename std::iterator_traits<Iter>::value_type>::value &&
					std::is_unsigned<typename std::iterator_traits<Iter>::value_type>::value
			>::type * = nullptr
			)
	:
	base::_isaac(begin, end)
	{}

	isaac64(const isaac64& rhs)
	:
	base::_isaac(static_cast<const base&>(rhs))
	{}

private:

	static constexpr result_type _golden()
	{
		return 0x9e3779b97f4a7c13LL; /* the golden ratio */
	}

	inline void
	_mix(result_type& a, result_type& b, result_type& c, result_type& d, result_type& e, result_type& f, result_type& g, result_type& h)
	{
	   a -= e; f ^= h >> 9;  h += a;
	   b -= f; g ^= a << 9;  a += b;
	   c -= g; h ^= b >> 23; b += c;
	   d -= h; a ^= c << 15; c += d;
	   e -= a; b ^= d >> 14; d += e;
	   f -= b; c ^= e << 20; e += f;
	   g -= c; d ^= f >> 17; f += g;
	   h -= d; e ^= g << 14; g += h;
	}

	inline result_type
	ind(result_type* mm, result_type x)
	{
		return *(result_type *)((std::uint8_t *)(mm) + ((x) & ((base::state_size - 1) << 3)));
	}

	inline void
	rngstep(const result_type mix, result_type& a, result_type& b, result_type*& mm,
			result_type*& m, result_type*& m2, result_type*& r, result_type& x, result_type& y)
	{
	  x = *m;
	  a = (mix) + *(m2++);
	  *(m++) = y = ind(mm, x) + a + b;
	  *(r++) = b = ind(mm, y >> Alpha) + x;
	}

	void
	_do_isaac()
	{
		result_type a;
		result_type b;
		result_type x;
		result_type y;
		result_type * m;
		result_type * mm;
		result_type * m2;
		result_type * r;
		result_type * mend;
		
		mm = base::memory_;
		r = base::result_;
		a = base::a_;
		b = base::b_ + (++base::c_);
		for (m = mm, mend = m2 = m + (base::state_size / 2); m < mend; )
		{
			rngstep(~(a ^ (a << 21)), a, b, mm, m, m2, r, x, y);
			rngstep(a ^ (a >> 5), a, b, mm, m, m2, r, x, y);
			rngstep(a ^ (a << 12), a, b, mm, m, m2, r, x, y);
			rngstep(a ^ (a >> 33), a, b, mm, m, m2, r, x, y);
		}
		for (m2 = mm; m2 < mend; )
		{
			rngstep(~(a^(a << 21)), a, b, mm, m, m2, r, x, y);
			rngstep(a^(a >> 5), a, b, mm, m, m2, r, x, y);
			rngstep(a^(a << 12), a, b, mm, m, m2, r, x, y);
			rngstep(a^(a >> 33), a, b, mm, m, m2, r, x, y);
		}
		base::b_ = b; base::a_ = a;
	}

};

}

#endif /* guard_utils_isaac_h */
