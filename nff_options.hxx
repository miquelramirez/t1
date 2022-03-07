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
#ifndef __NFF_OPTIONS__
#define __NFF_OPTIONS__

#include <string>

enum Opt_Symbol
{
	OPT_EHC_USES_HX = 0,
	OPT_EHC_USES_HS,
	OPT_ADDITIVE_H,
	OPT_MAX_H,
	OPT_HX_COUNTING,
	OPT_HX_ADDING
};

class NFF_Options
{
public:
	static NFF_Options& 	instance();
	static void 		parse_command_line( int argc, char** argv );
	~NFF_Options();
	static void		print_usage();

	std::string& 		domain_filename() { return m_domain_fname; }
	std::string& 		instance_filename() { return m_instance_fname; }
	std::string&		test_sequence() { return m_test_sequence; }

	bool	     		verbose_mode() { return m_verbose; }
	bool			only_grounding() { return m_only_grounding; }
	bool			only_initial_node() { return m_only_initial; }
	bool			use_static_ors() { return m_use_static_ors; }
	bool			use_action_compilation() { return m_use_action_compilation; }
	bool			use_invariant_xors() { return m_use_invariant_xors; }
	bool			use_invariant_diameter() { return m_use_invariant_diameter; }
	int			improvement_reward() { return m_improvement_reward; }
	std::string		search_strategy() { return m_search_strategy; }
	int			helpful_ratio() { return m_helpful_ratio; }
	bool			dynamic_samples() { return m_dynamic_samples; }
	int			cond_eff_penalty() { return m_cond_eff_penalty; }
	Opt_Symbol		ehc_heuristic() { return m_ehc_heuristic; }
	Opt_Symbol		heuristic_function() { return m_heuristic_function; }
	Opt_Symbol		hX_evaluation() { return m_hX_evaluation; }
	unsigned		combination_weight() { return m_combination_weight; }
        bool                    output_ipc() { return m_ipc_output; }
private:
	NFF_Options();
private:

	std::string		m_domain_fname;
	std::string		m_instance_fname;
	bool			m_verbose;
	static const char*	m_optstring;
	bool			m_only_grounding;
	std::string		m_test_sequence;
	bool			m_only_initial;
	bool			m_use_action_compilation;
	bool			m_use_static_ors;
	bool			m_use_invariant_xors;
	bool			m_use_invariant_diameter;
	int			m_improvement_reward;
	std::string		m_search_strategy;
	int			m_helpful_ratio;
	bool			m_dynamic_samples;
	int			m_cond_eff_penalty;
	Opt_Symbol		m_ehc_heuristic;
	Opt_Symbol		m_heuristic_function;
	Opt_Symbol		m_hX_evaluation;
	unsigned		m_combination_weight;
        bool                    m_ipc_output;
};


#endif // nff_options.hxx
