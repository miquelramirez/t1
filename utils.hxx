/*
    Miguel Ramirez, Nir Lipovetzky, Hector Geffner
    C^3: A planner for the sequential, satisficing track of the IPC-6
    Copyright (C) 2008  

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __UTILS__
#define __UTILS__

#include <sys/times.h>
#include <sys/resource.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>

#include <math.h>

#include <stdexcept>

#include "error.hpp"

const unsigned int infinity = ~0;

/**
 * @brief Returns the log base 2 of a number @b zvalue.
 **/
inline unsigned int log2(unsigned int zvalue)
{
    // log2(0) is undefined so error
    if (zvalue == 0) ERROR(std::domain_error("Log base 2 of 0 is undefined."));

    unsigned int result = 0;

    // Loop until zvalue equals 0
    while (zvalue)
    {
        zvalue >>= 1;
        ++result;
    }

    return result - 1;
}

inline double  time_used()
{
	struct rusage  data;

	getrusage( RUSAGE_SELF, &data );

	double system_time = (double)data.ru_stime.tv_sec + ((double)data.ru_stime.tv_usec/1e6);
	double user_time = (double)data.ru_utime.tv_sec + ((double)data.ru_utime.tv_usec/(double)1e6);
	
	return user_time + system_time;
}

void report_memory_usage();

inline void report_interval( double t0, double t1, std::ostream& os )
{
	double delta = t1 - t0;
	if (delta < 1e-12)
		os << "<1 msec" << std::endl;
	else
		os << std::setprecision(3) << delta << std::endl;
}

inline double mem_used()
{
	struct rusage data;

	getrusage( RUSAGE_SELF, &data );
	
	return (double)data.ru_maxrss / 1024.;
}


#define THRESHOLD 1e-04

inline bool dless(float f1, float f2) {

  //  std::cout << "f1: " << f1 << ", f2: " << f2 << ", f2 - f1: " << f2 - f1 << ", th: " << THRESHOLD << std::endl;

  return ((f2 - f1) > THRESHOLD);
}

inline bool dequal(float f1, float f2) {

  //  std::cout << "f1: " << f1 << ", f2: " << f2 << ", f2 - f1: " << f2 - f1 << ", th: " << THRESHOLD << std::endl;

  return ((fabs(f2 - f1)) < THRESHOLD);
}


#endif // utils.hxx
