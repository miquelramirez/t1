/*
    Alexandre Albore, Miguel Ramirez, Hector Geffner
    T1: A conformant planner
    Copyright UPF (C) 2010  

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
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <signal.h>

#include "utils.hxx"
#include "PDDL.hxx"
#include "nff_options.hxx"
#include "global_options.hxx"

#include "nff_belief.hxx"
#include "nff_minisat.hxx"
#include "nff_search_node.hxx"
#include "nff_bsearch.hxx"

#include "nff_log.hxx"
#include "nff_ipc.hxx"
#include "nff_planner_stats.hxx"
#include "nff_mutexes.hxx"

void	load_test_sequence( std::string fname, std::vector<PDDL::Operator*>& ops )
{
	PDDL::Task&	the_task = PDDL::Task::instance();
	std::ifstream instream( fname.c_str() );
	if ( instream.fail() )
	{
		std::cerr << "Could not open " << fname << std::endl;
		std::exit(1);
	}
	while ( !instream.eof() )
	{
		char buffer[2048];
		std::string line;
		instream.getline( buffer, 2048, '\n' );
		line.assign( buffer );
		if (!line.empty() ) 
		{
			std::cout << "Got " << line << " from " << fname << std::endl;
			bool found = false;
			for ( unsigned k = 0; k < the_task.prototype_ops().size(); k++ )
			{
				PDDL::Operator* op = the_task.prototype_ops()[k];
				if ( the_task.str_tab().get_token( op->code() ) 
					== line )
				{
					ops.push_back(op);
					found = true;
					break;	
				}
			}
			if ( !found )
			{
				std::cerr << "Operator " << line << " does not exist" << std::endl;
				std::exit(1);
			}
		}
	}
	instream.close();
	for ( unsigned k = 0; k < ops.size(); k++ ) 
	{
		std::cout << "Loaded ";
		the_task.print_operator( ops[k], std::cout );
		std::cout << std::endl;
	}
}

void handle_exit()
{
	NFF::Log&		log = NFF::Log::instance();
	NFF::Planner_Stats&	stats = NFF::Planner_Stats::instance();
	log.stream() << ";; heuristic_computations=" << stats.heuristic_computations() << std::endl;
	log.stream() << ";; expanded_ehc=" << stats.expanded_ehc() << std::endl;
	log.stream() << ";; generated_ehc=" << stats.generated_ehc() << std::endl;
	log.stream() << ";; expanded_bfs=" << stats.expanded_bfs() << std::endl;
	log.stream() << ";; generated_bfs=" << stats.generated_bfs() << std::endl;
	log.stream() << ";; primary_expansions=" << stats.primary_expansions() << std::endl;
	log.stream() << ";; secondary_expansions=" << stats.secondary_expansions() << std::endl;
	log.stream() << ";; tertiary_expansions=" << stats.tertiary_expansions() << std::endl;
	log.stream() << ";; entailment_tests=" << stats.entailment_tests() << std::endl;
	log.stream() << ";; sat_solver_calls=" << stats.sat_solver_calls() << std::endl;
	log.stream() << ";; max_num_clauses=" << stats.max_num_clauses() << std::endl;
	log.stream() << ";; problem_inconsistent=" << ( stats.problem_inconsistent() ? "yes" : "no" ) << std::endl;
	log.stream() << ";; initial_sample_h=" << stats.initial_sample_heuristic() << std::endl;
	log.stream() << ";; initial_card_h=" << stats.initial_card_heuristic() << std::endl;
	log.stream() << ";; avg_dead_models=" << stats.num_dead_models() << std::endl;
	log.stream() << ";; dead_samples_replaced=" << stats.dead_samples_replaced() << std::endl;
	log.stream() << ";; width_1=" << ( stats.is_width_1() ? "yes" : "no" ) << std::endl;
//	log.stream() << ";; hash_size=" << stats.hashtable_nodes() << std::endl;
	log.stream() << ";; nodes_mem=" << stats.node_megabytes() << std::endl;
	log.stream() << ";; b0_mc=" << stats.b0_mc() << std::endl;
	log.stream().flush();
	log.stream().close();
}

void handle_signal( int signum )
{
	NFF::Log&		log = NFF::Log::instance();
	NFF::Planner_Stats&	stats = NFF::Planner_Stats::instance();
	log.stream() << ";; heuristic_computations=" << stats.heuristic_computations() << std::endl;
	log.stream() << ";; expanded_ehc=" << stats.expanded_ehc() << std::endl;
	log.stream() << ";; generated_ehc=" << stats.generated_ehc() << std::endl;
	log.stream() << ";; expanded_bfs=" << stats.expanded_bfs() << std::endl;
	log.stream() << ";; generated_bfs=" << stats.generated_bfs() << std::endl;
	log.stream() << ";; primary_expansions=" << stats.primary_expansions() << std::endl;
	log.stream() << ";; secondary_expansions=" << stats.secondary_expansions() << std::endl;
	log.stream() << ";; tertiary_expansions=" << stats.tertiary_expansions() << std::endl;
	log.stream() << ";; entailment_tests=" << stats.entailment_tests() << std::endl;
	log.stream() << ";; sat_solver_calls=" << stats.sat_solver_calls() << std::endl;
	log.stream() << ";; max_num_clauses=" << stats.max_num_clauses() << std::endl;
	log.stream() << ";; problem_inconsistent=" << ( stats.problem_inconsistent() ? "yes" : "no" ) << std::endl;
	log.stream() << ";; initial_sample_h=" << stats.initial_sample_heuristic() << std::endl;
	log.stream() << ";; initial_card_h=" << stats.initial_card_heuristic() << std::endl;
	log.stream() << ";; avg_dead_models=" << stats.num_dead_models() << std::endl;
	log.stream() << ";; dead_samples_replaced=" << stats.dead_samples_replaced() << std::endl;
	log.stream() << ";; width_1=" << ( stats.is_width_1() ? "yes" : "no" ) << std::endl;
//	log.stream() << ";; hash_size=" << stats.hashtable_nodes() << std::endl;
	log.stream() << ";; nodes_mem=" << stats.node_megabytes() << std::endl;
	log.stream() << ";; b0_mc=" << stats.b0_mc() << std::endl;
	log.stream().close();

	std::exit( signum );
}

void register_signal_handlers()
{
	int signals[] = { SIGXCPU, SIGTERM, SIGTRAP, SIGABRT, SIGFPE, SIGBUS, SIGSEGV };
	for ( int i = 0; i < 7; i++ ) 
		signal( signals[i], handle_signal );
}

int main( int argc, char** argv )
{
	register_signal_handlers();
	double t0, tf;

	NFF_Options::parse_command_line( argc, argv );
	NFF_Options&	prog_opts = NFF_Options::instance();
        NFF::Ipc&	ipc = NFF::Ipc::instance();
	NFF::Log&	log = NFF::Log::instance();
	log.stream() <<	";; t1 - A Heuristic Search Conformant Planner" << std::endl;
	log.stream() << ";; Alexandre Albore, Hector Geffner & Miquel Ramirez (c) 2010" << std::endl;

	PDDL::Task& task = PDDL::Task::instance();
	task.setup();

	t0 = time_used();
	
	if ( prog_opts.use_invariant_xors() || prog_opts.use_invariant_diameter() )
	{
		NFF::Mutexes& m = NFF::Mutexes::instance();
		task.set_mutexes( &m );
		m.create_mutexes();
	}
	
        task.calculate_relevance();


	tf = time_used();
	log.stream() << ";; preprocessing="; report_interval( t0, tf, log.stream() );

	if ( prog_opts.verbose_mode() )
	{
		std::ofstream fluents_out( "fluents.list" );
		std::ofstream ops_out( "operators.list" );
		std::ofstream init_out( "initial.list" );
		std::ofstream goal_out( "goal.list" );
	
		task.print_fluents( fluents_out );
		task.print_operators( ops_out );
		task.print_initial_state( init_out );
		task.print_goal_state( goal_out );

		fluents_out.close();
		ops_out.close();
	}
	std::cout << "Fluents=" << task.fluents().size() << std::endl;
	std::cout << "Operators=" << task.useful_ops().size() << std::endl;
	log.stream() << ";; domain=" << task.domain_name() << std::endl;
	log.stream() << ";; problem=" << task.problem_name() << std::endl;
	log.stream() << ";; fluents=" << task.fluents().size() << std::endl;
	log.stream() << ";; operators=" << task.useful_ops().size() << std::endl;

        if ( prog_opts.output_ipc() )
        {
                ipc.stream() << task.fluents().size()-1 << std::endl;
                for (unsigned f = 0; f < task.fluents().size()-1; f++)
                {
                        task.print_fluent( f, ipc.stream() );
                        ipc.stream() << " ";
                }
                ipc.stream() << std::endl << "%%" << std::endl;
                ipc.stream() << task.prototype_ops().size() << std::endl;
                for (unsigned f = 0; f < task.prototype_ops().size(); f++)
                {
                        task.print_operator( task.prototype_ops()[f], ipc.stream() );
                        ipc.stream() << " ";
                }
                ipc.stream() << std::endl << "%%" << std::endl;
        }
	if ( NFF_Options::instance().only_grounding() )
	{
		std::cout << "Stopping after PDDL task has been processed" << std::endl;
		std::exit(0);
	}
	
	/** Now starts the proper search **/	

	NFF::BeliefSearch::BeliefSearch search;
	if ( !prog_opts.test_sequence().empty() )
	{
		std::vector<PDDL::Operator*> plan_prefix;
		load_test_sequence( prog_opts.test_sequence(), plan_prefix );
		search.execute( plan_prefix );
		std::exit(0);
	}
	if ( prog_opts.only_initial_node() )
	{
		search.process_root_node();
		handle_exit();
		std::exit(0);
	}

	search.solve();
	handle_exit();
	std::exit(0);
	return 0;
}
