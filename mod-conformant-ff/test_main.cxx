#include "libff.h"
#include "output.h"
#include <iostream>
#include <fstream>
#include <cstdlib>

int main( int argc, char** argv )
{

        if (argc < 3)
	  std::cout << "\n\tUsage: " << argv[0] << " <domain file> <instance file>" << std::endl;
  
	int res = FF_parse_problem( argv[1], argv[2] );
	if ( res == 0 )
	{
		std::cout << "Parsing of " << argv[1] << " and " << argv[2] << " was successful!" << std::endl;
 	}
	else
	{
		std::cerr << "Trouble parsing, check PDDL files!" << std::endl;
		std::exit(1);
	}

	res = FF_instantiate_problem();
	
	if ( res == 0 )
	{
		std::cout << "Problem was instantiated" << std::endl;
	}
	else
	{
		std::cerr << "Could not instantiate problem!" << std::endl;
		std::exit(1);
	}

	// Creating atoms.list
	std::ofstream out("atoms.list");
	for(int i = 0; i < gnum_ft_conn; i++) 
	  {
	    //print_ft_name(i);
	    out << i+1 << " "<< FF::get_ft_name(i) <<  std::endl;
	  }
	out.close();

	// Creating initial and goal file
	std::ofstream out1("init_goal.list");
	out1 << "Initial state is:" << std::endl; 
	out1 << "\nKNOWN ATOMS:\n" << std::endl;
	for ( int i = 0; i < ginitial_state.num_F; i++ ) 
	  out1 << " "<< FF::get_ft_name( ginitial_state.F[i] ) << std::endl;
 
	out1 << "\nUNKNOWN ATOMS:\n" << std::endl;
        for ( int i = 0; i < ginitial_state.num_U; i++ ) 
	  out1 << " "<< FF::get_ft_name( ginitial_state.U[i] ) << std::endl;

	out1 << "\nOR CLAUSES:\n" << std::endl;
        for ( int i = 0; i < gnum_initial_or; i++ ) {
	  out1 << " OR:" << std::endl;;
	  for ( int j = 0; j < ginitial_or_length[i]; j++ ) 
	    out1 << "     " << FF::get_ft_name( ginitial_or[i][j] ) << std::endl;
	}
	out1 << "\nGoal state is:\n" << std::endl;
        for ( int i = 0; i < ggoal_state.num_F; i++ ) 
	  out1 << " "<< FF::get_ft_name( ggoal_state.F[i] ) << std::endl;
        out1.close();

	std::ofstream out2("operators.list");
	out2 << "Name:" << std::endl;
	out2 << "START()" << std::endl;
	std::vector<unsigned> I, G;
	FF::get_initial_state( I );
	out2 << "Preconditions:" << std::endl;
	out2 << "Adds:" << std::endl;
	for ( unsigned i = 0; i < I.size(); i++ )
		out2 << FF::get_ft_name( I[i] ) << std::endl;
	out2 << "Deletes:" << std::endl;

	FF::get_goal_state( G );
	out2 << "Name:" << std::endl;
	out2 << "END()" << std::endl;
	out2 << "Preconditions:" << std::endl;
	for ( unsigned i = 0; i < G.size(); i++ )
		out2 << FF::get_ft_name( G[i] ) << std::endl;
	out2 << "Adds:" << std::endl;
	out2 << "Deletes:" << std::endl;


	for(int i = 0; i < gnum_ef_conn; i++) 
	{
		if ( gef_conn[i].removed == TRUE ) continue;
		/*	if ( gef_conn[i].illegal == TRUE ) continue; */
		out2 << "Name: " << std::endl;
		out2 << FF::get_op_name(i) << std::endl;

		out2 << "Preconditions:" << std::endl;
		for(int j = 0; j < gef_conn[i].num_PC; j++) {
			out2 << FF::get_ft_name( gef_conn[i].PC[j] ) << std::endl;
		}


		out2 << "Adds:" << std::endl;
		for(int j = 0; j < gef_conn[i].num_A; j++) {
			out2 << FF::get_ft_name( gef_conn[i].A[j] ) << std::endl;
		}

		
		out2 << "Deletes:" << std::endl;
		for(int j = 0; j < gef_conn[i].num_D; j++) {
			out2 << FF::get_ft_name( gef_conn[i].D[j] ) << std::endl;
		}


	}


	return 0;
}
