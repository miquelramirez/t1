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
#include "nff_dijkstra.hxx"
#include <stdlib.h>
#include <iterator>
#include <limits>
#include <set>
#include "nff.hxx"
#include "nff_options.hxx"
#include "nff_mutexes.hxx"
#include "nff_search_node.hxx"

#define PENALTY 1000
#define DEBUG_DIJKSTRA 0

namespace NFF
{

const unsigned Dijkstra::null_rule = std::numeric_limits<unsigned>::max();
 
bool Dijkstra::instate( int value, std::vector<int>& myvector )
{
	return std::find( myvector.begin(), myvector.end(), value) != myvector.end();
}

bool Dijkstra::instate_unsigned ( const int& value, std::vector<unsigned> myvector )
{
	std::vector<unsigned>::iterator first = myvector.begin();
	std::vector<unsigned>::iterator last = myvector.end();
	
	for ( ;first!=last; first++)
		if ( *first==(unsigned)value ) return true;
	return false;
}

Dijkstra::Dijkstra( unsigned approx_sz ) 
       	: sm_task( PDDL::Task::instance() ), m_all_states_index( approx_sz ),
	m_goal_mask( sm_task.fluent_count()+1), m_ext_goal_mask( sm_task.fluent_count()+1 )
{

    	m_num_fluents = sm_task.fluent_count();

	/* Test vector of states: s \in bel' */
	/* NB_ -1 is all_states */
	
	int num_models = m_all_states_index;
	m_goal = BeliefLiteral( m_num_fluents-1 , m_all_states_index );
	// resizing the heuristics matrix (dovrebbe essere max_fluents)
	m_heuristic_table.resize( m_num_fluents );
	m_supporters_table.resize( m_num_fluents );
	for (unsigned i = 0; i < m_heuristic_table.size(); ++i)
	{
		m_heuristic_table[i].resize( num_models+1 );
		m_supporters_table[i].resize( num_models+1 );
		// initialize to -1
		for (unsigned j = 0; j < m_heuristic_table[i].size(); j++) {
			m_heuristic_table[i][j] = infty();
			m_supporters_table[i][j] = null_rule;
		}
	}
	// resizing processed matrix
	m_processed.resize( m_num_fluents );
	for (unsigned i = 0; i < m_processed.size(); ++i)
	{
		m_processed[i].resize( num_models+1 );
		for (unsigned j = 0; j < m_processed[i].size(); j++)
			m_processed[i][j] = false;
	}
    
	m_seen_lits.resize(sm_task.fluents().size()*(m_all_states_index+1));
	m_seen_effs.resize( sm_task.useful_ops().size() );

	set_penalty( PENALTY );

	make_merge_rules();
	make_states_rules();

	NFF_Options&	opt = NFF_Options::instance();
	if ( opt.use_static_ors() )	
		make_static_rules();
	if ( opt.use_action_compilation() )
		make_action_compilation();

	m_all_rules.clear();
	m_all_rules.insert( m_all_rules.end(), m_states_rules.begin(), m_states_rules.end() );
	m_all_rules.insert( m_all_rules.end(), m_merge_rules.begin(), m_merge_rules.end() );
	m_all_rules.insert( m_all_rules.end(), m_static_rules.begin(), m_static_rules.end() );

	//print_rules( "merge_rules.list", m_merge_rules );
	//print_rules( "state_rules.list", m_states_rules );
	print_rules( "static_rules.list", m_static_rules );
	// MRJ: Once all rules vector contains every rule, we build the table binding
	// BeliefLiterals with rules' heads
	make_lit_to_rule_table();
	for ( unsigned k = 0; k < sm_task.goal_state().size(); k++ )
		m_goal_mask.set( sm_task.goal_state()[k] );
	if ( opt.use_invariant_xors() ||  opt.use_invariant_diameter() )
	{
		Mutexes& m = Mutexes::instance();
		for (unsigned kx = 0; kx < m.goal_invariants().size(); kx++)
		{
			unsigned kgi = m.goal_invariants()[kx];
			m_ext_goal_mask.set( kgi );
		}

	}
} 

Dijkstra::~Dijkstra()
{
}



Dijkstra::BeliefLiteral Dijkstra::new_lit( PDDL::Fluent* f, int s) 
{
	Dijkstra::BeliefLiteral l( f->code(), s );

	return l;
}


