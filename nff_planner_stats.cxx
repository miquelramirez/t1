#include "nff_planner_stats.hxx"

namespace NFF
{
	Planner_Stats::Planner_Stats()
		: m_expanded_ehc( 0 ),
                  m_generated_ehc( 0 ),
                  m_expanded_bfs( 0 ),
                  m_generated_bfs( 0 ),
                  m_entailment_tests( 0 ),
                  m_max_clauses( 0 ),
                  m_sat_solver_calls( 0 ),
                  m_heuristic_computations( 0 ),
                  m_problem_inconsistent( false ),
                  m_primary_expansions(0),
                  m_secondary_expansions(0),
                  m_tertiary_expansions(0),
                  m_num_dead_models(0.0f),
                  m_ndm_samples(0),
                  m_dead_samples_replaced(0),
                  m_width_1( false ),
                  m_hashtable_nodes(0),
                  m_accum_node_bytes(0.0f)
	{
	}
	
	Planner_Stats::~Planner_Stats()
	{
	}

	Planner_Stats&	Planner_Stats::instance()
	{
		static Planner_Stats instance;
		return instance;
	}
}
