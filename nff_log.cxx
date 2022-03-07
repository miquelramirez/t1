#include "nff_log.hxx"

namespace NFF
{

	Log::Log()
	{
		m_stream.open( "planner.result" );
	}

	Log::~Log()
	{
	}

	Log&	Log::instance() 
	{
		static Log the_instance;
		return the_instance;
	}

	
}
