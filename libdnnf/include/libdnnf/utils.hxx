#ifndef __UTILS__
#define __UTILS__

#include <sys/times.h>
#include <sys/resource.h>
#include <unistd.h>
#include <vector>
#include <cstring>

/* Returns the time used by the current process so far (in seconds)*/
inline double  time_used()
{
	struct rusage  data;

	getrusage( RUSAGE_SELF, &data );

	//double system_time = (double)data.ru_stime.tv_sec + ((double)data.ru_stime.tv_usec/1e6);
	double user_time = (double)data.ru_utime.tv_sec + ((double)data.ru_utime.tv_usec/(double)1e6);
	
	//return system_time + user_time;
	return user_time;
}

inline clock_t ticks()
{
	struct tms buf;
	memset( &buf, 0, sizeof(struct tms) );
	return times( &buf );
}

#endif // utils.hxx