     /** ACTION COMPILATION **
      * Act(a) s.t.
      * Prec: P
      * Effects:
      *   C & -L -> L
      *   A -> -L
      * ==> P & C & -A -> L   */

void           Dijkstra::make_action_compilation()
{

	/* for al the actions */
	for (unsigned  i = 0; i < sm_task.prototype_ops().size(); i++)
	{
		PDDL::Operator* o = sm_task.prototype_ops()[i];

		/* for all the conditional effects */
		for ( unsigned m = 0; m < o->same_op_vec().size(); m++ )
		{
			PDDL::Operator* op1 =  o->same_op_vec().at(m);
	     
			for (unsigned j = 0; j < op1->add_vec().size(); j++)
			{
				unsigned lit = op1->add_vec()[j];
	
		       		if ( !sm_task.not_equivalent(lit) )
					continue;

				unsigned neg_lit = sm_task.not_equivalent(lit);

				if ( !(op1->preconds().isset( neg_lit )) )
					continue;

				/* NB: Now we known that op1 is s.t. C & -L -> L   */
		
			       	/* Builds the list of the operators deleting lit */
				std::vector<PDDL::Operator*> del_op_list;
				for (unsigned m2 = 0; m2 < o->same_op_vec().size(); m2++ )
				{
					if (m == m2)
						continue;

					PDDL::Operator * op2 = o->same_op_vec().at(m2);
		
				     	if ( op2->dels().isset( lit ))
						del_op_list.push_back( op2 );
				}

				Rule r;
				std::vector<BeliefLiteral> conditions;
				r.op = op1->get_idx();
				r.set_cost ( 1 );
				r.set_penalty( 0 );
				
				for ( unsigned k = 0; k < op1->prec_vec().size(); k++ )
				{
					if ( op1->prec_vec()[k] == neg_lit ) continue;

					BeliefLiteral ci ( op1->prec_vec()[k] , m_all_states_index );
					conditions.push_back(ci);
				}
				for ( unsigned k = 0; k < del_op_list.size(); k++ )
				{
					// NB: We make here an approx, selecting only one fluent for each condition
					if ( del_op_list[k]->prec_vec().size() > 0 )
					{
						if ( sm_task.not_equivalent(del_op_list[k]->prec_vec()[0]) )
						{
							BeliefLiteral ci ( sm_task.not_equivalent(del_op_list[k]->prec_vec()[0]) ,
								m_all_states_index );
							conditions.push_back(ci);
						}
						else
						{
#ifdef DEBUG 
							std::cout << "Warning: has not negative counterpart!" << std::endl;
							sm_task.print_operator(  del_op_list[k], std::cout );
							std::cout << std::endl;
#endif
						}
					}
				}
				r.head = conditions;
				BeliefLiteral pi ( lit , m_all_states_index );
				r.effect = pi;
		
				m_merge_rules.push_back(r);
			}
		}
		
	}

}



void Dijkstra::make_merge_rules( )
{
   	std::vector<PDDL::Fluent*>& fluents = sm_task.fluents();
	NFF_Options& opt = NFF_Options::instance();
	unsigned merge_cost = ( opt.heuristic_function() == OPT_ADDITIVE_H ? 0 : 1 );
      	for ( unsigned i = 1; i < fluents.size(); i++  )
       	{
       		Dijkstra::Rule r;
		r.op = -1;
	       	for ( unsigned j = 0; j < m_all_states_index; j++ )
			r.head.push_back(BeliefLiteral( i, j ));
		r.effect = BeliefLiteral( i, m_all_states_index );
		r.set_cost( merge_cost );
		r.set_penalty(0); 
		m_merge_rules.push_back(r);
	}
}


void Dijkstra::make_static_rules( )
{
	std::vector<unsigned> static_clauses;
        PDDL::Fluent_Set conditions_to_check;

	for ( unsigned oof = 0; oof < sm_task.oneofs().size(); oof++)
	{      		
		std::vector<unsigned>& clause = sm_task.oneofs().at(oof);

		for (unsigned i = 0; i < clause.size(); i++)
		{
			// Makes the rules: AND_{j!=i} -Lj -> Li
			Dijkstra::Rule r;
			r.op = -2;
			for (unsigned j = 0; j < clause.size(); j++)
			{
				if ( j == i )
					continue;
			  
				if (sm_task.not_equivalent(clause[j]) )
					r.head.push_back(BeliefLiteral( sm_task.not_equivalent(clause[j]), m_all_states_index ));
			}

			r.effect = BeliefLiteral( clause[i], m_all_states_index );
			r.set_cost( 0 );
			r.set_penalty(0); 
			m_static_rules.push_back(r);
		}
                std::cout << std::endl;

	}
}

void Dijkstra::make_head_w_mutexes( std::vector<BeliefLiteral>& head, std::vector<BeliefLiteral>& conds, unsigned index )
{
	NFF::Mutexes& m = NFF::Mutexes::instance();
	PDDL::Fluent_Set already_in_Gx( sm_task.fluent_count()+1 );
	head.assign( conds.begin(), conds.end() );
	for ( unsigned k = 0; k < conds.size(); k++ )
	{
		unsigned gk = conds[k].first;
		Atom_Vec& mutexes = m.mutex_vec(gk);
		for ( unsigned i = 0; i < mutexes.size(); i++ )
		{
			unsigned mi = mutexes[i];
			unsigned neg_mi = sm_task.not_equivalent(mi);
			if ( neg_mi != 0 && !already_in_Gx.isset(neg_mi) )
			{
				head.push_back( BeliefLiteral( neg_mi, index ) );
				already_in_Gx.set(neg_mi);
			}
		} 
	}
}

  
void Dijkstra::make_states_rules()
{
	std::vector<BeliefLiteral> conditions;
	std::vector<PDDL::Operator*>&        operators = sm_task.useful_ops();

	// Make goal rule
	Dijkstra::Rule r;
	std::vector< BeliefLiteral > G;
	for ( unsigned k = 0; k < sm_task.goal_state().size(); k++ )
		G.push_back( BeliefLiteral( sm_task.goal_state()[k], m_all_states_index ) );

	NFF_Options& opt = NFF_Options::instance();
	if ( opt.use_invariant_xors() && opt.hX_evaluation() == OPT_HX_ADDING )
	{
		make_head_w_mutexes( r.head, G, m_all_states_index );
#ifdef DEBUG
		std::cout << "Extended Goal Condition:" << std::endl;
		for ( unsigned k = 0; k < r.head.size(); k++ )
		{
			sm_task.print_fluent( r.head[k].first, std::cout );
			std::cout << " ";
			if ( k % 5 == 0 ) std::cout << std::endl;
		}
#endif
	}
	else
		r.head.assign( G.begin(), G.end() );
	r.op = sm_task.end();	
	r.effect =  m_goal;
	r.set_cost(0);
	r.set_penalty(0);
	m_states_rules.push_back(r);
	
	// NB: remember: operator 0,1, and 2 are reserved
	for ( unsigned i = 3; i < operators.size(); i++  )
	{
		PDDL::Operator* curr_op = operators[i];

		// MRJ: operator i has no conditional effects
		if (curr_op->prec_vec().size() == 0)
		{
			/** Makes the Rules (<Pre(a), T>, <eff_i>, cost = 1) **/
			/* ...because Pre(a) has to hold in all the models */
	
		   	// Makes the conditions vector, common for all rules with this action
			for ( unsigned j = 0; j < curr_op->hard_prec_vec().size(); j++ )
			{
				BeliefLiteral ci ( curr_op->hard_prec_vec()[j] , m_all_states_index );
				conditions.push_back(ci);
			}
	
		   	// For each add effect, it makes a rule
			for ( unsigned j = 0; j < curr_op->add_vec().size(); j++ )
			{
				Rule r;
				BeliefLiteral pi ( curr_op->add_vec()[j] , m_all_states_index );
				r.op = i;
				/*
				if ( opt.use_invariant_xors() )
					make_head_w_mutexes( r.head, conditions, m_all_states_index );
				else
				*/
				r.head = conditions;
	
				r.effect =  pi;
				if ( i == operators.size() - 1 ) // MRJ: the End action
					r.set_cost(0);
				else
					r.set_cost(1);
				r.set_penalty(0);

				m_states_rules.push_back(r);
			}
		} 
		else 
		{ // MRJ: otherwise

			/*** Makes the Cond-Rules (<Pre(a), T> U <Cond, s>, <eff_i, s>, cost = K) ***/
	 
			// Builds the conditions vector
			for ( unsigned j = 0; j < curr_op->hard_prec_vec().size(); j++ )
			{
				//std::pair<int,int>
				BeliefLiteral ci ( curr_op->hard_prec_vec()[j] , m_all_states_index );
				conditions.push_back(ci);
			}
	   
	   
			for ( unsigned state_idx = 0; state_idx < m_all_states_index; state_idx++ )
			{
				std::vector<BeliefLiteral> conditions2 = conditions;
				for ( unsigned j = 0; j < curr_op->prec_vec().size(); j++ )
				{
					if ( curr_op->hard_preconds().isset(  curr_op->prec_vec()[j] ) )
						continue;
					BeliefLiteral ci ( curr_op->prec_vec()[j] , state_idx );
					conditions2.push_back(ci);
				}
	       
				for ( unsigned k = 0; k < curr_op->add_vec().size(); k++ )
				{
					Rule r;
					BeliefLiteral pi ( curr_op->add_vec()[k] , state_idx );
		  			r.op = i; 
					r.head = conditions2;

					/*
					if ( opt.use_invariant_xors() )
						make_head_w_mutexes( r.head, conditions2, state_idx );
					else
					*/
					r.head = conditions2;
					r.effect =  pi;
					r.set_cost ( 1 );
					r.set_penalty( opt.cond_eff_penalty() );
					m_states_rules.push_back(r);
				}
				conditions2.clear();
			}
			for ( unsigned j = 0; j < curr_op->prec_vec().size(); j++ )
			{
				if ( curr_op->hard_preconds().isset(  curr_op->prec_vec()[j] ) )
						continue;

				BeliefLiteral ci ( curr_op->prec_vec()[j] , m_all_states_index );
				conditions.push_back(ci);
			}
	       
			for ( unsigned k = 0; k < curr_op->add_vec().size(); k++ )
			{
				Rule r;
				BeliefLiteral pi ( curr_op->add_vec()[k] , m_all_states_index );
				r.op = i;	
				r.head = conditions;
				/*
				if ( opt.use_invariant_xors() )
					make_head_w_mutexes( r.head, conditions, m_all_states_index );
				else
				*/
				r.head = conditions;
				
				r.effect =  pi;
				r.set_cost ( 1 );
				r.set_penalty(0);
				m_states_rules.push_back(r);
			}
		
		}
		conditions.clear();

	} // end for (operators)
}

void Dijkstra::print_h_table_file()
{ 

	print_h_table_file("heuristic.debug"); 
}

void Dijkstra::print_h_table_file( std::string fname )
{ 

	std::ofstream out( fname.c_str() );

	print_h_table( out );

	out.close();
}


void Dijkstra::print_h_table()
{
        print_h_table(std::cout);
}


void Dijkstra::print_h_table( std::ostream& out)
{


	for (unsigned i = 1; i < m_heuristic_table.size(); ++i) 
	{
		for (unsigned j = 0; j < m_heuristic_table[i].size(); ++j)
		{
			out << "<";
			sm_task.print_fluent(i, out);
			out << ", " << j << ">: " ;
			if ( h_value(i,j)==infty() )
				out << "oo";
			else
				out << h_value(i,j);
			out << std::endl;
		}
	}

    }

void Dijkstra::processed( Dijkstra::BeliefLiteral l )
{
	m_processed[l.first][l.second] = true;
}

void Dijkstra::not_processed( Dijkstra::BeliefLiteral l )
{
      m_processed[l.first][l.second] = false;
}

void Dijkstra::not_processed( int i, int j )
{
      m_processed[i][j] = false;
}

bool Dijkstra::is_processed( Dijkstra::BeliefLiteral l )
{
      return m_processed[l.first][l.second];
}

void  Dijkstra::initialize_queue_and_heuristics( Belief& b, 
	std::vector< std::vector<int> >& models,
	queue& p_queue )
{
	BeliefLiteral lit;
	std::vector<int>::iterator it;

	set_h_value( m_goal, infty() );
	not_processed( m_goal );	
	for ( unsigned i = 1; i < sm_task.fluents().size(); i++ )
	{
		not_processed( i, m_all_states_index );
		set_h_value( i, m_all_states_index, infty() );
		for ( unsigned j = 0; j < m_all_states_index; j++ )
		{
			not_processed( i, j );
			set_h_value( i, j, infty() );
		}
	}
	while ( ! p_queue.empty() )
		p_queue.pop();	 

	for ( unsigned k = 0; k < b.known_vec().size(); k++ )
	{
		lit = BeliefLiteral( b.known_vec()[k], m_all_states_index );
		set_h_value( lit, 0 );
		p_queue.push( lit );
		processed(lit);
		for ( unsigned j = 0; j < m_all_states_index; j++ ) 
		{
			lit = BeliefLiteral( b.known_vec()[k], j );
			set_h_value( lit, 0 );
			p_queue.push( lit );
			processed(lit);	
		}
	}

	for ( unsigned k = 0; k < models.size(); k++ ) 
	{
		for ( unsigned f = 0; f < models[k].size(); f++ )
		{
			if ( models[k][f] < 0 ) continue;
			lit = BeliefLiteral( f, k );
			if ( is_processed(lit) ) continue;
			set_h_value( lit, 0 );
			p_queue.push( lit );
			processed(lit);
		}
	}


	// checks goal condition <g,T>; 
	//  NB: maybe goal has value +- 1
	if ( h_value( goal_state() ) != 0 )
		set_h_value( goal_state(), infty() );

	for ( unsigned i = 0; i < get_all_rules().size(); i++ )
		get_all_rules()[i].dirty = true;  
 
	//print_h_table_file( "heuristic-after-init.table" ); 
}

void Dijkstra::print_rule( Rule &r )
{
                        std::cout << "\t Rule: " << std::endl;
                        if ( r.op == -1 ) 
		{
			std::cout << "MERGE" << std::endl;
		}
                else if ( r.op == -2 ) 
		{
                        std::cout << "STATIC OR" << std::endl;
		}
                else if ( r.op == -3 ) 
		{
                        std::cout << "ACTION COMPILATION" << std::endl;
		}
		else
		{
			sm_task.print_operator( r.op, std::cout );
			std::cout << std::endl;
		}
		std::cout << "     Conditions: " ;
		for ( unsigned j = 0; j < r.head.size(); j++)
		{
			std::cout << "<"; 
			sm_task.print_fluent( r.head[j].first, std::cout );
			std::cout << ", " << r.head[j].second << "> " ;
		}
		std::cout << " Effect: " ;
		std::cout << "<";
		sm_task.print_fluent( r.effect.first, std::cout );
		std::cout << ", " << r.effect.second << "> " << std::endl;



}


void Dijkstra::print_rules ( std::string fname, std::vector<Rule>& rules_vec )
{
	std::ofstream out( fname.c_str() );

	for ( unsigned i = 0; i < rules_vec.size(); i++ )
	{
		Rule &r = rules_vec.at(i);
		out << "Rule: " << i << "  from ";
		if ( r.op == -1 ) 
		{
			out << "MERGE" << std::endl;
		}
                else if ( r.op == -2 ) 
		{
                        out << "STATIC OR" << std::endl;
		}
                else if ( r.op == -3 ) 
		{
                        out << "ACTION COMPILATION" << std::endl;
		}
		else
		{
			sm_task.print_operator( r.op, out );
			out << std::endl;
		}
		out << "     Conditions: " ;
		for ( unsigned j = 0; j < r.head.size(); j++)
		{
			out << "<"; 
			sm_task.print_fluent( r.head[j].first, out );
			out << ", " << r.head[j].second << "> " ;
		}
		out << " Effect: " ;
		out << "<";
		sm_task.print_fluent( r.effect.first, out );
		out << ", " << r.effect.second << "> " << std::endl;
	}
}

void	Dijkstra::extract_relaxed_plan_from_hmax( std::vector< std::vector< unsigned > >& eff_layers,
					PDDL::Fluent_Set& rp_precs,
					Operator_Vec&	relaxed_plan_actions,
					Operator_Vec&	helpful_actions )
{
	for ( int l = eff_layers.size()-1; l > 0; l-- )
	{
		for ( unsigned k = 0; k < eff_layers[l].size(); k++ )
		{
			Rule& r = get_all_rules()[eff_layers[l][k]];
			for ( unsigned i = 0; i < r.head.size(); i++ )
			{
				if ( m_seen_lits[belief_lit_index(r.head[i])] )
					continue;
				m_seen_lits[ belief_lit_index(r.head[i]) ] = true;
				if ( h_value(r.head[i]) == 0 ) continue;
				unsigned supporter = get_supporter( r.head[i] );
				Rule& r2 = get_all_rules()[ supporter ];
				eff_layers[ r2.value - r2.get_cost() ].push_back( supporter );
			}
		}
	}
	for ( int l = 0; l < eff_layers.size(); l++ )
	{
#ifdef DEBUG
		if ( !eff_layers[l].empty() )
			std::cout << "At layer " << l << std::endl;	
#endif
		for (unsigned k = 0; k < m_seen_effs.size(); k++ )
			m_seen_effs[k] = false;

		for ( unsigned k = 0; k < eff_layers[l].size(); k++ )
		{
			Rule& r = get_all_rules()[ eff_layers[l][k] ];
			if ( r.op <= 1 )
			{
#ifdef DEBUG
				if ( r.op == 1 )
				{
					std::cout << "(END)";

				}
				else if ( r.op == -1 )
					std::cout << "(MERGE)";
				else if ( r.op == -2 )
					std::cout << "(STATIC-OR)";
				else
					sm_task.print_operator( r.op, std::cout );
				std::cout << " h(r) = {";
				for ( unsigned k = 0; k < r.head.size(); k++ ) {
					std::cout << "(<";
					sm_task.print_fluent( r.head[k].first, std::cout );
					std::cout << "," << r.head[k].second;
					std::cout << ">, " << h_value( r.head[k] ) << ")";
					if ( k < r.head.size()-1 )
						std::cout << ",";
				}
				std::cout << " }" ;
				std::cout << " -> <";
				sm_task.print_fluent( r.effect.first, std::cout );
				std::cout << " , " << r.effect.second << ">" << std::endl;
#endif
				continue;
			}
#ifdef DEBUG
			sm_task.print_operator( r.op, std::cout );
			std::cout << " h(r) = {";
			for ( unsigned k = 0; k < r.head.size(); k++ ) {
				std::cout << "(<";
				sm_task.print_fluent( r.head[k].first, std::cout );
				std::cout << "," << r.head[k].second;
				std::cout << ">, " << h_value( r.head[k] ) << ")";
				if ( k < r.head.size()-1 )
					std::cout << ",";
			}
			std::cout << " }" ;
			std::cout << " -> <";
			sm_task.print_fluent( r.effect.first, std::cout );
			std::cout << " , " << r.effect.second << ">" << std::endl;
#endif				
			if ( m_seen_effs[r.op]  ) continue;
			m_seen_effs[r.op] = true;
			PDDL::Operator* op_ptr = sm_task.useful_ops()[r.op];
			//std::cout << " Adding ";
			//sm_task.print_operator( op_ptr, std::cout );
			//std::cout << " to relaxed plan" << std::endl;
			relaxed_plan_actions.push_back( op_ptr->prototype() );
			
			if ( l == 0 )
			{
				if ( std::find( helpful_actions.begin(), helpful_actions.end(),  op_ptr->prototype() )
					== helpful_actions.end() )
					helpful_actions.push_back( op_ptr->prototype() );
			}
			else
			{
				Atom_Vec&	precs = op_ptr->prec_vec();
				for ( unsigned k = 0; k < precs.size(); k++ )
					rp_precs.set( precs[k] );
			}
		}
	}

}

void	Dijkstra::extract_relaxed_plan_from_hadd( std::vector< std::vector< unsigned > >& eff_layers,
					PDDL::Fluent_Set& rp_precs,
					Operator_Vec&	relaxed_plan_actions,
					Operator_Vec&	helpful_actions )
{
	for ( int l = eff_layers.size()-1; l > 0; l-- )
	{
		for ( unsigned k = 0; k < eff_layers[l].size(); k++ )
		{
			Rule& r = get_all_rules()[eff_layers[l][k]];
			for ( unsigned i = 0; i < r.head.size(); i++ )
			{
				if ( m_seen_lits[belief_lit_index(r.head[i])] )
					continue;
				m_seen_lits[ belief_lit_index(r.head[i]) ] = true;
				if ( h_value(r.head[i]) == 0 ) continue;
				unsigned supporter = get_supporter( r.head[i] );
				Rule& r2 = get_all_rules()[ supporter ];
				eff_layers[ r2.value - r2.get_cost() ].push_back( supporter );
			}
		}
	}
	
	for ( int l = 0; l < (int)eff_layers.size(); l++ )
	{
#ifdef DEBUG
		if ( !eff_layers[l].empty() )
			std::cout << "At layer " << l << std::endl;	
#endif
		for ( unsigned k = 0; k < eff_layers[l].size(); k++ )
		{
			Rule& r = get_all_rules()[ eff_layers[l][k] ];
			if ( r.op <= 1 )
			{
#ifdef DEBUG
				if ( r.op == 1 )
				{
					std::cout << "(END)";

				}
				else if ( r.op == -1 )
					std::cout << "(MERGE)";
				else if ( r.op == -2 )
					std::cout << "(STATIC-OR)";
				else
					sm_task.print_operator( r.op, std::cout );
				std::cout << " h(r) = {";
				for ( unsigned k = 0; k < r.head.size(); k++ ) {
					std::cout << "(<";
					sm_task.print_fluent( r.head[k].first, std::cout );
					std::cout << "," << r.head[k].second;
					std::cout << ">, " << h_value( r.head[k] ) << ")";
					if ( k < r.head.size()-1 )
						std::cout << ",";
				}
				std::cout << " }" ;
				std::cout << " -> <";
				sm_task.print_fluent( r.effect.first, std::cout );
				std::cout << " , " << r.effect.second << ">" << std::endl;
#endif
				continue;
			}
#ifdef DEBUG
			sm_task.print_operator( r.op, std::cout );
			std::cout << " h(r) = {";
			for ( unsigned k = 0; k < r.head.size(); k++ ) {
				std::cout << "(<";
				sm_task.print_fluent( r.head[k].first, std::cout );
				std::cout << "," << r.head[k].second;
				std::cout << ">, " << h_value( r.head[k] ) << ")";
				if ( k < r.head.size()-1 )
					std::cout << ",";
			}
			std::cout << " }" ;
			std::cout << " -> <";
			sm_task.print_fluent( r.effect.first, std::cout );
			std::cout << " , " << r.effect.second << ">" << std::endl;
#endif				
			if ( m_seen_effs[r.op]  ) continue;
			m_seen_effs[r.op] = true;
			PDDL::Operator* op_ptr = sm_task.useful_ops()[r.op];
			//std::cout << " Adding ";
			//sm_task.print_operator( op_ptr, std::cout );
			//std::cout << " to relaxed plan" << std::endl;
			relaxed_plan_actions.push_back( op_ptr->prototype() );
			
			if ( l == 0 )
			{
				if ( std::find( helpful_actions.begin(), helpful_actions.end(),  op_ptr->prototype() )
					== helpful_actions.end() )
					helpful_actions.push_back( op_ptr->prototype() );
			}
			else
			{
				Atom_Vec&	precs = op_ptr->prec_vec();
				for ( unsigned k = 0; k < precs.size(); k++ )
					rp_precs.set( precs[k] );
			}
		}
	}

}

bool	Dijkstra::condition_holds_in_sample( Atom_Vec& cond, std::vector<std::vector<int> >& models )
{
	for ( unsigned j = 0; j < models.size(); j++ )
	{
		bool holds = true;
		for ( unsigned k = 0; k < cond.size(); k++ )
			if ( models[j][ cond[k] ] < 0 )
			{
				holds = false;
				break;
			}
		if ( holds ) return true;	
	}
	return false;	
}



void	Dijkstra::complete_helpful_actions( SearchNode& n, PDDL::Fluent_Set& rp_precs, Operator_Vec& helpful_actions, std::vector<std::vector<int> >& samples )
{
	Belief&	b = *(n.b);

	for ( unsigned k = 0; k < sm_task.prototype_ops().size(); k++ )
	{
		PDDL::Operator* action = sm_task.prototype_ops()[k];
		if ( !b.can_apply( action ) ) continue;
		if ( std::find( helpful_actions.begin(), helpful_actions.end(), k )
				!= helpful_actions.end() )
			continue;
#ifdef DEBUG
		std::cout << "Checking whether action ";
		sm_task.print_operator( action, std::cout );
		std::cout << " is helpful" << std::endl;
#endif
		std::vector< PDDL::Operator* >& effects = action->same_op_vec();
		std::vector< PDDL::Operator* >  relevant_effs;
		for ( unsigned i = 0; i < effects.size(); i++ )
		{
			bool relevant = false;
			Atom_Vec& adds = effects[i]->add_vec();
			for ( unsigned j = 0; j < adds.size(); j++ )
				if ( rp_precs.isset(adds[j]) )
				{
					relevant = true;
					break;
				}
			if ( relevant )
				relevant_effs.push_back( effects[i] );
		}
		if ( relevant_effs.empty() ) 
		{
#ifdef DEBUG
			std::cout << "Action isn't adding anything relevant for the relaxed plan" << std::endl;
#endif
			continue;
		}
		// Now check if any of the relevant effects conditions
		// hold in some state in S(b)
		bool  is_helpful = false;
		for ( unsigned i = 0; i < relevant_effs.size(); i++ )
		{
			if ( condition_holds_in_sample( relevant_effs[i]->prec_vec(), samples ) )
			{
				is_helpful = true;
				break;
			}
		}
		
		if ( is_helpful )
			helpful_actions.push_back( k );
		else
		{
#ifdef DEBUG	
			std::cout << "relevant effects condition didn't hold in any sample" << std::endl;
#endif
		}
	}
}

void	Dijkstra::compute_cheapest_unachieved_goal( SearchNode& n )
{
	Atom_Vec& G = sm_task.goal_state();
	m_cheapest_goal_estimate = infty();
	bool all_achieved = true;
	for ( unsigned k = 0; k < G.size(); k++ )
	{
		if ( n.b->known_set().isset( G[k] ) ) continue;
		all_achieved = false;
		BeliefLiteral lgi = BeliefLiteral( G[k], m_all_states_index+1 );	
		unsigned hgi = h_value(lgi);
		if ( hgi < m_cheapest_goal_estimate )
			m_cheapest_goal_estimate = hgi;
	}
	if ( all_achieved )
		m_cheapest_goal_estimate = 0;
}



void	Dijkstra::pre_compute( SearchNode& n, std::vector< std::vector<int> >& models )
{
	NFF_Options& opt = NFF_Options::instance();
	compute( *(n.b), models );
	compute_hS_rp(n, models);
	m_hX_rp_value = 0;
	if ( opt.use_invariant_xors() )
		xor_h_value(n, models);
        //AA: we removed that feature
	if ( 0 && opt.use_invariant_diameter() )
                diameter_h_value(n);
	//compute_cheapest_unachieved_goal( n );
}


/* Updates the heuristic_table for all BeliefLiterals
 * @returns the number of updated literals.    **/
int  Dijkstra::compute( Belief& b, std::vector< std::vector< int > >& models )
{
	NFF_Options&	opt = NFF_Options::instance();
     	BeliefLiteral current_lit, q, effect_lit;
	int n = 0;
	Literals_Comparer lc( m_heuristic_table ); //heuristics_table() );
	// Priority Queue
	queue  p_queue(lc);
	bool has_changed;

	initialize_queue_and_heuristics( b, models, p_queue);
#ifdef DEBUG
        print_h_table_file("heuristic.init.table");	
#endif
	while ( !p_queue.empty() )
	{

		current_lit = p_queue.top();
		p_queue.pop();

		if ( current_lit == goal_state() )
	                break;

		processed( current_lit );

		// for each rule r where current_lit is in Conditions
		std::vector<unsigned>&	relevant_rules = get_head_rules(current_lit);
		for ( unsigned i = 0; i < relevant_rules.size(); i++ )
		{
                        if (DEBUG_DIJKSTRA) has_changed=false;

			Rule& r = m_all_rules[relevant_rules[i]];
			BeliefLiteral effect_lit = r.effect;
			if ( is_processed(effect_lit) ) continue;
			if ( opt.heuristic_function() == OPT_ADDITIVE_H )
				r.compute_value_additive(*this);
			else
				r.compute_value_max( *this );
			if ( r.value == infty() ) continue;

			if ( h_value( effect_lit ) > r.value  )
			{
                                if (DEBUG_DIJKSTRA) has_changed = true;

				set_h_value ( effect_lit, r.value );
				set_supporter( effect_lit, relevant_rules[i] );
				++n;
				p_queue.push( effect_lit );
				// MRJ: mark all rules depending on this literal to
				// have its value recomputed
				std::vector<unsigned>& affected_rules = get_head_rules(effect_lit);
				for ( unsigned j = 0; j < affected_rules.size(); j++ )
					m_all_rules[affected_rules[j]].dirty = true;
			}  
                        if (DEBUG_DIJKSTRA)
                                if (has_changed) {
                                        print_rule(r);
                                        print_h_table();
                                }
                }
	} // end while
#ifdef DEBUG
        print_h_table_file("heuristic.table");
	std::cout << "Dijkstra: h(g) = " << h_value(goal_state()) << std::endl;
#endif
	return n;
}


void	Dijkstra::initialize_effect_layers( 	std::vector< std::vector< unsigned > >& eff_layers,
						PDDL::Fluent_Set& rp_precs,
						PDDL::Fluent_Set& mask )
{
	Rule& r = get_all_rules()[ get_supporter( m_goal ) ];
	for ( unsigned i = 0; i < r.head.size(); i++ )
	{
		BeliefLiteral li = r.head[i];
		if ( !mask.isset(li.first)) continue;
		rp_precs.set( li.first );
		if ( m_seen_lits[belief_lit_index(li)] )
			continue;
		m_seen_lits[ belief_lit_index(li) ] = true;
		if ( h_value(li) == 0 ) continue;
		unsigned supporter = get_supporter( li );
		Rule& r2 = get_all_rules()[ supporter ];
		eff_layers[ r2.value - r2.get_cost() ].push_back( supporter );
	}
}
							
void	Dijkstra::compute_hS_rp( SearchNode& n, std::vector<std::vector<int> >& m )
{
	NFF_Options& opt = NFF_Options::instance();	
	m_hS_relaxed_plan.clear();
	m_hS_helpful.clear();
	for (unsigned k = 0; k < m_seen_effs.size(); k++ )
		m_seen_effs[k] = false;
	for (unsigned k = 0; k < m_seen_lits.size(); k++ )
		m_seen_lits[k] = false; 
	if ( h_value(m_goal) == std::numeric_limits<unsigned>::max() )
	{
		m_hS_rp_value = std::numeric_limits<unsigned>::max();
		return;
	}
	PDDL::Fluent_Set	rp_precs( sm_task.fluent_count()+1 );
	std::vector< std::vector< unsigned > > eff_layers;
	eff_layers.resize( h_value(m_goal) + 1 );
	//eff_layers[h_value(m_goal)].push_back( get_supporter( m_goal ) );

	initialize_effect_layers( eff_layers, rp_precs, m_goal_mask );

	if ( opt.heuristic_function() == OPT_ADDITIVE_H )
		extract_relaxed_plan_from_hadd( eff_layers, rp_precs, m_hS_relaxed_plan, m_hS_helpful );
	else
		extract_relaxed_plan_from_hmax( eff_layers, rp_precs, m_hS_relaxed_plan, m_hS_helpful );

	m_hS_rp_value = 0;

#ifdef DEBUG
	std::cout << "Relaxed plan: " ;
#endif

	for ( unsigned k = 0; k < m_hS_relaxed_plan.size(); k++ )
	{
		m_hS_rp_value += sm_task.prototype_ops()[m_hS_relaxed_plan[k]]->metric_cost();

#ifdef DEBUG
		sm_task.print_operator( sm_task.prototype_ops()[m_hS_relaxed_plan[k]], std::cout );
		if ( k < m_hS_relaxed_plan.size() - 1 )
			std::cout << " ";
#endif
	}

	complete_helpful_actions( n, rp_precs, m_hS_helpful, m);

#ifdef DEBUG
	std::cout << std::endl;
	std::cout << "Helpful operators:" << std::endl;
	for ( unsigned k = 0; k < m_hS_helpful.size(); k++ )
	{
		sm_task.print_operator( sm_task.prototype_ops()[m_hS_helpful[k]], std::cout );
		std::cout << " ";
	}
	std::cout << std::endl;
	std::cout << "Relaxed Plan Heuristic: " << m_hS_rp_value << std::endl;
#endif

}

void Dijkstra::make_lit_to_rule_table()
{
	m_lit_to_rule_head_table.resize( sm_task.fluents().size()*(m_all_states_index+1) );
	
	// MRJ: Now we associate rules with the literals appearing on the
	// rule head
	for ( unsigned i = 0; i < get_all_rules().size(); i++ ) 
	{
		Rule& r = get_all_rules()[i];
		for ( unsigned j = 0; j < r.head.size(); j++ )
			m_lit_to_rule_head_table[belief_lit_index(r.head[j])].push_back(i);
		if ( r.head.size() == 0 ) // Relevant to everybody!
		{
			for ( unsigned p = 1; p < unsigned(sm_task.fluent_count()-1); p++ )
			{
				for ( unsigned j = 0; j < m_all_states_index; j++ )
				{
					BeliefLiteral l(p,j);
					m_lit_to_rule_head_table[belief_lit_index(l)].push_back(i);
				}
				BeliefLiteral l(p,m_all_states_index);
				m_lit_to_rule_head_table[belief_lit_index(l)].push_back(i);
			}
		}
	}	
}

// Returns h(p|b,q)
unsigned Dijkstra::h_diameter( unsigned p, unsigned q)
{
        unsigned h = 0;
        unsigned count = 0;
        for ( unsigned j = 0; j < m_all_states_index; j++ )
        {
                // if q true in s, then h = MAX h(p,s)
                if  ( h_value(q,j) == 0 )
                {
                        h = std::max(h, h_value(p ,j) );
                        count++;
                } 
        }
        if ( count == 0)
                h = infty();
        return h;
}



unsigned   Dijkstra::diameter_h_value( SearchNode& n )
{
	NFF_Options& opt = NFF_Options::instance();
        PDDL::Task&	sm_task = PDDL::Task::instance();
	if ( !opt.use_invariant_diameter() ) return 0;
        std::vector<unsigned> reduced_set; // here we store all the p not known to be false
//	PDDL::Fluent_Set	pending_set( sm_task.fluent_count()+1 );
        unsigned hp;
        unsigned hq;
        unsigned hd_value =0;

#ifdef DEBUG
	std::cout << "h_D(n) heuristic" << std::endl;
        print_h_table_file("heuristic.table");
#endif
  
        Mutexes& mu = Mutexes::instance();
        for ( unsigned g = 0; g < sm_task.goal_state().size(); g++ )
        {
                unsigned gk = sm_task.goal_state()[g];
                unsigned max_value = 0;
                bool diameter_zero = false;
                reduced_set.clear();
                // Checks if a value is known, i.e. diameter = 0
                for (unsigned k = 0; k < mu.invariants_lits( g ).size(); k++ )
                {
                        unsigned p = mu.invariants_lits( g )[k];
                        
                        if ( h_value(BeliefLiteral( p, m_all_states_index )) == 0 )  
                        {
/*
#ifdef DEBUG
std::cout << "We've found that h_D";
sm_task.print_fluent( p, std::cout );
std::cout << " is 0" << std::endl;
#endif
*/
                                diameter_zero = true;
                                break;
                        }
                        //AA: comment next line when using distance from goal
                        if (p == gk) continue;

                        unsigned neg_p = sm_task.not_equivalent(p); 
                        // Checks if p is not known to be false                
                        if ( neg_p && (h_value(BeliefLiteral( neg_p, m_all_states_index )) == 0) )  
                                continue;
                        reduced_set.push_back( p);
                }
                if (diameter_zero) continue;

// Distance from the goal
                if (0)
                for (unsigned ki = 0; ki < reduced_set.size(); ki++ )
                {
                        unsigned p = reduced_set[ki];
                        hp =  h_diameter(gk, p);
                        max_value = std::max(hp, max_value);
                }

// Max distance between two literals
                if (1)
                for (unsigned ki = 0; ki < reduced_set.size(); ki++ )
                {
                        unsigned p = reduced_set[ki];

                        for (unsigned kj = (ki+1); kj < reduced_set.size(); kj++)
                        {
                                unsigned q = reduced_set[kj];
                                unsigned h_pq;

                                hp =  h_diameter(p, q);
                                hq =  h_diameter(q, p);
                                h_pq = std::min( hp, hq );
                            
                                if (h_pq > max_value)
                                {
                                        max_value = h_pq;
/*                                        // Put all lits into helpful
                                        if  (sm_task.not_equivalent(p))
                                                pending_set.set(sm_task.not_equivalent(p));
                                        if  (sm_task.not_equivalent(q))
                                                pending_set.set(sm_task.not_equivalent(q));
*/
                                }
                        }
                }

                hd_value += max_value;  
        }
#ifdef DEBUG
        std::cout << "Value: " << m_hX_rp_value << std::endl;
        std::cout << "Value: " << hd_value << std::endl;
#endif
                return hd_value;
}


unsigned	Dijkstra::xor_h_value( SearchNode& n, std::vector<std::vector<int> >& mu )
{
	NFF_Options& opt = NFF_Options::instance();
	if ( !opt.use_invariant_xors() ) return 0;

        Mutexes& m = Mutexes::instance();
	m_hX_rp_value = 0;

#ifdef DEBUG
	std::cout << "h_X(n) heuristic" << std::endl;
#endif
	PDDL::Fluent_Set	pending_set( sm_task.fluent_count()+1 );
	unsigned min_kgi_h = infty();
        for (unsigned kx = 0; kx < m.goal_invariants().size(); kx++)
        {
                unsigned kgi = m.goal_invariants()[kx];
                if ( h_value(BeliefLiteral(kgi,m_all_states_index)) > 0 ) // !b.known_set().isset(kgi))
		{
#ifdef DEBUG
			std::cout << "Invariant ";
			sm_task.print_fluent( kgi, std::cout );
			std::cout << " not in KL h(kgi)=" << h_value(BeliefLiteral(kgi,m_all_states_index)) << std::endl;
#endif
			//pending_set.set( kgi );
			min_kgi_h = ( min_kgi_h > h_value(BeliefLiteral(kgi,m_all_states_index))
                                      ?	 h_value(BeliefLiteral(kgi,m_all_states_index)) 
                                      :  min_kgi_h );
			if ( opt.hX_evaluation() == OPT_HX_ADDING )
				m_hX_rp_value = std::add( m_hX_rp_value, h_value(BeliefLiteral(kgi,m_all_states_index)) );
			else
                        	m_hX_rp_value++;	
		}
        }

	PDDL::Fluent_Set covered_one_ofs( sm_task.oneofs().size() + 1 );
	for ( unsigned kx = 0; kx < m.goal_invariants().size(); kx++)
        {
		
                unsigned kgi = m.goal_invariants()[kx];
		unsigned one_of = sm_task.oneofs().size();
		for ( unsigned i = 0; i < sm_task.oneofs().size(); i++ )
		{
			std::vector<unsigned>& oneof = sm_task.oneofs().at(i);
			if ( std::find( oneof.begin(), oneof.end(), kgi ) != oneof.end() )
			{
				one_of = i;
			}
		} 
		if ( covered_one_ofs.isset(one_of) ) continue;
		if ( h_value(BeliefLiteral(kgi,m_all_states_index)) == min_kgi_h )
		{
			pending_set.set( kgi );
			covered_one_ofs.set( one_of );
		}
	}
	
	unsigned min_gi_h = infty();
	for ( unsigned k = 0; k < sm_task.goal_state().size(); k++ )
	{
		unsigned gk = sm_task.goal_state()[k];
		if ( m_ext_goal_mask.isset( gk ) ) continue;
		if ( h_value(BeliefLiteral(gk,m_all_states_index)) > 0 ) 
		{
#ifdef DEBUG
			std::cout << "Goal ";
			sm_task.print_fluent( gk, std::cout );
			std::cout << " not in KL h(gi)=" << h_value(BeliefLiteral(gk,m_all_states_index)) << std::endl;
#endif
			//pending_set.set( gk );
			min_gi_h = ( min_gi_h > h_value(BeliefLiteral(gk,m_all_states_index)) ?
					h_value(BeliefLiteral(gk,m_all_states_index)) : min_gi_h );

			if ( opt.hX_evaluation() == OPT_HX_ADDING )
				m_hX_rp_value = std::add( m_hX_rp_value, h_value(BeliefLiteral(gk,m_all_states_index)) );
			else
				m_hX_rp_value++;
		}
	}

	for ( unsigned k = 0; k < sm_task.goal_state().size(); k++ )
	{
		unsigned gk = sm_task.goal_state()[k];
		if ( h_value(BeliefLiteral(gk,m_all_states_index)) == min_gi_h )
			pending_set.set( gk );
	}


	m_hX_helpful.clear();
	complete_helpful_actions( n, pending_set, m_hX_helpful, mu );
	return m_hX_rp_value;
}
	
} // end NFF
