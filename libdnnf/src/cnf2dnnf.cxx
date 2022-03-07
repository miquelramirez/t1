#include <fstream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <unistd.h>
#include <errno.h>
#include <cstdio>
#include <cctype>
#include <iostream>
#include <libdnnf/cnf2dnnf.hxx>
#include <libdnnf/string_ops.hxx>
#include <libdnnf/utils.hxx>

namespace DNNF {

static int time_out = 0;
static int child_PID = 0;
static void alarm_handler( int code )
{
	//does nothing
	time_out = 1;
	kill( child_PID, SIGKILL );
}

CNF_DNNF_Compiler::CNF_DNNF_Compiler( std::string path )
	: m_compiler_path( path ), m_time_limit( 60 )
{
	m_compile_time = 0.0;
	m_pre_time = 0.0;
	m_post_time = 0.0;
	m_total_time = 0.0;
	m_model_count = 0.0;
}

CNF_DNNF_Compiler::~CNF_DNNF_Compiler()
{

}

void CNF_DNNF_Compiler::retrieve_model_count()
{
	std::ifstream compiler_output( "c2d.stdout" );
	char line_buffer[2048];
	std::string line;

	while ( !compiler_output.eof() )
	{
		compiler_output.getline( line_buffer, 2047, '\n' );
		line.assign( line_buffer );
		std::string tag = "Counting...";
		unsigned pos = line.find( tag );
		if ( pos == 0 ) 
		{
			std::string tag2 = "models";
			unsigned pos_end = line.find( tag2 );
			std::string number;
			for ( unsigned i = tag.size(); line[i] != ' '; i++ )
			{
				if ( isdigit( line[i] ) )
				{
					number.push_back(line[i]);
				}
			}
			from_string( m_model_count, number, std::dec );
			break;
		}
	}
	compiler_output.close();
}

void CNF_DNNF_Compiler::retrieve_times()
{
	// get times from c2d.stdout
	std::ifstream compiler_output( "c2d.stdout" );

	char line_buffer[2048];
	std::string line;

	while ( !compiler_output.eof() )
	{
		compiler_output.getline( line_buffer, 2047, '\n' );
		line.assign( line_buffer );

		std::string tag = "Compile";
		unsigned pos = line.find( tag );
		
		//Compile Time: 0.080s / Pre-Processing: 0.050s / Post-Processing: 0.030s
		if ( pos == 0 ) // we found it!
		{
			TokenList slash_tokens = split( line, '/' ); 
			TokenList numbers;
			for ( unsigned k = 0; k < slash_tokens.size(); k++ )
			{
				TokenList nv = split( slash_tokens[k], ':' );
				std::string number;
				for ( unsigned l = 0; l < nv[1].size(); l++ )
				{
					if ( isdigit( nv[1][l] ) ) number+= nv[1][l];
					if ( nv[1][l] == '.' ) number+= nv[1][l];
				}
				numbers.push_back( number );
			}
			from_string( m_compile_time, numbers[0], std::dec );
			from_string( m_pre_time, numbers[1], std::dec );
			from_string( m_post_time, numbers[2], std::dec );
			
		}

		tag = "Total Time";
		pos = line.find( tag );
		//Total Time: 1.180s
		if ( pos == 0 )
		{
			TokenList colon_tokens = split( line, ':' );
			std::string number;
			for ( unsigned l = 0; l < colon_tokens[1].size(); l++ )
			{
				if ( isdigit( colon_tokens[1][l] ) ) number+= colon_tokens[1][l];
				if ( colon_tokens[1][l] == '.' ) number += '.';
			}	
			from_string( m_total_time, number, std::dec );
			break;
		}
	}

	compiler_output.close();	
}

bool CNF_DNNF_Compiler::execute_compiler_on( std::string fname, std::string ftree, std::string fexist )
{
	std::cout << "Compiler time limit set to: " << m_time_limit << std::endl;
	if ( ( child_PID = fork() ) )
	{
		int ret;

		struct itimerval new_settings;
		struct itimerval old_settings;
		new_settings.it_interval.tv_sec = 0;
		new_settings.it_interval.tv_usec = 0;
		new_settings.it_value.tv_sec = m_time_limit;
		new_settings.it_value.tv_usec = 0;
		setitimer( ITIMER_REAL, &new_settings, &old_settings );
		sighandler_t old_handler = signal( SIGALRM, &alarm_handler );

		wait( &ret ); // simulating blocking call

		if ( time_out )
			std::cerr << "TIMEOUT!" << std::endl;
	
		signal( SIGALRM, old_handler ); // restoring original SIGALRM handler
		setitimer( ITIMER_REAL, &old_settings, NULL );
		std::string outname = fname + ".nnf";
		std::ifstream test( outname.c_str() );

		if ( time_out ) return false;
		if ( test.fail() ) return false;

		test.close();
		bool success = true; 
		if ( WIFEXITED( ret ) )
		{
			if ( WEXITSTATUS( ret ) == 0 ) // everything went OK 
				success = true;
			else
				success = false;
		}

		if ( success )
		{
			retrieve_times();
			retrieve_model_count();
		}
		else
		{
			m_compile_time = 0.0;
			m_pre_time = 0.0;
			m_post_time = 0.0;
		}

		return success;
	}
	else
	{
		std::cout << "Calling compiler at: " << m_compiler_path << std::endl;	
		int stdout_log = open( "c2d.stdout", O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR );
		int stderr_log = open( "c2d.stderr",  O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR );
		std::cout << "step created files."  << std::endl;

		if ( stdout_log == -1 || stderr_log == -1 ) exit(-1);
		dup2( stdout_log, 1 );
		std::cout << "dup2."  << std::endl;

		dup2( stderr_log, 2 );
		TokenList path_elems = split( compiler_path(), '/' );

		if ( ftree.empty() )
		{
			if ( fexist.empty() )
			{
				char* const  argv[] = { (char* const)path_elems.back().c_str(), 
							(char* const)"-in", (char* const)fname.c_str(), 
							(char* const)"-reduce",
							(char* const)"-smooth_all",
							(char* const)"-dt_method", (char* const)"4",
							(char* const)"-dt_out",
							/*
							(char* const)"-minimize",
							(char* const)"-count",
							*/
							NULL };
				execvp( m_compiler_path.c_str(), argv );
			}
			else
			{
				char* const  argv[] = { (char* const)path_elems.back().c_str(), 
							(char* const)"-in", (char* const)fname.c_str(), 
							(char* const)"-reduce",
							(char* const)"-smooth_all",
							(char* const)"-dt_method", (char* const)"4",
							(char* const)"-dt_out",
							(char* const)"-exist", (char* const)fexist.c_str(),
							/*
							(char* const)"-minimize",
							(char* const)"-count",
							*/
							NULL };	
				execvp( m_compiler_path.c_str(), argv );
				
			}
		}
		else
		{
			if ( fexist.empty() )
			{
				char* const  argv[] = { (char* const)path_elems.back().c_str(), 
							(char* const)"-in", (char* const)fname.c_str(), 
							(char* const)"-reduce", 
							(char* const)"-smooth",
							(char* const)"-dt_in", (char* const)ftree.c_str(),
							NULL };
				execvp( m_compiler_path.c_str(), argv );
			}
			else
			{
				char* const  argv[] = { (char* const)path_elems.back().c_str(), 
							(char* const)"-in", (char* const)fname.c_str(), 
							(char* const)"-reduce", 
							(char* const)"-smooth_all",
							(char* const)"-dt_in", (char* const)ftree.c_str(),
							(char* const)"-exist", (char* const)fexist.c_str(),
							NULL };	
				execvp( m_compiler_path.c_str(), argv );
				
			}
		}
		std::cout << "(EACCES=" << ( errno == EACCES ? "1" : "0" ) << std::endl;
		std::cerr << "Error doing execv call to c2d: check path to compiler" << std::endl;
		std::exit(1);
	}

	return false;

}

void	CNF_DNNF_Compiler::execute_compiler_for_model_counting( std::string fname )
{
	std::cout << "Compiler time limit set to: " << m_time_limit << std::endl;
	if ( ( child_PID = fork() ) )
	{
		int ret;

		struct itimerval new_settings;
		struct itimerval old_settings;
		new_settings.it_interval.tv_sec = 0;
		new_settings.it_interval.tv_usec = 0;
		new_settings.it_value.tv_sec = m_time_limit;
		new_settings.it_value.tv_usec = 0;
		setitimer( ITIMER_REAL, &new_settings, &old_settings );
		sighandler_t old_handler = signal( SIGALRM, &alarm_handler );
		
		wait( &ret ); // simulating blocking call
		if ( time_out )
			std::cerr << "TIMEOUT!" << std::endl;
	
		signal( SIGALRM, old_handler ); // restoring original SIGALRM handler
		setitimer( ITIMER_REAL, &old_settings, NULL );
		std::string outname = fname + ".nnf";
		std::ifstream test( outname.c_str() );

		test.close();
		bool success = true; 
		if ( WIFEXITED( ret ) )
		{
			if ( WEXITSTATUS( ret ) == 0 ) // everything went OK 
				success = true;
			else
				success = false;
		}

		if ( success )
		{
			retrieve_times();
			retrieve_model_count();
		}
		else
		{
			m_compile_time = 0.0;
			m_pre_time = 0.0;
			m_post_time = 0.0;
		}

	}
	else
	{
		//std::cout << "Calling compiler at: " << m_compiler_path << std::endl;	
		int stdout_log = open( "c2d.stdout", O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR );
		int stderr_log = open( "c2d.stderr",  O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR );
		if ( stdout_log == -1 || stderr_log == -1 ) exit(-1);
		dup2( stdout_log, 1 );
		dup2( stderr_log, 2 );
		TokenList path_elems = split( compiler_path(), '/' );

		char* const  argv[] = { (char* const)path_elems.back().c_str(), 
					(char* const)"-in", (char* const)fname.c_str(), 
					(char* const)"-reduce",
					(char* const)"-smooth_all",
					(char* const)"-dt_method", (char* const)"4",
					(char* const)"-dt_out",
					//(char* const)"-minimize",
					(char* const)"-count",
					NULL };
		execvp( m_compiler_path.c_str(), argv );
		std::cout << "(EACCES=" << ( errno == EACCES ? "1" : "0" ) << std::endl;
		std::cerr << "Error doing execv call to c2d: check path to compiler" << std::endl;
		std::exit(1);
	}
}

double	CNF_DNNF_Compiler::count_models( CNF_Theory& cnf_t )
{
	std::string in_theory = "inp.cnf";
	std::ofstream out( in_theory.c_str() );
	cnf_t.store_as_DIMACS( out );
	out.close();

	execute_compiler_for_model_counting( in_theory );

	return m_model_count;
}


bool 	CNF_DNNF_Compiler::compile( std::string t_file )
{
	return execute_compiler_on( t_file, "", "" );	
}

bool CNF_DNNF_Compiler::compile( CNF_Theory& cnf_t, DNNF_Theory& dnnf_t, bool project_unweighted )
{
	std::string in_theory = "inp.cnf";
	std::ofstream out( in_theory.c_str() );
	cnf_t.store_as_DIMACS( out );
	out.close();

	std::string d_tree = "";
	std::string exist = "";
	if ( project_unweighted )
	{
		create_exist_file( cnf_t );
		exist = "inp.exist";
	}
	
	bool cres = execute_compiler_on( in_theory, d_tree, exist );	
	if (!cres) return cres;

	std::string in_dnnf = in_theory + ".nnf";
	std::ifstream infile( in_dnnf.c_str() );
	bool loadres = dnnf_t.load_from( infile );
	dnnf_t.allocate_weights();
	dnnf_t.set_model_count( m_model_count );
	infile.close();
	//system( "rm -rf inp.cnf inp.cnf.nnf");
	
	return loadres;
}

void CNF_DNNF_Compiler::create_exist_file( CNF_Theory& tau )
{
	std::vector<int> projectable_vars;
	const int max_var = tau.max_var_name();
	
	for ( int x = 1; x <= max_var; x++ )
	{
		if ( !tau.valid(x) ) continue;
		if ( tau.is_projected(x) ) 
			projectable_vars.push_back(x);
		/*
		if ( !tau.var_is_weighted( x ) )
		{
			if ( !tau.is_alias(x) )
			{
				if ( tau.is_projected(x) )
					projectable_vars.push_back( x );
			}
			else
			{
				if ( tau.is_projected(tau.alias_of(x)) )
				{
					projectable_vars.push_back( x );
					tau.set_projected( x, 1 );
				}
			}
		}
		*/
	}

	std::ofstream out( "inp.exist" );

	out << projectable_vars.size() << " ";
	
	for ( unsigned k = 0; k < projectable_vars.size(); k++ )
		out << projectable_vars[k] << " ";
	
	out.close();
}
}
