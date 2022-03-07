#ifndef __EXT_MATH__
#define __EXT_MATH__

#include <cmath>
#include <limits>
#include <algorithm>

namespace std
{

template <typename T>
int sgn( const T& x )
{
	return ( x > 0 ? 1 : -1 );
}

template <typename T>
T add( T a, T b )
{
	static const T inf = std::numeric_limits<T>::max();
	return ( a==inf || b==inf ? inf : a+b );
}

template <>
inline 
float add( float a, float b )
{
	return a + b;
}


}

inline int gcd( int a, int b )
{
	if ( b == 0 ) return a;
	return gcd( b, a % b );
}

inline int gcd( int* numbers, int sz )
{
	assert( sz != 0 );
	if ( sz == 1 )
		return 1;

	int accum = gcd( numbers[0], numbers[1] );
	for ( int k = 2; k < sz; k++ )
	{
		accum = gcd( accum, numbers[k] );
	}	

	return accum;
}

inline int lcm( int* numbers, int sz )
{
	if ( sz == 0 ) return 1;
	int den = gcd( numbers, sz );
	int num = numbers[0];
	for ( int i = 1; i < sz; i++ )
		num *= numbers[i];

	return num / den;
}



inline int small_lit( int l )
{
	return (abs(l)<<1)|(l>0?0:1);
}

inline int var( int small )
{
	return (small^1)>>1;
}

inline int polarity( int small )
{
	return (small & 1) ? 1 : -1;
}

inline int big_lit( int small_literal )
{
	return var(small_literal)*polarity(small_literal);
}




#endif // ext_math.hxx
