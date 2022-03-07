#ifndef __NFF_IPC__
#define __NFF_IPC__

#include <fstream>

namespace NFF
{
	class	Ipc
	{
	public:
		static Ipc&	instance();
		std::ofstream&	stream();

		~Ipc();

	protected:
		
		Ipc();
		
	protected:
		
		std::ofstream	m_stream;

	};

	inline	std::ofstream&	Ipc::stream() 
	{
		return m_stream;
	}

}

#endif // nff_ipc.hxx
