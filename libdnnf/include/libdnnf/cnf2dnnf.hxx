#ifndef __cnf2dnnf__
#define __cnf2dnnf__

#include <string>
#include <libdnnf/cnf_theory.hxx>
#include <libdnnf/dnnf_theory.hxx>

namespace DNNF
{


class CNF_DNNF_Compiler
{
public:
	CNF_DNNF_Compiler( std::string compiler_path = "./c2d_linux" );
	~CNF_DNNF_Compiler();
	bool 		compile( CNF_Theory& cnf_t, DNNF_Theory& dnnf_t, bool project_unweighted = false );
	bool 		compile( std::string theory_fname );
	double		count_models( CNF_Theory& cnf_t );
	std::string 	compiler_path( ) const;
	void 		set_compiler_path( std::string new_path );
	void 		set_time_limit( long max_secs ) { m_time_limit = max_secs; }
	double 		total_time(); 

protected:
	
	bool 	execute_compiler_on( std::string fname, std::string ftree, std::string exist_name );
	void	execute_compiler_for_model_counting( std::string fname );
	void 	retrieve_times();
	void	retrieve_model_count();
	void 	create_exist_file( CNF_Theory& tau );
protected:

	std::string  m_compiler_path;
	long         m_time_limit;
	double       m_compile_time;
	double       m_pre_time;
	double       m_post_time;
	double       m_total_time;
	double	     m_model_count;
};

inline	double	CNF_DNNF_Compiler::total_time()
{
	return m_total_time;
}

inline	std::string	CNF_DNNF_Compiler::compiler_path() const
{
	return m_compiler_path;
}

inline	void		CNF_DNNF_Compiler::set_compiler_path( std::string new_path )
{
	m_compiler_path = new_path;
}

}

#endif // cnf2dnnf.hxx
