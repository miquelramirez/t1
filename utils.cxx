#include "utils.hxx"
#include <iostream>
#include <cassert>
#include <unistd.h>
#include <cstdio>

void report_memory_usage()
{
	/*
	struct rusage usage_report;
	int rv = getrusage( RUSAGE_THREAD, &usage_report );
	assert( rv == 0);
	std::cout << "Memory usage report:" << std::endl;
	std::cout << "Max. Resident Set Size:" << usage_report.ru_maxrss << std::endl;
	std::cout << "Shared Memory Size:" << usage_report.ru_ixrss << std::endl;
	std::cout << "Unshared Data Size:" << usage_report.ru_idrss << std::endl;
	std::cout << "Unshared Stack Size:" << usage_report.ru_isrss << std::endl;
	*/
	static int 		n_bytes_per_page = getpagesize();
	static char		statm_fname[256];
	static int		kilobyte = 1024;
	static int		megabyte = 1024*1024;
	snprintf( statm_fname, 256, "/proc/%d/statm", getpid() );
	FILE* f = fopen( statm_fname, "r" );
	int program_size, resident_set_size, shared, text, lib, data, dirty;
	fscanf( f, "%d %d %d %d %d %d %d", &program_size,
						&resident_set_size,
						&shared,
						&text,
						&lib,
						&data,
						&dirty );
	program_size *= n_bytes_per_page;
	data *=	n_bytes_per_page;
	std::cout << "Memory Usage Report:" << std::endl;
	std::cout << "\tProgram Size: " << (float)program_size / (float)kilobyte << " KBytes ";
	std::cout << (float)program_size / (float)megabyte << " MBytes" << std::endl;
	std::cout << "\tResident Set Size: " << (float)resident_set_size / (float)kilobyte << " KBytes";
	std::cout << (float)resident_set_size / (float)megabyte << " MBytes" << std::endl;
	std::cout << "\tShared Set Size: " << (float)shared / (float)kilobyte << " KBytes";
	std::cout << (float)shared / (float)megabyte << " MBytes" << std::endl;
	std::cout << "\tText Size: " << (float)text / (float)kilobyte << " KBytes";
	std::cout << (float)text / (float)megabyte << " MBytes" << std::endl;
	std::cout << "\tLib Size: " << (float)lib / (float)kilobyte << " KBytes";
	std::cout << (float)lib / (float)megabyte << " MBytes" << std::endl;

	std::cout << "\tData Size: " << (float)data / (float)kilobyte << " KBytes ";
	std::cout << (float)data / (float)megabyte << " MBytes" << std::endl;
	std::cout << "\tDirty Size: " << (float)dirty / (float)kilobyte << " KBytes ";
	std::cout << (float)dirty / (float)megabyte << " MBytes" << std::endl;

	fclose(f);
}
