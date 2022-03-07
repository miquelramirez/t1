/*
  Miguel Ramirez, Alexandre Albore, Hector Geffner
  C^3 conformant
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
#ifndef __NFF_DIJKSTRA__
#define __NFF_DIJKSTRA__

#include "PDDL.hxx"
#include <vector>
#include <iostream>
#include <queue>
#include <libdnnf/dnnf_theory.hxx>
#include <algorithm>
#include "nff.hxx"
#include "nff_belief.hxx"

namespace NFF
{
	class Fluent;
	//  class DNNF::DNNF_Theory;
	//  class Literals_Comparer;
	class SearchNode;	
	class Dijkstra
	{
	
	public:
	//    explicit 
	//    Dijkstra( std::vector<unsigned>& lits, std::vector<NFF::State>& states );
	//    Dijkstra( std::vector<unsigned>* lits, std::vector<NFF::State>* states );
	//  Dijkstra( Dijkstra& b );
	Dijkstra( unsigned approx_sz );
	~Dijkstra();

	typedef std::pair<int,int> BeliefLiteral;

	class Rule {
	public:
      
		// AA: DEBUG PURPOSE
		int 		op;
		bool		dirty;
		unsigned	value;

		Rule()
			: dirty(true), value(0)
		{
		}
	private:
		unsigned cost; // c(a,r): cost and penalty
		unsigned penalty; 
	public:
		std::vector<BeliefLiteral> head ;// C(r): conditions
		BeliefLiteral effect ; // p(r): literal effect
      
		void set_cost( unsigned c )
		{
			cost = c;
		}

		unsigned get_cost()
		{
			return cost;
		}
      
		void set_penalty( unsigned c ) 
		{
			penalty = c;
		}

		unsigned get_penalty()
		{
			return penalty;
		}

		bool inRuleHead( BeliefLiteral lit )
		{
			if ( std::find(head.begin(), head.end(), lit) == head.end() )
				return false;
			return true;
		}

		void	compute_value_max( Dijkstra& heuristic )
		{
			if ( !dirty ) return;
			value = 0;
			for ( unsigned k = 0; k < head.size(); k++ )
			{
				int h = heuristic.h_value(head[k]);
				if ( h == infty() ) 
				{
					value = h;
					return;
				}
				value = ( h > value ? h : value );
			}
			value = std::add( value, get_cost() );
			value = std::add( value, get_penalty() );
			dirty = false;
		}

		void	compute_value_additive( Dijkstra& heuristic )
		{
			if ( !dirty ) return;
			value = 0;
			for ( unsigned k = 0; k < head.size(); k++ )
			{
				int h = heuristic.h_value(head[k]);
				if ( h == infty() ) 
				{
					value = h;
					return;
				}
				value += h;
			}
			value = std::add( value, get_cost() );
			value = std::add( value, get_penalty() );
			dirty = false;
		}
	};
//   	void	pre_compute( SearchNode& n ); 
        void	pre_compute( SearchNode& n, std::vector< std::vector<int> >& models );
	int  	compute(  Belief& b, std::vector< std::vector<int> >& models );

	void	compute_relaxed_plan( SearchNode& n );

	//    static Belief* make_initial_belief();
	static BeliefLiteral     make_goal_state( unsigned i );
	//    static void set_goal_state( BeliefLiteral b ) { m_goal = b; }  
	std::vector< std::vector<unsigned> >   heuristics_table();    


	std::vector< std::vector<unsigned> >	        heuristics_vec();

	
	void		print( std::ostream& os );
	void		print_indices( std::ostream& os );
	void 		print_rules ( std::string fname, std::vector<Rule>& rules_vec );
	void                add_lit(unsigned lit);
	
		/* Adds rule to one of the 3 rules vectors, returns its index */
	int             add_static_rule( PDDL::Operator* op );
	int	        add_static_rule( unsigned op_idx );
	
	int             add_condeff_rule( PDDL::Operator* op );
	int		add_condeff_rule( unsigned op_idx );
	void            set_penalty(unsigned pen);
	unsigned        penalty();
	void	    	make_merge_rules();
	void		make_head_w_mutexes( std::vector<BeliefLiteral>& head, std::vector<BeliefLiteral>& cond, unsigned state_index );
	void	    	make_states_rules();
	void	    	make_static_rules();
	void            make_action_compilation();
	std::vector<Rule>&  states_rules () {      return  m_states_rules;    }
	std::vector<Rule>&  merge_rules () {      return  m_merge_rules;    }    

        unsigned xor_h_value( SearchNode& n, std::vector<std::vector<int> >& m );
	unsigned diameter_h_value( SearchNode& n );
        unsigned h_diameter( unsigned p, unsigned q);

	unsigned h_value()
	{
		return h_value(goal_state());
	}

	/* Returns the heuristic value of a literal */
	unsigned h_value( BeliefLiteral l)
	{
		return m_heuristic_table[l.first][l.second];
	}

	/* Returns the heuristic value of a literal i in model j */
	unsigned h_value( unsigned i , unsigned j)
	{
		return m_heuristic_table[i][j];
	}

	unsigned hS_value( )
	{
		return m_hS_rp_value;	
	}
	
	unsigned hX_value( )
	{
		return m_hX_rp_value;
	}

	std::vector<unsigned>&	hS_helpful()
	{
		return m_hS_helpful;
	}

	std::vector<unsigned>&	hX_helpful()
	{
		return m_hX_helpful;
	}
	
	unsigned	tie_breaking()
	{
		return m_cheapest_goal_estimate;
	}

	inline void set_supporter( Dijkstra::BeliefLiteral l, unsigned rule )
	{
		m_supporters_table[l.first][l.second] = rule;
	}

	inline	unsigned get_supporter( Dijkstra::BeliefLiteral l ) 
	{
		return m_supporters_table[l.first][l.second];
	}

	inline void set_h_value( Dijkstra::BeliefLiteral l, int h)
	{
		m_heuristic_table[l.first][l.second] = h;
	}

	inline void set_h_value( int l, int s, unsigned h)
	{
		m_heuristic_table[l][s] = h;
	}

	BeliefLiteral new_lit( PDDL::Fluent* f, int s);
    
	void processed( BeliefLiteral l );
    
	void not_processed( BeliefLiteral l );
    
	void not_processed( int i, int j );
    
	bool is_processed( BeliefLiteral l );

	// Gives back the node with the lower heuristic value
	class Literals_Comparer
	{
	protected:
      
		std::vector<std::vector<unsigned> >& m_h_table;
      
	public:
      
		Literals_Comparer(std::vector<std::vector<unsigned> >& hvalues  )
			: m_h_table (hvalues)
		{
			//	m_h_table = hvalues;
		}
      
		/* Returns: True h(a) > h(b)
		*          False if h(b) > h(a)  */
		bool operator()( BeliefLiteral a, BeliefLiteral b )
		{
			if ( m_h_table[a.first][a.second] > m_h_table[b.first][b.second] )
				return true;
	
			return false;
		}
	
	};
    

	typedef std::priority_queue<BeliefLiteral, 
			std::vector<BeliefLiteral>, 
			Literals_Comparer>  queue; 


	/* try version, where states are int */
	void   initialize_queue_and_heuristics(Belief& b, std::vector< std::vector<int> >& models, queue &q);
	std::vector< std::vector<bool> >	     m_processed; // [0..TOT_lits][0..Tot_states]   

        void print_rule( Rule &r );
	void print_h_table();
	void print_h_table( std::ostream & e );
	void print_h_table_file();
	void print_h_table_file( std::string fname );
	std::vector< std::vector<unsigned> >   m_heuristic_table; // [0..TOT_lits][0..Tot_states]
	std::vector< std::vector<unsigned> >   m_supporters_table;
	// MRJ: Layered rules vec
	std::vector< std::vector<std::pair<unsigned,unsigned> > >   m_lrr_vec;
	std::vector< unsigned >		       m_hS_relaxed_plan;	
	std::vector< unsigned >		       m_hS_helpful;
	std::vector< unsigned >		       m_hX_relaxed_plan;	
	std::vector< unsigned >		       m_hX_helpful;

	// For relaxed plan extraction
	std::vector< bool >		       	m_layer_ops;
	std::vector< bool >			m_seen_effs;	
	std::vector< bool >		       	m_seen_lits; 
	static const unsigned		       	null_rule;
	unsigned			       	m_hS_rp_value;
	unsigned			       	m_hX_rp_value;
	unsigned				m_cheapest_goal_estimate;
                void	compute_hX_rp( SearchNode& n, std::vector<std::vector<int> >& m );
                void	compute_hS_rp( SearchNode& n, std::vector<std::vector<int> >& m );
	void	compute_cheapest_unachieved_goal( SearchNode& n );
	void	initialize_effect_layers( 	std::vector< std::vector< unsigned > >& eff_layers,	
						PDDL::Fluent_Set& rp_precs,				
						PDDL::Fluent_Set& mask );

        bool	condition_holds_in_sample( Atom_Vec& cond, std::vector<std::vector<int> >& models );


	void extract_relaxed_plan_from_hadd( 	std::vector< std::vector< unsigned > >& eff_layers,
						PDDL::Fluent_Set& rp_precs,
						Operator_Vec&	relaxed_plan_actions,
						Operator_Vec&	helpful_actions );
	void extract_relaxed_plan_from_hmax(    std::vector< std::vector< unsigned > >& eff_layers,
						PDDL::Fluent_Set& rp_precs,
						Operator_Vec&	relaxed_plan_actions,
						Operator_Vec&	helpful_actions );

	void complete_helpful_actions( 	SearchNode& b, 
					PDDL::Fluent_Set& rp_precs, 
					Operator_Vec& helpful_actions,
                                        std::vector<std::vector<int> >& samples );


	PDDL::Task&	sm_task;
	
	protected:

	//    std::vector<BeliefLiteral>*    m_lits;  // [0..TOT_lits]
	
	std::vector<Rule>	     		m_static_rules;
	std::vector<Rule>	     		m_merge_rules;
	std::vector<Rule>	     		m_states_rules; // the conditional rules <l, s>
	std::vector<Rule>	     		m_all_rules;
	std::vector<std::vector<unsigned> >	m_lit_to_rule_head_table;
	BeliefLiteral     	m_goal;
	unsigned 		m_penalty;
	unsigned 		m_all_states_index;
	PDDL::Fluent_Set	m_goal_mask;
	PDDL::Fluent_Set	m_ext_goal_mask;
	unsigned 		m_num_fluents;	
	
	
	public:
	std::vector<unsigned>&	get_head_rules( BeliefLiteral& l );
	int		   belief_lit_index( BeliefLiteral& l );
	std::vector<Rule>& get_static_rules() {return m_static_rules;}
	std::vector<Rule>& get_merge_rules() {return m_merge_rules;}
	std::vector<Rule>& get_states_rules() {return m_states_rules;}
	std::vector<Rule>& get_all_rules()    {return m_all_rules;}
	
	bool instate( int value, std::vector<int>& myvector );
	bool instate_unsigned ( const int& value, std::vector<unsigned> myvector );
	std::vector<Rule>& unify_all_rules() {
		m_all_rules.clear();
		m_all_rules.insert( m_all_rules.end(), m_states_rules.begin(), m_states_rules.end() );
		m_all_rules.insert( m_all_rules.end(), m_merge_rules.begin(), m_merge_rules.end() );
		m_all_rules.insert( m_all_rules.end(), m_static_rules.begin(), m_static_rules.end() );

		return m_all_rules;
	}
	void		make_lit_to_rule_table();
	BeliefLiteral     goal_state() { return m_goal; }
	};

	inline	std::vector<unsigned>&	Dijkstra::get_head_rules( BeliefLiteral& l )
	{
		return m_lit_to_rule_head_table[ belief_lit_index(l) ];
	}

	inline	int	Dijkstra::belief_lit_index( BeliefLiteral& l )
	{
		return l.first*(m_all_states_index+1) + l.second;
	} 


	inline  std::vector< std::vector<unsigned> >   Dijkstra::heuristics_vec()
	{
		return m_heuristic_table;
	}

	inline  std::vector< std::vector<unsigned> >   Dijkstra::heuristics_table()
	{
		return m_heuristic_table;
	}

      	inline void Dijkstra::set_penalty(unsigned pen)
	{
		m_penalty = pen;
	}

	inline unsigned Dijkstra::penalty()
	{
		return m_penalty;
	}
}

#endif // nff_dijkstra.hxx
