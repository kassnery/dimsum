// Probabilistic Random Number Generators
// Collected from various sources by Graham Cormode, 2000-2003
// 
#ifndef _PRNG_h
#define _PRNG_h

#define _USE_MATH_DEFINES

#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <string>
#include <sstream>
#include <fstream>
#include <queue>
#include <vector>
#include <map>
#include <set>
#include <stack>
#include <list>
#include <algorithm>
#include <cmath>
#include <locale>
#include <climits>
#include <limits>

#ifdef _MSC_VER
typedef unsigned __int8 byte;
//typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#endif

#define MOD 2147483647
#define HL 31

extern long hash31(int64_t, int64_t, int64_t);
extern long fourwise(int64_t, int64_t, int64_t, int64_t, int64_t);

#define KK  17
#define NTAB 32

#define SWAP(a,b) temp=(a);(a)=(b);(b)=temp;

int64_t LLMedSelect(int k, int n, int64_t arr[]);
int MedSelect(int k, int n, int arr[]);

typedef struct prng_type{
  int usenric; // which prng to use
  float scale;             /* 2^(- integer size) */
  long floatidum;
  long intidum; // needed to keep track of where we are in the 
  // nric random number generators
  long iy;
  long iv[NTAB];
  /* global variables */
  unsigned long randbuffer[KK];  /* history buffer */
  int r_p1, r_p2;          /* indexes into history buffer */
  int iset;
  double gset;
} prng_type;

extern long prng_int(prng_type *);
extern float prng_float(prng_type *);
extern prng_type * prng_Init(long, int);
extern void prng_Destroy(prng_type * prng);
void prng_Reseed(prng_type *, long);

//extern long double zipf(double, long) ;
extern double fastzipf(double, long, double, prng_type *);
extern double zeta(long, double);
extern double prng_normal(prng_type * prng);
extern double prng_stable(prng_type * prng, double);

//extern double stable(double); // stable distributions 
//extern double stabledevd(float) ;
//extern long double stabledevl(float) ;
//extern double altstab(double); 

namespace Tools
{
	enum Level
	{
		LVL_VERYLOW = 0x0,
		LVL_LOW,
		LVL_MEDIUM,
		LVL_HIGH,
		LVL_VERYHIGH
	};

	enum Architecture
	{
		ARCH_LITTLEENDIAN = 0x0,
		ARCH_BIGENDIAN,
		ARCH_NONIEEE
	};

	enum RandomGeneratorType
	{
		RGT_DRAND48 = 0x0,
		RGT_MERSENNE
	};

	class Exception
	{
	public:
		virtual std::string what() = 0;
		virtual ~Exception() {}
	};

	class IllegalStateException : public Exception
	{
	public:
		IllegalStateException(std::string s);
		virtual ~IllegalStateException() {};
		virtual std::string what();

	private:
		std::string m_error;
	}; // IllegalStateException

	class IllegalArgumentException : public Exception
	{
	public:
		IllegalArgumentException(std::string s);
		virtual ~IllegalArgumentException() {};
		virtual std::string what();

	private:
		std::string m_error;
	}; // IllegalArgumentException

	class NotSupportedException : public Exception
	{
	public:
		NotSupportedException(std::string s);
		virtual ~NotSupportedException() {};
		virtual std::string what();

	private:
		std::string m_error;
	}; // NotSupportedException

	class System
	{
	public:
		static Architecture getArchitecture();
	}; // System

	// Code for the Mersenne generator has been kindly contributed by the MassDAL Code Bank:
	// http://www.cs.rutgers.edu/~muthu/massdal-code-index.html
	class Random
	{
	public:
		Random();
		Random(uint32_t seed);
		Random(uint32_t seed, RandomGeneratorType t);
		Random(uint32_t seed, uint16_t xsubi0);
		virtual ~Random();

		int32_t nextUniformLong();
			// returns a uniformly distributed long (32 random bits).
		int64_t nextUniformLongLong();
			// returns a uniformly distributed long long (64 random bits).
		int32_t nextUniformLong(int32_t low, int32_t high);
			// returns a uniformly distributed long in the range [low, high).
		int64_t nextUniformLongLong(int64_t low, int64_t high);
		uint32_t nextUniformUnsignedLong();
		uint64_t nextUniformUnsignedLongLong();
		uint32_t nextUniformUnsignedLong(uint32_t low, uint32_t high);
		uint64_t nextUniformUnsignedLongLong(uint64_t low, uint64_t high);
		int16_t nextUniformShort();
		uint16_t nextUniformUnsignedShort();
		double nextUniformDouble();
			// returns a uniformly distributed double in the range [0, 1).
		double nextUniformDouble(double low, double high);
			// returns a uniformly distributed double in the range [low, high).
	
		// these use the inversion method, thus they are extremely slow. Use with caution.
		double nextNormalDouble();
			// returns doubles using a normal distribution with mean 0.0 and std 1.0 (unbounded).
		double nextNormalDouble(double mean, double std);
			// returns doubles using a normal distribution with mean mean and std std (unbounded).

		// these use the inversion method, thus they are extremely slow. Use with caution.
		int32_t nextSkewedLong(int32_t low, int32_t high, Level);
			// returns longs using a Zipf distribution in the range [low, high).
		double nextSkewedDouble(double low, double high, Level);
			// returns doubles using a Zipf distribution in the range [low, high).
		double nextSkewedDouble(Level);
			// returns doubles using a Zipf distribution in the range [0.0, 1.0).

		bool flipCoin();
			// A Bernoulli trial with probability p = 50%.
		bool bernulliTrial(double p);
			// A Bernoulli trial with probability of success p.

		size_t getSize() const;
			// Returns the total size of the random number generator (seed size, etc.).

		uint32_t getSeed() const;

	private:
		void initMersenne();
		void initDrand(uint16_t xsubi0);

		enum
		{
			MERS_N = 624,
			MERS_M = 397,
			MERS_R = 31,
			MERS_U = 11,
			MERS_S = 7,
			MERS_T = 15,
			MERS_L = 18,
			MERS_A = 0x9908B0DF,
			MERS_B = 0x9D2C5680,
			MERS_C = 0xEFC60000
		};

		RandomGeneratorType m_type;
		void* m_buffer;
		Architecture m_architecture;
		uint32_t m_seed;
		uint16_t m_xsubi[3];
	}; // Random

	class PRGZipf
	{
	public:
		PRGZipf(int32_t min, int32_t max, double s, Tools::Random* pRandom);
		virtual ~PRGZipf();

		int32_t nextLong();

	private:
		void initLookupTable();

		int32_t m_min;
		int32_t m_max;
		double m_s;
		Tools::Random* m_pRandom;
		double* m_pLookupTable;
	}; // PRGZipf
}

#endif
