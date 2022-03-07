/*
  Miguel Ramirez, Alexandre Albore, Hector Geffner
  T1 conformant planner
  Copyright (C) 2010  

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
#ifndef __NNF_FOCUS_BFS__
#define __NNF_FOCUS_BFS__

#include "nff_search_node.hxx"
#include "nff_dijkstra.hxx"
#include "PDDL.hxx"
#include "hash_table.hxx"
#include <libdnnf/cnf_theory.hxx>
#include <libdnnf/cnf2dnnf.hxx>
#include <libdnnf/dnnf_theory.hxx>
#include "MinMaxHeap.hpp"
//#include <deque>
#include <queue>

namespace NFF
{
        const int MAX_MEMORY = 2000;
        const bool EVAL_ROOT_CHILDS = false;
        

        class BeliefSearch
        {
                enum Open_List_Types
                {
                        hS_HA = 0,
                        hX_HA = 1,
                        not_HA = 2
                };

                enum	Combined_Open_List_Types
                {
                        HA_open = 0,
                        not_HA_open
                };

                struct	Combined_BFS_Node_Comparer
                {
                        typedef SearchNode* value_type;
                        bool operator()( SearchNode* a, SearchNode* b )
                        {
                                if ( a->fn == b-> fn )
                                {
                                        if ( a-> gn == b->gn )
                                                return a->tb > b->tb;
                                        return a->gn > b->gn;
                                }
                                return a->fn > b->fn;
                        }
                };

    
                // Equivalent to the function std::less()
                struct Greedy_BFS_Node_Comparer2
                {
                        typedef SearchNode*	value_type;
      
                        bool operator()( const SearchNode* a, const SearchNode* b ) const
                        {	
                                assert( a->queue == b->queue );
                                if ( a->fn == b->fn )
                                {
                                        unsigned a_sec_h = ( a->queue != hX_HA ? a->hX : a->hS );
                                        unsigned b_sec_h = ( b->queue != hX_HA ? b->hX : b->hS );
                                        if ( a_sec_h == b_sec_h )
                                        {
// AA: uncomment if you want to use hd as tie breaker.
//                                        unsigned ha = a->heuristics->diameter_h_value(*a);
//                                        unsigned hb = b->heuristics->diameter_h_value(*b);
//                                        if ( ha == hb )
                                                {
                                                        /*     if ( a->tb == b->tb )
                                                               {
                                                               return a->gn < b->gn;
                                                               } */
                                                        return a->tb < b->tb;
                                                }
