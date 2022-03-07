#ifndef __NFF_PLANNER_STATS__
#define __NFF_PLANNER_STATS__

namespace NFF
{

class Planner_Stats
{
public:

	~Planner_Stats();
	static	Planner_Stats&	instance();
	
	void		notify_expanded_ehc();
	unsigned	expanded_ehc() const;
	void		notify_generated_ehc();
	unsigned	generated_ehc() const;
	void		notify_expanded_bfs();
	unsigned	expanded_bfs() const;
	void		notify_generated_bfs();
	unsigned	generated_bfs() const;
	void		notify_entailment_test();
	unsigned	entailment_tests() const;
	void		notify_sat_solver_call();
	unsigned	sat_solver_calls() const;
	void		notify_num_clauses( unsigned nClauses );
	unsigned	max_num_clauses() const;
	void		notify_heuristic_computation();
	unsigned	heuristic_computations() const;	
	void		report_problem_inconsistent();
	bool		problem_inconsistent() const;
	void		notify_primary_expansion();
	unsigned	primary_expansions() const;
	void		notify_secondary_expansion();
	unsigned	secondary_expansions() const;
	void		notify_tertiary_expansion();
	unsigned	tertiary_expansions() const;
	void		notify_initial_sample_heuristic( unsigned v );
	unsigned	initial_sample_heuristic() const;
	void		notify_initial_card_heuristic(unsigned v);
	unsigned	initial_card_heuristic() const;
	void		notify_num_dead_models( unsigned n );
	float		num_dead_models() const;
	void		notify_dead_sample_replaced();
	unsigned	dead_samples_replaced();
	void		notify_width_1();
	bool		is_width_1() const;
	void		add_node_size( float nBytes );
	void		substract_node_size( float nBytes );
 	void		notify_hashtable_node(unsigned n);
        unsigned	hashtable_nodes() const;
	float		node_megabytes() const;
	void		notify_b0_mc( double nModels );
	double		b0_mc() const;
	

protected:

	Planner_Stats();

protected:

	unsigned	m_expanded_ehc;
	unsigned	m_generated_ehc;
	unsigned	m_expanded_bfs;
	unsigned	m_generated_bfs;
	unsigned	m_entailment_tests;
	unsigned	m_max_clauses;
	unsigned	m_sat_solver_calls;
	unsigned	m_heuristic_computations;
	bool		m_problem_inconsistent;
	unsigned	m_primary_expansions;
	unsigned	m_secondary_expansions;
	unsigned	m_tertiary_expansions;
	unsigned	m_initial_sample_heuristic;
	unsigned	m_initial_card_heuristic;
	float		m_num_dead_models;
	unsigned	m_ndm_samples;
	unsigned	m_dead_samples_replaced;
	bool		m_width_1;
	float		m_accum_node_bytes;
        unsigned        m_hashtable_nodes;
	double		m_b0_mc;

};
inline	void		Planner_Stats::notify_b0_mc( double nModels )
{
	m_b0_mc = nModels;
}

inline	double		Planner_Stats::b0_mc() const
{
	return m_b0_mc;
}

inline void		Planner_Stats::substract_node_size( float nBytes )
{
	static const float MBytes = 1024.0f*1024.0f;
	m_accum_node_bytes -= (nBytes / MBytes);
}

inline void		Planner_Stats::add_node_size( float nBytes )
{
	static const float MBytes = 1024.0f*1024.0f;
	m_accum_node_bytes += (nBytes / MBytes);
}

inline float		Planner_Stats::node_megabytes() const
{
	return m_accum_node_bytes;
}

inline void		Planner_Stats::notify_tertiary_expansion()
{
	m_tertiary_expansions++;
}

inline unsigned		Planner_Stats::tertiary_expansions() const
{
	return m_tertiary_expansions;
}


inline void		Planner_Stats::notify_width_1()
{
	m_width_1 = true;
}

inline	bool		Planner_Stats::is_width_1() const
{
	return m_width_1;
}


inline	void		Planner_Stats::notify_dead_sample_replaced()
{
	m_dead_samples_replaced++;
}

inline	unsigned	Planner_Stats::dead_samples_replaced()
{
	return m_dead_samples_replaced;
}

inline	void		Planner_Stats::notify_num_dead_models( unsigned n )
{
	m_num_dead_models += ((float)n - m_num_dead_models)/(float)(m_ndm_samples+1) ;
	m_ndm_samples++;
}

inline	float	Planner_Stats::num_dead_models() const
{
	return m_num_dead_models;
}

inline	void		Planner_Stats::notify_initial_sample_heuristic( unsigned v )
{
	m_initial_sample_heuristic = v;
}

inline unsigned		Planner_Stats::initial_sample_heuristic() const
{
	return m_initial_sample_heuristic;
}

inline	void		Planner_Stats::notify_initial_card_heuristic(unsigned v)
{
	m_initial_card_heuristic = v;
}

inline	unsigned	Planner_Stats::initial_card_heuristic() const
{
	return m_initial_card_heuristic;
}


inline void		Planner_Stats::report_problem_inconsistent()
{
	m_problem_inconsistent = true;
}

inline bool		Planner_Stats::problem_inconsistent() const
{
	return m_problem_inconsistent;
}

inline	void		Planner_Stats::notify_expanded_ehc()
{
	m_expanded_ehc++;
}

inline	unsigned	Planner_Stats::expanded_ehc() const
{
	return m_expanded_ehc;
}

inline  void		Planner_Stats::notify_generated_ehc()
{
	m_generated_ehc++;
}

inline	unsigned	Planner_Stats::generated_ehc() const
{
	return m_generated_ehc;
}

inline 	void		Planner_Stats::notify_expanded_bfs()
{
	m_expanded_bfs++;
}

inline	unsigned	Planner_Stats::expanded_bfs() const
{
	return m_expanded_bfs;
}

inline	void		Planner_Stats::notify_generated_bfs()
{
	m_generated_bfs++;
}

inline	unsigned	Planner_Stats::generated_bfs() const
{
	return m_generated_bfs;
}

inline	void		Planner_Stats::notify_entailment_test()
{
	m_entailment_tests++;
}

inline	unsigned	Planner_Stats::entailment_tests() const
{
	return m_entailment_tests;
}

inline	void		Planner_Stats::notify_sat_solver_call()
{
	m_sat_solver_calls++;
}

inline	unsigned	Planner_Stats::sat_solver_calls() const
{
	return m_sat_solver_calls;
}

inline	void		Planner_Stats::notify_num_clauses( unsigned nc )
{
	m_max_clauses = ( nc > m_max_clauses ? nc : m_max_clauses );
}

inline	unsigned	Planner_Stats::max_num_clauses() const
{
	return m_max_clauses;
}

inline 	void		Planner_Stats::notify_heuristic_computation()
{
	m_heuristic_computations++;
}

inline	unsigned	Planner_Stats::heuristic_computations() const
{
	return m_heuristic_computations;
}

inline	void		Planner_Stats::notify_primary_expansion()
{
	m_primary_expansions++;
}

inline	unsigned	Planner_Stats::primary_expansions() const
{
	return m_primary_expansions;
}

inline	void		Planner_Stats::notify_secondary_expansion()
{
	m_secondary_expansions++;
}

inline unsigned		Planner_Stats::secondary_expansions() const
{
	return m_secondary_expansions;
}

inline 	void		Planner_Stats::notify_hashtable_node(unsigned n)
{
	m_hashtable_nodes = n;
}

inline 	unsigned	Planner_Stats::hashtable_nodes() const
{
	return m_hashtable_nodes;
}
}
#endif // nff_planner_stats.hxx
