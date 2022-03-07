/*
    Miguel Ramirez, Nir Lipovetzky, Alexandre Albore, Hector Geffner
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
#include "nff_options.hxx"
#include "global_options.hxx"
#include <getopt.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>

const char* NFF_Options::m_optstring = "d:i:a:pvh?g:t:IACOxrW:S:f:Dy:E:H:X:w:";

NFF_Options::NFF_Options()
        : m_verbose( false ),m_only_grounding( false ), m_only_initial(false),
          m_use_action_compilation(false), m_use_static_ors(false), 
          m_use_invariant_xors(false), m_use_invariant_diameter(false), 
          m_improvement_reward( 100 ),	m_ipc_output(false),
          m_search_strategy( "ff" ), m_helpful_ratio(10),
          m_dynamic_samples( false ), m_cond_eff_penalty( 0 ),
          m_ehc_heuristic( OPT_EHC_USES_HS ), m_heuristic_function( OPT_ADDITIVE_H ),
          m_hX_evaluation( OPT_HX_COUNTING ), m_combination_weight( 5 )
{
}

NFF_Options::~NFF_Options()
{
}

NFF_Options& NFF_Options::instance()
{
	static NFF_Options instance;
	return instance;
}

void NFF_Options::parse_command_line( int argc, char** argv )
{
	int opt = getopt( argc, argv, m_optstring );
	bool domain_specified = false;
	bool instance_specified = false;
	
	Global_Options& g_options = Global_Options::instance();
	NFF_Options& options = instance();
	
	while( opt != -1 )
	{
		switch (opt)
		{
		case 'd' : // domain file
			options.m_domain_fname = optarg;
			g_options.set_domain_filename( options.m_domain_fname );
			domain_specified = true;
			break;
		case 'f' :
			options.m_helpful_ratio = atoi(optarg);
			break;
		case 'g' :
		{
			int step = atoi(optarg);
			switch( step )
			{
			case 1:
				std::cout << "Stopping after Grounding" << std::endl; 
				options.m_only_grounding = true;
				break;
			default:
				std::cerr << "Unrecognized program step ("<< step << "), aborting" << std::endl;
				print_usage();
				exit(0);
				break;	
			}
			break;
		}
		case 'i' : // instance file
			options.m_instance_fname = optarg;
			g_options.set_instance_filename( options.m_instance_fname );
			instance_specified = true;
			break;
		case 'p' :
			g_options.set_only_preconds_and_goals_mode(true);
			break;
		case 'r' :
                        if (options.m_use_invariant_xors)
                        {
                                std::cout << "Cannot use -r and -x at the same time" << std::endl;
                                print_usage();
                                exit(0);
                        }
			options.m_use_invariant_diameter = true;
                        //AA: option actually disabled
                        std::cout << "Option -r has been disabled. Thanks." << std::endl;
                        exit(0);
			break;
		case 't':
			options.m_test_sequence = optarg;
			break;
		case 'v' : // verbose mode activated
			options.m_verbose = true;
			break;
		case 'w' :
			options.m_combination_weight = atoi(optarg);
			break;
		case 'x' :
                        if (options.m_use_invariant_diameter)
                        {
                                std::cout << "Cannot use -r and -x at the same time" << std::endl;
                                print_usage();
                                exit(0);
                        }
			options.m_use_invariant_xors = true;
			break;
		case 'y' :
			options.m_cond_eff_penalty = atoi( optarg );
			break;
		case 'A' :
			options.m_use_action_compilation = true;
			break;
		case 'C' :
			options.m_ipc_output = true;
			break;
		case 'D' :
			options.m_dynamic_samples = true;
			break;
		case 'E' :
		{
			std::vector<std::string>  hset;
			hset.push_back( "hS" );
			hset.push_back( "hX" );
			std::string tmp( optarg );
			if ( std::find( hset.begin(), hset.end(), tmp )
				== hset.end() )
			{
				std::cerr << "Unknown " << tmp << " heuristic specified for EHC" << std::endl;
				std::exit(1);
			}
			if ( tmp == "hS" ) options.m_ehc_heuristic = OPT_EHC_USES_HS;
			if ( tmp == "hX" )
			{
				options.m_ehc_heuristic = OPT_EHC_USES_HX;
				options.m_use_invariant_xors = true;
			}
			break;
		}
		case 'H' :
		{
			std::vector<std::string> fset;
			fset.push_back( "add" );
			fset.push_back( "max" );
			std::string tmp(optarg);
			if ( std::find( fset.begin(), fset.end(), tmp ) 
				== fset.end() )
			{
				std::cerr << "Unknown " << tmp << " function specified for the heuristic" << std::endl;
				std::exit(1);
			}
			if ( tmp == "add" ) options.m_heuristic_function = OPT_ADDITIVE_H;
			if ( tmp == "max" ) options.m_heuristic_function = OPT_MAX_H;
			break;
		}
		case 'I' :
			options.m_only_initial = true;
			break;
		case 'O' :
			options.m_use_static_ors = true;
			break;
		case 'S' :
		{
			std::vector<std::string> strategies;
			strategies.push_back( "ehc" );
			strategies.push_back( "dqbfs" );
			strategies.push_back( "bfs" );
			strategies.push_back( "ff" );
			strategies.push_back( "mqbfs" );
			options.m_search_strategy = optarg;
			if ( std::find( strategies.begin(), strategies.end(), 
					options.m_search_strategy ) == strategies.end() )
			{
				std::cerr << "Search strategy '" << options.m_search_strategy << "' not recognized" << std::endl;
				std::exit(1);
			}	
			break;
		}
		case 'W' :
			options.m_improvement_reward = atoi( optarg );
			break;
		case 'X' :
		{
			std::vector<std::string> fset;
			fset.push_back( "add" );
			fset.push_back( "count" );
			std::string tmp(optarg);
			if ( std::find( fset.begin(), fset.end(), tmp ) 
				== fset.end() )
			{
				std::cerr << "Unknown " << tmp << " function specified for the heuristic" << std::endl;
				std::exit(1);
			}
			if ( tmp == "count" ) options.m_hX_evaluation = OPT_HX_COUNTING;
			if ( tmp == "add" ) options.m_hX_evaluation = OPT_HX_ADDING;
			break;
		}
		case '?':
		case 'h':
			print_usage();
			exit(0);
			break;
		default:
			std::cerr << "Unrecognized option" << opt << std::endl;
			print_usage();
		}
		opt = getopt( argc, argv, m_optstring );
	}

	if ( !domain_specified )
	{
		std::cerr << "PDDL domain description not specified" << std::endl;
		print_usage();
		exit(1);
	}	
	if ( !instance_specified )
	{
		std::cerr << "PDDL instance description not specified" << std::endl;
		print_usage();
		exit(1);
	}
}

void NFF_Options::print_usage()
{
	std::cerr << "T1 (c) Universitat Pompeu Fabra, 2010" << std::endl;
	std::cerr << "Authors: Hector Geffner, Alexandre Albore, Miguel Ramirez" << std::endl;
	std::cerr << std::endl;
	std::cerr << "Usage: ./nff -d <domain file> -i <instance file> [-v|-g:1]" << std::endl << std::endl;
	std::cerr << "Mandatory parameters:" << std::endl;
	std::cerr << "-d            Domain specification in PDDL 2.1" << std::endl;
	std::cerr << "-i            Instance specification in PDDL 2.1" << std::endl;
	std::cerr << std::endl;
	std::cerr << "Optional parameters: " << std::endl;
	std::cerr << "-a            Plan specification to validate" << std::endl;
	std::cerr << "-v            Verbose Mode ON (default is OFF)" << std::endl;
	std::cerr << "-g <number>   Debugging - Stop execution when some process step completes:" << std::endl;
	std::cerr << "      1                   PDDL parsing, ADL -> STRIPS, and preprocessing finishes" << std::endl;
	std::cerr << "-t <file>     Execute sequence of actions and display node info as actions are executed" << std::endl;
	std::cerr << "-I            Process initial node (for heuristic debugging)" << std::endl;
	std::cerr << "-p            Only Precondition and Goals are tested during Belief Tracking" << std::endl;
	std::cerr << "-A            Add Action Compilation rules (AAAI-06) to heuristic" << std::endl;
	std::cerr << "-O            Add Static OR rules (AAAI-06) to heuristic" << std::endl;
	std::cerr << "-x            Compute invariant XOR sets and extend heuristic goal rule" << std::endl;
	std::cerr << "-m            Compute invariant XOR sets and use diameter heuristic (disabled)" << std::endl;
	std::cerr << "-W            Reward awarded to a heuristic when its minimum f(n) is improved" << std::endl;
	std::cerr << "-S <name>     Search strategy. Currently implemented:" << std::endl;
	std::cerr << "                 ehc         - Enforced Hill-Climbing" << std::endl;
	std::cerr << "                 bfs        - Greedy Best-First Search, f(n) = h(n)" << std::endl;
	std::cerr << "                 ff          - Enforced Hill-Climbing + Greedy Best-First Search" << std::endl;
	std::cerr << "                 dqbfs       - Dual queue (helpful, non-helpful) Best-First Search" << std::endl;
	std::cerr << "-f <num>      Ratio of helpful to non-helpful action expansions <num>:1 (only used when Dual Queue BFS is enabled" << std::endl;
}
