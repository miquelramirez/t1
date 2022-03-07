#ifndef __NFF_LOG__
#define __NFF_LOG__

#include <fstream>

namespace NFF
{

	class	Log
	{
	public:
		static Log&	instance();
		std::ofstream&	stream();

		~Log();

	protected:
		
		Log();
		
	protected:
		
		std::ofstream	m_stream;

	};

	inline	std::ofstream&	Log::stream() 
	{
		return m_stream;
	}

}

#endif // nff_log.hxx
