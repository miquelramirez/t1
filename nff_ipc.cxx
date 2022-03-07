#include "nff_ipc.hxx"

namespace NFF
{

	Ipc::Ipc()
	{
                //NB: should use the extension 'linear' for conformant planning
		m_stream.open( "plan.ipc" );
	}

	Ipc::~Ipc()
	{
	}

	Ipc&	Ipc::instance() 
	{
		static Ipc the_instance;
		return the_instance;
	}

}