//                                        return ha < hb;
                                        }
                                        return a_sec_h < b_sec_h;
                                }
                                return a->fn < b->fn;
                        }
                }; 


                struct Greedy_BFS_Node_Comparer
                {
                        typedef SearchNode*	value_type;
                        
                        bool operator()( const SearchNode* a, const SearchNode* b ) const
                        {	
                                assert( a->queue == b->queue );
                                if ( a->fn == b->fn )
                                {
//AA: for BFS (to restore)
                                        unsigned a_sec_h = ( a->queue != hX_HA ? a->hX : a->hS );
                                        unsigned b_sec_h = ( b->queue != hX_HA ? b->hX : b->hS );
                                        if ( a_sec_h == b_sec_h )
                                        {
// AA: uncomment if you want to use hd as tie breaker.
//                                        unsigned ha = a->heuristics->diameter_h_value(*a);
//                                        unsigned hb = b->heuristics->diameter_h_value(*b);
//                                        if ( ha == hb )
                                                {
                                                        if ( a->tb == b->tb )
                                                        {
                                                                return a->gn > b->gn;
                                                        }
                                                        return a->tb > b->tb;
                                                }
//                                        return ha > hb;
                                        }
// to restore                
                                        return a_sec_h > b_sec_h;
                                }
                                return a->fn > b->fn;
                }
        }; 


        struct EHC_Node_Comparer
                {
                        typedef SearchNode*	value_type;
                        
                        bool operator()( SearchNode* a, SearchNode* b )
                        {	
                                return a->tb > b->tb;
                        }
                }; 
        public:
                typedef          MinMaxHeap<    SearchNode*, 
                                                std::vector<SearchNode*>, 
                                                Greedy_BFS_Node_Comparer >	Sorted_Open_List2;
                typedef std::priority_queue<	SearchNode*, 
                                                std::vector<SearchNode*>, 
                                                Greedy_BFS_Node_Comparer >	Sorted_Open_List;
        
        typedef std::priority_queue<	SearchNode*,
                                        std::vector<SearchNode*>,
                                        Combined_BFS_Node_Comparer >	Combined_Open_List;
        typedef std::priority_queue<	SearchNode*, 
                                        std::vector<SearchNode*>, 
                                        EHC_Node_Comparer> 		EHC_Sorted_Open_List;

        typedef std::queue<		SearchNode*, 
					std::list<SearchNode*> > 	Unsorted_Open_List;
	typedef Hash_Table<SearchNode> 					Closed_List;


	BeliefSearch();
	~BeliefSearch();

	bool	solve();
	bool	failed();

	bool 		instate( int value, std::vector<int>& myvector );
	double 		compile( DNNF::CNF_Theory& cnf, DNNF::DNNF_Theory& dnnf );
	double 		count_models( DNNF::CNF_Theory& cnf);
	void 		make_initial_CNF( DNNF::CNF_Theory& initial );
	SearchNode* 	best_first_search ( );
	SearchNode*	enforced_hill_climbing();
	SearchNode*    	ff_search( );

	// Common
	bool		in_closed( SearchNode* n );
	void		close( SearchNode* n );
	bool		goal_achieved( SearchNode* n );
	template < typename Queue_Type >
	bool		search_space_exhausted( std::vector< Queue_Type> & open_set )
	{
		for ( unsigned k = 0; k < open_set.size(); k++ )
			if ( !open_set[k].empty() ) return false;
		return true;
	}


	// Dual-queue best first search
	void		initialize_multi_queue_search();
	SearchNode*	multi_queue_best_first_search();
	SearchNode*	dual_queue_best_first_search();
	SearchNode*	dual_queue_get_node_from_open();
	SearchNode*	multi_queue_get_node_from_open();
	void		multi_queue_expand( SearchNode* n );
	void		dual_queue_expand( SearchNode* n );
        void            dual_queue_expand_and_evaluate( SearchNode* n );
	void		report_queues_state();
	SearchNode*	execute_search_strategy();
	Dijkstra* 	heuristic() { return heuristics;}
	void		execute( std::vector<PDDL::Operator*>& prefix );
	void		process_root_node();
        std::vector<std::vector<int> >&        models();

	SearchNode* 	root() { return m_root; }
        void 		set_root( SearchNode* r ) {  m_root = r; }
public:
      	// For ease of use (again)
	//   std::vector<unsigned>	    path_found;
	unsigned        expanded;
	unsigned        evaluated;
protected:

        void print_model( std::vector<int>& model, DNNF::DNNF_Theory &initial_dnnf );

	double		approximate_initial_belief();  
	void		compute_relevant_prec_and_goals( Atom_Vec& fluents, PDDL::Fluent_Set& fluents_set );
	void		set_lit_weights_to(  DNNF::DNNF_Theory& t, Atom_Vec& L, int w );
        void		determine_problem_width( DNNF::DNNF_Theory& t, std::vector<bool>& fluent_widths, std::vector<bool>& zero_widths ); 
	void		decode_dnnf_model( char* dnnf_model, std::vector<int>& state );
	void		initialize_dnnf_lit_weights( DNNF::DNNF_Theory& ctau );
	bool		already_in_approx( std::vector<int>& state );

	unsigned	ehc_get_node_h( SearchNode* n );
	void		ehc_get_helpful_actions( SearchNode* n, std::vector< PDDL::Operator* >& ops );

        SearchNode*     improve_heuristic( SearchNode* n );
        void            print_the_queue( unsigned h );
      	Dijkstra* 	heuristics;

	static PDDL::Task&		sm_task;
	// MRJ: Matrix for holding the states we sample from
	//	the initial belief
	std::vector< std::vector< int > >       m_models;
    
	bool       				m_failed;
	SearchNode*				m_root;
	Closed_List				m_bfs_closed;

	std::vector< Sorted_Open_List >        m_open_set;
        std::vector<unsigned>                   last_heap_size;
	std::vector< unsigned >			m_open_counters;
	int					m_hS_score;
	unsigned				m_min_hS_fn;
	int					m_hX_score;
	unsigned				m_min_hX_fn;
	unsigned				m_selected_open;
};

inline bool BeliefSearch::failed() { return m_failed; }

inline bool	BeliefSearch::in_closed( SearchNode* n )
{
	SearchNode* duplicate = m_bfs_closed.get_element(n);
	return duplicate != NULL;
}

inline void	BeliefSearch::close( SearchNode* n )
{
	Planner_Stats& stats = Planner_Stats::instance();
	stats.add_node_size(n->size());
	m_bfs_closed.add_element(n);
}

inline bool	BeliefSearch::goal_achieved( SearchNode* n )
{
	return n->b->entails_goal();
}


}


#endif // nnf_focus_bfs.hxx
