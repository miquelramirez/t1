/*
    Alexandre Albore, Miguel Ramirez, Hector Geffner
    T1 conformant planner
    Copyright (C) UPF 2010

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
#include "nff_search_node.hxx"
#include <algorithm>
#include "nff.hxx"
#include "hash_table.hxx"
#include "nff_planner_stats.hxx"
#include "global_options.hxx"
#include "nff_options.hxx"
#include "nff_mutexes.hxx"
#include "nff_dijkstra.hxx"
#define DEBUG_SAT 0

namespace NFF
{

PDDL::Task&	SearchNode::sm_task = PDDL::Task::instance();

SearchNode::SearchNode()
        : b(NULL), gn(0), hn(0), hS(0), hX(0), tb(0), father(NULL), op(0), timestep(0), theory_step(NULL),
	num_dead_models(0)
	
{
	m_hS_HA_set = new PDDL::Fluent_Set( sm_task.prototype_ops().size() + 1 );
	m_hX_HA_set = new PDDL::Fluent_Set( sm_task.prototype_ops().size() + 1 );
}

SearchNode::~SearchNode()
{
        if ( b )
		delete b;
	if ( theory_step )
		delete theory_step;
//        if ( models )
//                delete models; 
}

unsigned SearchNode::size()
{
	unsigned numBytes = sizeof(b);
	numBytes += sizeof(gn);
	numBytes += sizeof(hn);
	numBytes += sizeof(hS);
	numBytes += sizeof(hX);
	numBytes += sizeof(tb);
	numBytes += sizeof(queue);
	numBytes += sizeof(fn);
	numBytes += sizeof(father);
	numBytes += sizeof(op);
	numBytes += sizeof(timestep);
//	numBytes += sizeof(models);
	numBytes += sizeof(theory_step);
	numBytes += sizeof(potentially_known);
//	numBytes += sizeof(dead_model);
	numBytes += sizeof(num_dead_models);
	numBytes += sizeof(m_hS_HA);
	numBytes += sizeof(m_hS_HA_set);
	numBytes += sizeof(m_hX_HA);
	numBytes += sizeof(m_hX_HA_set);
	numBytes += sizeof( sm_task );
	numBytes += sizeof( m_hash_key );
	numBytes += potentially_known.size()*sizeof(bool);
//	numBytes += dead_model.size()*sizeof(bool);
	numBytes += b->size();
	if ( theory_step != NULL )
		numBytes += theory_step->size();
	numBytes += m_hS_HA.size()*sizeof(unsigned);
	numBytes += m_hX_HA.size()*sizeof(unsigned);
	numBytes += m_hS_HA_set->bits().npacks()*sizeof(unsigned);
	numBytes += m_hX_HA_set->bits().npacks()*sizeof(unsigned);
// 	for ( unsigned k = 0; k < models.size(); k++ ) 
// 	{
// 		numBytes += sizeof(models[k]);
// 		numBytes += models[k].size()*sizeof(unsigned);	
// 	}
	return numBytes;
}

SearchNode*   SearchNode::root()
{
	SearchNode*	n = new SearchNode;
	n->b = Belief::make_initial_belief();
	n->theory_step = new Theory_Fragment();
	n->potentially_known.resize( sm_task.fluent_count() );
        Planner_Stats&	stats = Planner_Stats::instance();
        if ( !stats.is_width_1() )
                n->make_root_clauses();

#ifdef DEBUG
	MiniSAT solver(n);
        if ( !stats.is_width_1() )
                assert( solver.solve() );
#endif

	n->gn = 0;
	n->father = NULL;
	n->op = 0;
	//n->hS = n->hX = n->hn = 0;
	n->timestep = 0;
	return n;
}

/** Calculates the heuristic value *
 * returns INFTY if the lit is not reachable in all the models */
unsigned SearchNode::evaluate( Dijkstra* heuristic, std::vector< std::vector<int> >&models )
{
	NFF_Options& opt = NFF_Options::instance();
	Planner_Stats&	stats = Planner_Stats::instance();
	stats.notify_heuristic_computation();
	
	heuristic->pre_compute(*this, models);
	
        //AA: For BFS, we can only compute one of the 2 heuristics
        if ( opt.search_strategy() != "bfs" || opt.ehc_heuristic() != OPT_EHC_USES_HX )
        {
                m_hS_HA.assign( heuristic->hS_helpful().begin(), heuristic->hS_helpful().end() );
                for ( unsigned k = 0; k < m_hS_HA.size(); k++ )
                        m_hS_HA_set->set( m_hS_HA[k] );
                hS =  heuristic->hS_value();
        }
        if ( opt.search_strategy() != "bfs" || opt.ehc_heuristic() != OPT_EHC_USES_HS )
        {
                m_hX_HA.assign( heuristic->hX_helpful().begin(), heuristic->hX_helpful().end() );
                for ( unsigned k = 0; k < m_hX_HA.size(); k++ )
                        m_hX_HA_set->set( m_hX_HA[k] );
                hX = heuristic->hX_value();
        }

	tb = heuristic->h_value(heuristic->goal_state());//tie_breaking();
	
        //AA: once used for diameter heuristic
        //        heuristics = heuristic;

	return heuristic->hS_value();
}


/** Calculates the heuristic value *
 * returns INFTY if the lit is not reachable in all the models */
// unsigned SearchNode::evaluate( Dijkstra* heuristic )
// {
// 	Planner_Stats&	stats = Planner_Stats::instance();
// 	stats.notify_heuristic_computation();
	
// 	heuristic->pre_compute(*this);
	
// 	m_hS_HA.assign( heuristic->hS_helpful().begin(), heuristic->hS_helpful().end() );
// 	for ( unsigned k = 0; k < m_hS_HA.size(); k++ )
// 		m_hS_HA_set->set( m_hS_HA[k] );
// 	hS =  heuristic->hS_value();

// 	m_hX_HA.assign( heuristic->hX_helpful().begin(), heuristic->hX_helpful().end() );
// 	for ( unsigned k = 0; k < m_hX_HA.size(); k++ )
// 		m_hX_HA_set->set( m_hX_HA[k] );
// 	hX = heuristic->hX_value();

// 	tb = heuristic->h_value(heuristic->goal_state());//tie_breaking();
	
//         //AA: once used for diameter heuristic
//         //        heuristics = heuristic;

// 	return heuristic->hS_value();
// }

// bool	SearchNode::condition_holds_in_sample( Atom_Vec& cond )
// {
// 	for ( unsigned j = 0; j < models.size(); j++ )
// 	{
// 		bool holds = true;
// 		for ( unsigned k = 0; k < cond.size(); k++ )
// 			if ( models[j][ cond[k] ] < 0 )
// 			{
// 				holds = false;
// 				break;
// 			}
// 		if ( holds ) return true;	
// 	}
// 	return false;	
// }

// void	SearchNode::update_models( std::vector< PDDL::Operator* >& op_effs )
// {
// //        assert( father->models.size() > 0 );
// 	models = father->models;


// #ifdef DEBUG
// 	std::vector<bool> changed(models.size());
// #endif


// 	for ( unsigned i = 0; i < op_effs.size(); i++ )
// 	{
// //                std::cout << "Applying operator to models: ";
// //                sm_task.print_operator_full_notab( op_effs[i], std::cout );
// 		for ( unsigned j = 0; j < father->models.size(); j++ )
// 		{
// 			bool applicable = true;
// 			for ( unsigned k = 0; k < op_effs[i]->prec_vec().size(); k++ )
// 				if ( father->models[j][ op_effs[i]->prec_vec()[k] ] < 0 )
// 				{
// 					applicable = false;
// 					break;
// 				}	
// 			if ( !applicable ) continue;

// #ifdef DEBUG
// 			changed[j] = true;
// //			sm_task.print_operator_full_notab( op_effs[i], std::cout );
// //			std::cout << std::endl;
// #endif	

// 			for ( unsigned k = 0; k < op_effs[i]->add_vec().size(); k++ )
// 				models[j][ op_effs[i]->add_vec()[k] ] =  op_effs[i]->add_vec()[k]  ;
// 			for ( unsigned k = 0; k < op_effs[i]->del_vec().size(); k++ )
// 				models[j][ op_effs[i]->del_vec()[k] ] = -op_effs[i]->del_vec()[k];
// 		}

// 	}

// #ifdef DEBUG
// 	std::cout << "MODELS changed by the action:" << std::endl;
// 	for ( unsigned j = 0; j < models.size(); j++ )
// 		if ( changed[j] )
// 		{
// 			std::cout << "s_" << j << ": ";
// 			for ( unsigned l = 1; l < models[j].size(); l++ ) 
// 			{
// 				std::cout << models[j][l] << " " << (models[j][l] > 0 ? " " : " -");
// 				sm_task.print_fluent( models[j][l] > 0 ? models[j][l] : (unsigned)abs(models[j][l]) );
// 				std::cout << ", ";
// 			}
// 			std::cout << std::endl;
// 		}
// #endif

// }


void SearchNode::update_single_model ( std::vector< PDDL::Operator* >& op_effs, 
                                       std::vector< int >& lmodel )
{
        std::vector<int> father_model = lmodel;

	for ( unsigned i = 0; i < op_effs.size(); i++ )
	{
//                std::cout << "Applying operator to models: ";
//                sm_task.print_operator_full_notab( op_effs[i], std::cout );
			bool applicable = true;
			for ( unsigned k = 0; k < op_effs[i]->prec_vec().size(); k++ )
				if ( father_model[ op_effs[i]->prec_vec()[k] ] < 0 )
				{
					applicable = false;
					break;
				}	
			if ( !applicable ) continue;

			for ( unsigned k = 0; k < op_effs[i]->add_vec().size(); k++ )
				lmodel[ op_effs[i]->add_vec()[k] ] =  op_effs[i]->add_vec()[k]  ;
			for ( unsigned k = 0; k < op_effs[i]->del_vec().size(); k++ )
				lmodel[ op_effs[i]->del_vec()[k] ] = -op_effs[i]->del_vec()[k];
	}
}


void	SearchNode::update_models( std::vector< PDDL::Operator* >& op_effs, 
                                   std::vector< std::vector< int > >& lmodels)
{
/*
#ifdef DEBUG
	std::vector<bool> changed(lmodels.size());
#endif
*/

        std::vector< std::vector< int > >     father_models = lmodels;

	for ( unsigned i = 0; i < op_effs.size(); i++ )
	{

//                std::cout << "Applying operator to models: ";
//                sm_task.print_operator_full_notab( op_effs[i], std::cout );
		for ( unsigned j = 0; j < father_models.size(); j++ )
		{
			bool applicable = true;
			for ( unsigned k = 0; k < op_effs[i]->prec_vec().size(); k++ )
				if ( father_models[j][ op_effs[i]->prec_vec()[k] ] < 0 )
				{
					applicable = false;
					break;
				}	
			if ( !applicable ) continue;
/*
#ifdef DEBUG
			changed[j] = true;
//			sm_task.print_operator_full_notab( op_effs[i], std::cout );
//			std::cout << std::endl;
#endif	
*/
			for ( unsigned k = 0; k < op_effs[i]->add_vec().size(); k++ )
				lmodels[j][ op_effs[i]->add_vec()[k] ] =  op_effs[i]->add_vec()[k]  ;
			for ( unsigned k = 0; k < op_effs[i]->del_vec().size(); k++ )
				lmodels[j][ op_effs[i]->del_vec()[k] ] = -op_effs[i]->del_vec()[k];
		}

	}
/*
#ifdef DEBUG
	std::cout << "MODELS changed by the action:" << std::endl;
	for ( unsigned j = 0; j < lmodels.size(); j++ )
		if ( changed[j] )
		{
			std::cout << "s_" << j << ": ";
			for ( unsigned l = 1; l < lmodels[j].size(); l++ ) 
			{
				std::cout << lmodels[j][l] << " " << (lmodels[j][l] > 0 ? " " : " -");
				sm_task.print_fluent( lmodels[j][l] > 0 ? lmodels[j][l] : (unsigned)abs(lmodels[j][l]) );
				std::cout << ", ";
			}
			std::cout << std::endl;
		}
#endif
*/
}







void	SearchNode::action_compilation_rule( std::vector< PDDL::Operator* >& effs )
{
	std::vector<unsigned> lits_becoming_known;
/*
#ifdef DEBUG 
	std::cout << "Action Compilation through op: ";
	sm_task.print_operator( effs[0], std::cout );
	std::cout << std::endl;
#endif
*/
	for ( unsigned k = 0; k < effs.size(); k++ )
	{
		Atom_Vec eff_cond;
		for ( unsigned i = 0; i < effs[k]->prec_vec().size(); i++ )
			if ( !effs[k]->hard_preconds().isset( effs[k]->prec_vec()[i] ) )
				eff_cond.push_back( effs[k]->prec_vec()[i] );
		if ( effs[k]->del_vec().empty() ) continue;	
		bool can_execute = true;
		std::vector<unsigned> unknown;
		for ( unsigned i = 0; i < eff_cond.size(); i++ )
		{
			unsigned pi = eff_cond[i];
			if ( !b->known_set().isset(pi) && !b->unknown_set().isset(pi) ) 
			{
				can_execute = false;
				break;
			}
			if ( b->unknown_set().isset(pi) )
				unknown.push_back(pi);
		}
		if ( !can_execute || unknown.size() > 1 || unknown.empty() ) continue;
		
		unsigned candidate = unknown[0];

		if ( !effs[k]->dels().isset(candidate) ) continue;
/*
#ifdef DEBUG
		std::cout << "Found candidate ";
		sm_task.print_fluent( candidate, std::cout );
		std::cout << std::endl;
#endif
*/
		std::vector<unsigned> adding;
		for ( unsigned j = 0; j < effs.size(); j++ )
			if ( effs[j]->adds().isset(candidate) )
				adding.push_back( j );
/*
#ifdef DEBUG
		std::cout << "\t" << adding.size() << " rules adding candidate found" << std::endl;
#endif
*/
		bool all_rules_comply = true;
		for ( unsigned j = 0; j < adding.size(); j++ )
		{
			if ( adding[j] == k ) continue;
			Atom_Vec& dj_cond = effs[adding[j]]->prec_vec();
/*
#ifdef DEBUG
			std::cout << "\tChecking deleting rule condition: ";
			for ( unsigned i = 0; i < dj_cond.size(); i++ )
			{
				sm_task.print_fluent( dj_cond[i], std::cout );
				std::cout << " ";
			}
			std::cout << std::endl;
#endif
*/

			bool at_least_one_false = false;
			for (unsigned i = 0; i < dj_cond.size(); i++ )
			{
				if ( !b->known_set().isset( dj_cond[i] )
					&& !b->unknown_set().isset( dj_cond[i] ) )
				{
					at_least_one_false = true;
/*
#ifdef DEBUG
					std::cout << "\t\tFluent ";
					sm_task.print_fluent( dj_cond[i], std::cout );
					std::cout << " found in K~L" << std::endl;
#endif
*/
					break;
				}
			}
			if ( !at_least_one_false )
			{
/*
#ifdef DEBUG
				std::cout << "\t\tNo fluent found in K~L!" << std::endl;
#endif
*/
				all_rules_comply = false;
				break;
			}
		}
		if ( all_rules_comply )
		{
/*
#ifdef DEBUG
			std::cout << "\tLit is going to be known" << std::endl;
#endif
*/
			lits_becoming_known.push_back( candidate );	
		}
	}	

	for ( unsigned i = 0; i < lits_becoming_known.size(); i++ )
	{
		unsigned p =  lits_becoming_known[i];	
		unsigned neg_p = sm_task.not_equivalent(p);
			b->remove_unknown_lit( p );
		if ( neg_p != 0 )
		{
			b->add_known_lit( neg_p );
			b->remove_unknown_lit( neg_p );	
		}
	}
	if ( !lits_becoming_known.empty() )
	{
		b->unknown_vec().clear();
		unsigned p = b->unknown_set().first();
		while ( p != 0 )
		{
			b->unknown_vec().push_back(p);
			p = b->unknown_set().next(p);
		}
	}
/*
#ifdef DEBUG
	std::cout << "Belief after Action Compilation" << std::endl;
	b->print();
#endif
*/
}

void	SearchNode::compute_persist_set( Belief& b, 
					std::vector< PDDL::Operator*>& effs, 
					PDDL::Fluent_Set& not_affected )
{
	for ( unsigned p = 1; p < (unsigned)(sm_task.fluent_count()-1); p++ )
	{
		if ( not_affected.isset(p) ) continue;
		std::vector<unsigned> affecting;
		for ( unsigned k = 0; k < effs.size(); k++ )
		{
			if ( effs[k]->adds().isset(p) )
				affecting.push_back(k);
			if ( effs[k]->dels().isset(p) )
				affecting.push_back(k);
		}
		if ( affecting.empty() )
			not_affected.set(p);
		else
		{
			bool none_executes = true;
			for ( unsigned k = 0; k < affecting.size(); k++ )
				if ( condition_may_execute( b, effs[ affecting[k] ]->prec_vec() ) )
				{
					none_executes = false;
					break;
				}	
			if ( none_executes )
				not_affected.set(p);
		}
	}	
}

void	SearchNode::preserve_persisting_lits( Belief& b, Belief& bp, 
						PDDL::Fluent_Set& persist,
						PDDL::Fluent_Set& processed )
{
	for ( unsigned k = 0; k < b.known_vec().size(); k++ )
	{
		unsigned p = b.known_vec()[k];
		if ( persist.isset(p) )
		{
/*
#ifdef DEBUG
			std::cout << "[NOT AFFECTED] ";
			std::cout << "Known ";
			sm_task.print_fluent(p);
			std::cout << " in b -> Known ";
			sm_task.print_fluent(p);
			std::cout << " in b'" << std::endl;
#endif
*/
			bp.add_known_lit(p);
			processed.set(p);
		}
	}
	for ( unsigned k = 0; k < b.unknown_vec().size(); k++ )
	{
		unsigned p = b.unknown_vec()[k];
		if ( persist.isset(p) )
		{
/*
#ifdef DEBUG
			std::cout << "[NOT AFFECTED] ";
			std::cout << "Unknown ";
			sm_task.print_fluent(p);
			std::cout << " in b -> Unknown ";
			sm_task.print_fluent(p);
			std::cout << " in b'" << std::endl;
#endif
*/			bp.add_unknown_lit(p);
			processed.set(p);
		}
	}
	
}

void	SearchNode::retrieve_effect_conditions( std::vector<PDDL::Operator*>& effs,
						std::vector<Atom_Vec>& conds )
{
	for ( unsigned k = 0; k < effs.size(); k++ )
	{
		Atom_Vec eff_cond;
		for ( unsigned i = 0; i < effs[k]->prec_vec().size(); i++ )
			if ( !effs[k]->hard_preconds().isset( effs[k]->prec_vec()[i] ) )
				eff_cond.push_back( effs[k]->prec_vec()[i] );
		conds.push_back( eff_cond );
	}
	
}

void	SearchNode::check_support_rule( Belief& b, 
					std::vector<Atom_Vec>& conds,
					std::vector<bool>& results )
{
	for ( unsigned k = 0; k < conds.size(); k++ )
	{
		bool support_rule_holds = true;
		for ( unsigned i = 0; i < conds[k].size(); i++ )
		{
			if ( !b.known_set().isset(conds[k][i]) )
			{
				support_rule_holds = false;
				break;
			}
		}
		results[k] = support_rule_holds;
	}
}

void	SearchNode::check_cancellation_rule( 	Belief& b, 
						std::vector<Atom_Vec>& conds,
						std::vector<bool>& results )
{
	for ( unsigned k = 0; k < conds.size(); k++ )
	{
		bool	cancellation_rule_holds = true;
		for ( unsigned i = 0; i < conds[k].size(); i++ )
		{
			unsigned neg_ci = sm_task.not_equivalent(conds[k][i]);
			assert( neg_ci != 0 );
			if ( !b.unknown_set().isset( neg_ci ) )
			{
				cancellation_rule_holds = false;
				break;
			}
		}
		results[k] = cancellation_rule_holds;
	}
}

bool	SearchNode::condition_may_execute( Belief& b,
						Atom_Vec& cond )
{
	for ( unsigned i = 0; i < cond.size(); i++ )
		if ( !b.known_set().isset(cond[i])
			&& !b.unknown_set().isset( cond[i] ) )
			return false;
	return true;
}

void	SearchNode::check_action_compilation(	Belief& b,
						std::vector<Atom_Vec>& conds,
						std::vector<PDDL::Operator*>& effs,
						std::vector<bool>& result,
						std::vector<unsigned>& result_lit )
{
	for ( unsigned k = 0; k < effs.size(); k++ )
	{
		bool 			can_execute = true;
		std::vector<unsigned>	unknown;
		for ( unsigned i = 0; i < conds[k].size(); i++ )
		{
			unsigned ci = conds[k][i];
			if ( !b.known_set().isset(ci) && !b.unknown_set().isset(ci) ) 
			{
				can_execute = false;
				break;
			}
			if ( b.unknown_set().isset(ci) )
				unknown.push_back( ci );
		}
		if ( !can_execute || unknown.size() > 1 || unknown.empty() ) continue;
		unsigned not_L 	= unknown[0];
/*
#ifdef DEBUG
		std::cout << "Found candidate for action compilation!" << std::endl;
		sm_task.print_fluent( not_L, std::cout );
		std::cout << std::endl;
#endif
*/
		unsigned L 	= sm_task.not_equivalent(not_L);
		//assert( L != 0 );
		if ( L != 0 )
		{
			if ( !effs[k]->adds().isset(L)  )
			{
				result[k] = false;
				result_lit[k] = 0;
/*
#ifdef DEBUG
				std::cout << "\t rule doesn't comply with C & ~L -> L" << std::endl;
#endif
*/
				continue;
			}
			std::vector<unsigned>	adding;
			for ( unsigned j = 0; j < effs.size(); j++ )
			{
				if ( j == k ) continue;
				if ( effs[j]->adds().isset(not_L) )
					adding.push_back(j);	
			}
			bool act_comp_holds = true;
			for ( unsigned i = 0; i < adding.size(); i++ )
				if ( condition_may_execute( b, conds[adding[i]] ) )
				{
					act_comp_holds = false; 
					break;
				}
			if ( act_comp_holds )
			{
				result[k] = true;
				result_lit[k] = L;
/*
#ifdef DEBUG
				std::cout << "Atom "; sm_task.print_fluent( L, std::cout );
				std::cout << " should become known because of action compilation!" << std::endl;
				std::cout << std::endl;
#endif
*/
			}
			else
			{
				result[k] = false;
				result_lit[k] = 0;
			}
			continue;
		}
		else
		{
			result[k] = false;
			result_lit[k] = 0;
		}
	}	
}


void	SearchNode::k0_projection( std::vector< PDDL::Operator* >& effs )
{
        if (father == NULL) return; // Don't project Root

	Belief& b = *(father->b); // Father belief
	Belief& bp = *(this->b); // successor belief

/*#ifdef DEBUG
	std::cout << "K0 projection" << std::endl;
	std::cout << "Through operator: ";
	sm_task.print_operator( effs[0], std::cout );
	std::cout << std::endl;
	std::cout << "Parent belief: " << std::endl;
	b.print( std::cout );
	std::cout << std::endl;
#endif
*/
	PDDL::Fluent_Set not_affected( sm_task.fluent_count()+1 );
	PDDL::Fluent_Set processed( sm_task.fluent_count() + 1 );
	// MRJ: TODO: This can be precomputed
	compute_persist_set( b, effs, not_affected );
	// MRJ: preserve KL and ~KL lits
/*
#ifdef DEBUG
	std::cout << "Preserving atoms not affected by action" << std::endl;
#endif
*/
	preserve_persisting_lits( b, bp, not_affected, processed );	
/*
#ifdef DEBUG
	std::cout << "Checking effects" << std::endl;
#endif
*/
	std::vector< Atom_Vec > eff_conds;
	retrieve_effect_conditions( effs, eff_conds );
	std::vector<bool>	support_holds( eff_conds.size(), false );
	std::vector<bool>	cancellation_holds( eff_conds.size(), false );
	std::vector<bool>	act_comp_holds( eff_conds.size(), false );
	std::vector<unsigned>	act_comp_lit( eff_conds.size() );

	check_support_rule( b, eff_conds, support_holds );
	check_cancellation_rule( b, eff_conds, cancellation_holds );
	check_action_compilation( b, eff_conds, effs, act_comp_holds, act_comp_lit );

	PDDL::Fluent_Set	potentially_unknown( sm_task.fluent_count()+1 );
	for ( int p = 1; p < sm_task.fluent_count()-1; p++ )
	{
		if ( processed.isset(p) ) continue; // Nothing to do here
		//if ( !sm_task.fluents()[p]->is_pos() ) continue;
/*
#ifdef DEBUG
		std::cout << "Processing fluent "; sm_task.print_fluent(p, std::cout);
		std::cout << std::endl;
#endif	
*/
		std::vector<unsigned> adding_p;
		for ( unsigned k = 0; k < effs.size(); k++ )
			if ( effs[k]->adds().isset( p ) )
				adding_p.push_back(k);	
	
		bool support_applies = false;
		bool cancellation_applies = false;	
		for ( unsigned k = 0; k < adding_p.size(); k++ )
		{
			if ( support_holds[adding_p[k]] )
			{
				support_applies = true;
/*
#ifdef DEBUG
				std::cout << "Effect: Condition: " << std::endl;
				for ( unsigned i = 0; i < eff_conds[adding_p[k]].size(); i++ )
				{
					sm_task.print_fluent( eff_conds[adding_p[k]][i], std::cout );
					if ( i < eff_conds[adding_p[k]].size()-1 )
						std::cout << " ";
				}
				std::cout << " KC holds" << std::endl;
#endif
*/
				continue;
			}
			if ( cancellation_holds[k] )
			{
				cancellation_applies = true;
/*
#ifdef DEBUG
				std::cout << "Effect: Condition: " << std::endl;
				for ( unsigned i = 0; i < eff_conds[adding_p[k]].size(); i++ )
				{
					sm_task.print_fluent( eff_conds[adding_p[k]][i], std::cout );
					if ( i < eff_conds[adding_p[k]].size()-1 )
						std::cout << " ";
				}
				std::cout << " ~K~C holds" << std::endl;
#endif
*/
				continue;	
			}
		}

		// Check if p is the resulting lit for some action compilation effect
		bool 		act_comp_effect = false;
		for ( unsigned i = 0; i < act_comp_lit.size(); i++ )
		{
			if ( act_comp_lit[i] == p )
			{
				act_comp_effect = true;
				break;
			}
		}

		if ( !support_applies && !cancellation_applies && !act_comp_effect )
		{
/*
#ifdef DEBUG
			std::cout << "Atom "; sm_task.print_fluent(p, std::cout );
			std::cout << " becomes POTENTIALLY-UNKNOWN because no rule applying it" << std::endl;
#endif
*/
			if ( !not_affected.isset(p) )
				potentially_unknown.set(p);	
		}
		else if ( ( !support_applies && !cancellation_applies && act_comp_effect )
			|| ( support_applies && !cancellation_applies && !act_comp_effect)
			|| ( support_applies && !cancellation_applies && act_comp_effect ) )
		{

#ifdef DEBUG
			std::cout << "Atom "; sm_task.print_fluent(p, std::cout );
			std::cout << " becomes KNOWN because of ";
			if ( support_applies ) std::cout << "SUPPORT ";
			if ( act_comp_effect ) std::cout << "ACTION COMPILATION ";
			std::cout << "rule" << std::endl;			
#endif			

			bp.add_known_lit(p);
			processed.set(p);
		}
		else if ( !support_applies && cancellation_applies && !act_comp_effect )
		{
/*
#ifdef DEBUG
			std::cout << "Atom "; sm_task.print_fluent(p, std::cout );
			std::cout << " becomes POTENTIALLY-UNKNOWN because of CANCELLATION" << std::endl;
#endif		
*/
			potentially_unknown.set(p);	
		}
		else
		{
/*
#ifdef DEBUG
			std::cout << "Support & cancellation rules applying for atom ";
			sm_task.print_fluent( p, std::cout );

			std::cout <<  " which will considered KNOWN" << std::endl;
#endif
*/
			bp.add_known_lit( p );
			processed.set(p);
		}
	}

	for ( unsigned p = 1; p < sm_task.fluent_count()-1; p++ )
	{
		if ( processed.isset(p) ) continue;
		if ( potentially_unknown.isset(p) )
		{
			std::vector<unsigned> deleting_p;
			for ( unsigned k = 0; k < effs.size(); k++ )
				if ( effs[k]->dels().isset( p ) )
					deleting_p.push_back(k);	
			bool is_false = false;
			for ( unsigned k = 0; k < deleting_p.size(); k++ )
				if ( support_holds[deleting_p[k]] )
				{
					is_false = true;
					break;
				}
			if (is_false) continue;

			unsigned not_p = sm_task.not_equivalent(p);
			if ( not_p == 0 )
			{
/*
#ifdef DEBUG
				std::cout << "Atom "; sm_task.print_fluent(p,std::cout);
				std::cout << " becomes UNKNOWN" << std::endl;
#endif	
*/
				/*
				std::vector<unsigned> rel_oofs;
				for ( unsigned i = 0; i < sm_task.oneofs().size(); i++ )
				{	
					Atom_Vec& one_ofs = sm_task.oneofs()[i];
					if ( std::find( one_ofs.begin(), one_ofs.end(), p )
						!= one_ofs.end() )
						rel_oofs.push_back( i );
				}
					
				bool one_of_is_known = false;
				for ( unsigned i = 0; i < rel_oofs.size(); i++ )
				{
					Atom_Vec& one_ofs = sm_task.oneofs()[rel_oofs[i]];
					for ( unsigned k = 0; k < one_ofs.size(); k++ )
						if ( bp.known_set().isset( one_ofs[k] ) )
						{
							one_of_is_known = true;
							break;
						}
					if ( one_of_is_known ) break;
				}
				if ( !one_of_is_known )
				*/
				bp.add_unknown_lit(p);
				
			}
			else
			{
				if ( bp.known_set().isset( not_p ) )
				{
/*
#ifdef DEBUG
					std::cout << "Atom "; sm_task.print_fluent(p,std::cout);
					std::cout << " known to be FALSE" << std::endl;
#endif		
*/	
				}
				else if ( bp.unknown_set().isset( not_p ) )
				{
/*
#ifdef DEBUG
					std::cout << "Atom "; sm_task.print_fluent(p,std::cout);
					std::cout << " becomes UNKNOWN" << std::endl;
#endif				
*/
					bp.add_unknown_lit(p);
				}
				else if ( potentially_unknown.isset( not_p ) )
				{
/*
#ifdef DEBUG
					std::cout << "Atom "; sm_task.print_fluent(p,std::cout);
					std::cout << " becomes UNKNOWN" << std::endl;
#endif				
*/
					/*
					std::vector<unsigned> rel_oofs;
					for ( unsigned i = 0; i < sm_task.oneofs().size(); i++ )
					{	
						Atom_Vec& one_ofs = sm_task.oneofs()[i];
						if ( std::find( one_ofs.begin(), one_ofs.end(), not_p )
							!= one_ofs.end() )
							rel_oofs.push_back( i );
					}
						
					bool one_of_is_known = false;
					for ( unsigned i = 0; i < rel_oofs.size(); i++ )
					{
						Atom_Vec& one_ofs = sm_task.oneofs()[rel_oofs[i]];
						for ( unsigned k = 0; k < one_ofs.size(); k++ )
							if ( bp.known_set().isset( one_ofs[k] ) )
							{
								one_of_is_known = true;
								break;
							}
						if ( one_of_is_known ) break;
					}
					if ( !one_of_is_known )
					*/
					bp.add_unknown_lit(p);
	
				}
				else
				{
/*
#ifdef DEBUG
					std::cout << "Atom "; sm_task.print_fluent(not_p,std::cout);
					std::cout << " known to be false, but "; sm_task.print_fluent(p, std::cout);
					std::cout << "is unknown!" << std::endl;
					assert( false );
#endif
*/
				}
			}
			
		}
		else
		{
/*
#ifdef DEBUG
			std::cout << "Atom "; sm_task.print_fluent(p,std::cout);
			std::cout << " known to be FALSE" << std::endl;
#endif
*/
		}
	}

	if ( bp.check_consistency() )
	{
/*
#ifdef DEBUG
		std::cout << "K0 projection returned inconsistent state!" << std::endl;
		bp.print();
#endif
*/		assert(false);	
	}
}

/** Generates the successors node.
 * We suppose that the op is applicable in
 * the current belief state. **/
SearchNode*  SearchNode::successor( unsigned op )
{
	SearchNode* succ = new SearchNode;
	/* NB: Assumes that op is executable in father */

	
	succ->b = new Belief::Belief();
	succ->gn = gn + sm_task.op_cost( op );
	succ->op = op;
	succ->father = this;
	succ->timestep = timestep+1;
	//hS = hX = hn = 0;
	return succ;
}



// void	SearchNode::check_dead_models()
// {
// 	Planner_Stats&	stats = Planner_Stats::instance();
// 	num_dead_models = 0;
// 	if ( father != NULL )
// 	{
// 		for ( unsigned k = 0; k < models.size(); k++ )
// 		{
// 			dead_model[k] = father->dead_model[k];
// 			if ( dead_model[k] ) num_dead_models++;
// 		}
// 	}
// 	for ( unsigned k = 0; k < models.size(); k++ )
// 	{
// 		if ( dead_model[k] ) 
// 			continue;
// 		std::vector<int>& k_model = models[k];
// 		for ( unsigned j = k+1; j < models.size(); j++ )
// 		{
// 			if ( dead_model[j] ) 
// 				continue;
// 			std::vector<int>& j_model = models[j];
// 			bool are_equal = true;
// 			for ( int f = 1; f < sm_task.fluent_count()-1; f++ )
// 			{
// 				if ( std::sgn(k_model[f]) != std::sgn(j_model[f]) )
// 				{
// 					are_equal = false;
// 					break;
// 				}
// 			}
// 			if ( are_equal )
// 			{
// 				num_dead_models++;
// 				dead_model[j] = true;
// 			}
// 		}
// 	}
// 	stats.notify_num_dead_models( num_dead_models );
// }

void	SearchNode::determine_testable_precs( std::vector<PDDL::Operator*>& ops )
{
	for ( unsigned i = 0; i < sm_task.prototype_ops().size(); i++ )
	{
		PDDL::Operator* o = sm_task.prototype_ops()[i];
		bool testable = true;
		Atom_Vec&	prec = o->hard_prec_vec();
		for ( unsigned k = 0; k < prec.size(); k++ )
		{
			if ( !b->known_set().isset(prec[k]) 
				&& !b->unknown_set().isset(prec[k]) )
			{
				testable = false;
				break;
			}
			if ( !potentially_known[prec[k]] )
			{
				testable = false;
				break;
			}

		}
		if ( !testable ) continue;
		for ( unsigned k = 0; k < prec.size(); k++ )
			if ( b->unknown_set().isset(prec[k]) )
			{
				ops.push_back( o );
				break;
			}
	}
}

void	SearchNode::determine_testable_goals( Atom_Vec& tG )
{
	Atom_Vec& G = sm_task.goal_state();
	for ( unsigned i = 0; i < G.size(); i++ )
	{
		if ( !b->known_set().isset(G[i]) && !b->unknown_set().isset(G[i] ) )
			continue;
		if ( b->known_set().isset(G[i]) )
			continue;
		if ( potentially_known[G[i]]  )
			tG.push_back( G[i] );
	}
	
}

void 	SearchNode::test_conjunction( MiniSAT& solver, Atom_Vec& atoms )
{
	Atom_Vec atoms_to_test;
	
	for ( unsigned k = 0; k < atoms.size(); k++ )
	{
		if ( !b->unknown_set().isset(atoms[k]) && ! b->known_set().isset(atoms[k]) )
			continue;
		if ( b->known_set().isset(atoms[k] ) )
			continue;
		atoms_to_test.push_back( atoms[k] );
	} 
	if ( atoms_to_test.empty() ) return;
	
	std::vector<int> lits;
	
	for ( unsigned k = 0; k < atoms_to_test.size(); k++ )
	{
		unsigned p = atoms_to_test[k];
		
		if ( sm_task.fluents().at(p)->is_pos() )
			lits.push_back( -theory_step->fluent_var(p) );
		else
			lits.push_back( theory_step->fluent_var(p) );
	}	
	solver.add_clause( lits );
	if ( solver.solve( ) )
		solver.retract_last_clause();
	else
	{
		for ( unsigned k = 0; k < atoms_to_test.size(); k++ )
		{
			unsigned p = atoms_to_test[k];
			unsigned neg_p = sm_task.not_equivalent( atoms_to_test[k] );

			b->add_known_lit( p );
			b->remove_unknown_lit( p );
			if ( neg_p )
				b->remove_unknown_lit( neg_p );
		}
		for ( unsigned k = 0; k < lits.size(); k++ )
		{
			solver.add_fact( -lits[k] );
			theory_step->add_fact( -lits[k] );
		}
	}
}

/** In case the width of the problem is <= 1 an atom is true in the belief
 *  if it is true in all the models. 
 **/
void SearchNode::check_known_atoms_from_samples(std::vector<std::vector<int> >& samples)
{
        unsigned count_pos;
        unsigned count_neg;
/*
#ifdef DEBUG
        std::cout << "Projected Models\n";
	std::cout << "States in b' ( " << samples.size() <<  " )  " << std::endl;	
	for ( unsigned k = 0; k < samples.size(); k++ )
	{
		std::cout << "s_" << k << ": ";
		for ( unsigned l = 1; l < samples[k].size(); l++ ) 
                        {
                                unsigned f = std::abs( samples[k][l] );
                                int fsgn = std::sgn( samples[k][l] );
                                if ( sm_task.fluents()[f]->is_pos() )
                                {
                                        std::cout << ( fsgn > 0 ? " " : " -" );
                                        sm_task.print_fluent(f, std::cout );
                                        std::cout << std::endl;
                                }
                        }
                              std::cout << std::endl;
                      }
        }
#endif
*/            
        for ( unsigned f = 1; f < (unsigned)(sm_task.fluent_count()-1); f++ )
                {
                        if (!potentially_known[f ]) continue;
                        if (b->known_set().isset( f ) )
                                continue;
                        count_pos = 0;
                        count_neg = 0;
                        unsigned neg_f = sm_task.not_equivalent( f );
                        for ( unsigned j = 0; j < samples.size(); j++ )
                        {
                                if ( samples[j][f] > 0 )
                                        count_pos++;
                                else 
                                {
                                        count_neg++;
                                        break;
                                }
                        }
                        if ( count_neg == 0) // Lit is true
                        {
#ifdef DEBUG
                                std::cout << "Adding known fluent";
                                sm_task.print_fluent(f);
                                std::cout << std::endl;
#endif
                                b->add_known_lit( f );
                                b->remove_unknown_lit( f );
                                if ( neg_f )
                                {
                                        b->remove_known_lit( neg_f );
                                        b->remove_unknown_lit( neg_f );
                                }
                        }
                        else if ( count_pos == 0 ) // Lit is false
                        {
                                if ( neg_f )
                                {              
                                        b->add_known_lit( neg_f );
                                        b->remove_unknown_lit( neg_f );
                                }
                                b->remove_known_lit( f );
                                b->remove_unknown_lit( f );
                        }
                }
}



void   SearchNode::belief_tracking_with_sat()
{
        theory_step = new Theory_Fragment( op, father->theory_step );
        
	
	if ( b->unknown_vec().empty() )
		return;

        make_clauses();

	std::vector<PDDL::Operator*>	testable_actions;
	Atom_Vec			testable_goals;
	determine_testable_precs(testable_actions);
	determine_testable_goals(testable_goals);
	
	if ( testable_actions.empty() && testable_goals.empty() )
		return;	

	MiniSAT solver(this);

#ifdef DEBUG
	if ( !solver.solve() )
	{
		std::cout << "INCONSISTENT Belief!!!" << std::endl;
		describe();
		assert( false );
	}
#endif
/*
	// Check action preconditions	
	for ( unsigned i = 0; i < testable_actions.size(); i++ )
	{
		PDDL::Operator* o = testable_actions[i];
		Atom_Vec& 	precs = o->hard_prec_vec();
		for ( unsigned j = 0; j < precs.size(); j++ )
			if ( !is_literal_known( solver, precs[j] ) )
				break;
		//test_conjunction( solver, precs );
	}	
	// Check goals
	for ( unsigned j = 0; j < testable_goals.size(); j++ )
		if ( !is_literal_known( solver, testable_goals[j] ) )
			break;

	//test_conjunction( solver, testable_goals );
*/
	for ( unsigned i = 0; i < sm_task.prototype_ops().size(); i++ )
	{
		PDDL::Operator* o = sm_task.prototype_ops()[i];
#ifdef DEBUG
		std::cout << "Testing ";
		sm_task.print_operator( o, std::cout );
		std::cout << " operator preconditions" << std::endl;
#endif
		Atom_Vec&	precs = o->hard_prec_vec();
		for ( unsigned j = 0; j < precs.size(); j++ )
		{
#ifdef DEBUG
			std::cout << "\t Precondition: ";
			sm_task.print_fluent( precs[j], std::cout );
#endif
			if ( !is_literal_known( solver, precs[j] ) )
			{
#ifdef DEBUG
				std::cout << " remains unknown!" << std::endl;
#endif
				break;
			}
#ifdef DEBUG
			std::cout << " is known!" << std::endl;
		
#endif
		}	
	}
	// Check goal
	Atom_Vec& G = sm_task.goal_state();
	for ( unsigned i = 0; i < G.size(); i++ )
	{
#ifdef DEBUG
		std::cout << "Checking goal: ";
		sm_task.print_fluent( G[i], std::cout );
#endif
		if ( !is_literal_known( solver, G[i] ) ) 
		{
#ifdef DEBUG
			std::cout << " is unknown!" << std::endl;
#endif
			break;
		}
#ifdef DEBUG
		std::cout << " is known!" << std::endl;
#endif 
	}

}



void	SearchNode::do_belief_tracking(std::vector< std::vector<int> >& models)
{
        Planner_Stats&	stats = Planner_Stats::instance();
        std::vector<unsigned>                   prefix;
        SearchNode * n = this;

        // builds the plan prefix 
        while( n != NULL )
        {
                prefix.push_back(n->op);
                n = n->father;
        }
        n = this;
#ifdef DEBUG
        std::cout << "Printing the prefix in order we've it "<< std::endl;
        for (unsigned o = 0; o < prefix.size(); o++)
        {
                std::cout <<"(" << o << ". "; 
                sm_task.print_operator(  prefix[o], std::cout );
                std::cout <<")"; 
        }
        std::cout << std::endl;
#endif

        potentially_known.resize( sm_task.fluent_count() ); 

        for (int o = (prefix.size()-1); o >= 0; o--)
        {
                std::vector<PDDL::Operator*>& ops = sm_task.useful_ops()[prefix[o]]->same_op_vec();
                update_models(ops, models);	
//                dead_model = father->dead_model;

        } // end (for prefix)
#ifdef DEBUG
                std::cout << "Local MODELS changed by the action:" << std::endl;
                for ( unsigned j = 0; j < models.size(); j++ )
                {
                        std::cout << "s_" << j << ": ";
                        for ( unsigned l = 1; l < models[j].size(); l++ ) 
                        {
                                std::cout << models[j][l] << " " << (models[j][l] > 0 ? " " : " -");
                                sm_task.print_fluent( models[j][l] > 0 ? models[j][l] : (unsigned)abs(models[j][l]) );
                                std::cout << ", ";
                        }
                        std::cout << std::endl;
                }
                std::cout << std::endl;
#endif

        std::vector<PDDL::Operator*>& ops = sm_task.useful_ops()[n->op]->same_op_vec();        

        k0_projection(ops);
        
        compute_hash( models ); 
        
        if ( stats.is_width_1() )
                check_known_atoms_from_samples( models );
        else
                belief_tracking_with_sat();


	// Rebuilds Unknown Atoms List
	b->unknown_vec().clear();
	for (unsigned i = 1; i < sm_task.fluents().size()-1; i++)
		if ( b->unknown_set().isset( i ) )
		{
			if( b->known_set().isset(i) )
			{
				std::cout << "Inconsistent belief!" << std::endl;
				std::cout << "Atom: ";
				sm_task.print_fluent( i, std::cout );
				std::cout << " both in K and ~K" << std::endl;
				b->print(std::cout);
				assert(false);
			}
			if ( sm_task.not_equivalent(i)
                             && b->known_set().isset( sm_task.not_equivalent(i)) ) 
			{
				std::cout << "Inconsistent belief!" << std::endl;
				std::cout << "Atom: ";
				sm_task.print_fluent( i, std::cout );
				std::cout << " both in K and ~K" << std::endl;
				b->print(std::cout);
				assert(false);
			}
			b->unknown_vec().push_back( i );
		}	
}

// void SearchNode::replace_dead_sample( std::vector<int>& model )
// {
// 	if ( num_dead_models == 0 ) return; // No room for new models
// 	// Complete model
// 	for ( int x = theory_step->fluent_var(1); x < theory_step->nVars(); x++ )
// 	{
// 		if ( model[x] == 0 )
// 		{
// 			int f = theory_step->recover_fluent(x);
// 			if ( f == 0 ) continue;
// 			model[x] = -model[ theory_step->fluent_var( sm_task.not_equivalent(f) ) ];
// 		}
// 	}

// 	// Check that the model returned by solver is not already in our
// 	// sample set
// 	for ( unsigned k = 0; k < models.size(); k++ )
// 	{
// 		if ( dead_model[k] ) 
// 			continue;
// 		std::vector<int>& k_model = models[k];
// 		bool are_equal = true;
// 		for ( int f = 1; f < sm_task.fluent_count()-1; f++ )
// 		{
// 			if ( std::sgn(k_model[f]) != std::sgn(model[theory_step->fluent_var(f)]) )
// 			{
// 				are_equal = false;
// 				break;
// 			}
// 		}
// 		if ( are_equal )
// 			return;
// 	}
	
// 	// Replace first dead model
// 	for ( unsigned k = 0; k < dead_model.size(); k++ )
// 	{
// 		if ( dead_model[k] )
// 		{
// 			num_dead_models--;
// 			for ( int f = 1; f < sm_task.fluent_count()-1; f++ )
// 				models[k][f] = model[theory_step->fluent_var(f)];
// 			dead_model[k] = false;
// 			Planner_Stats&	stats = Planner_Stats::instance();
// 			stats.notify_dead_sample_replaced();
// 			return;
// 		}
// 	}	
// }

bool SearchNode::is_literal_known( MiniSAT& solver, unsigned p )
{
	unsigned neg_p = sm_task.not_equivalent( p );
	std::vector<int> model;
	NFF_Options& opt = NFF_Options::instance();

	if ( !b->unknown_set().isset(p) )
	{
#ifdef DEBUG
		std::cout << "\t\t Wasn't in the unknown set!" << std::endl;
#endif 
		return true;
	}
	if ( !potentially_known[p] )
	{
#ifdef DEBUG
		std::cout << "\t\t Wasn't potentially known!" << std::endl;
#endif 
		return false;
	}
	int test_literal = 0;
	/* Adding the fact to be checked */
	if ( sm_task.fluents().at(p)->is_pos() ) 
		//test_literal = (int)-(p + next_offset);
		test_literal = -theory_step->fluent_var(p);
	else
	{
		assert( neg_p != 0 );
		test_literal = theory_step->fluent_var(neg_p);
	}
	solver.add_fact( test_literal );
	if ( solver.solve( model ) )
	{
#ifdef DEBUG
		std::cout << "\t\tTheory is SAT. (Fluent remains unknown) ";
		sm_task.print_fluent( p );
		std::cout << std::endl;			
#endif
// 		if ( opt.dynamic_samples() )
// 			replace_dead_sample( model );
		solver.retract_last_fact();
		potentially_known[p] = false;
		return false;
/*
		// We don't know the atom to be true, let's see if we know
		// it to be false
		test_literal = -test_literal;
		solver.add_fact( test_literal );
		if ( solver.solve( model ) )
		{
#ifdef DEBUG
			std::cout << "\t\tTheory is SAT. (Fluent remains unknown) ~";
			sm_task.print_fluent( p );
			std::cout << std::endl;			
#endif
			if ( opt.dynamic_samples() )
				replace_dead_sample( model );
			solver.retract_last_fact();
			potentially_known[p] = false;
			return false;
		}
#ifdef DEBUG
		std::cout << "\t\tTheory is UNSAT! (Fluent is known) ~";
		sm_task.print_fluent( p );
		std::cout << std::endl;
#endif
		b->remove_unknown_lit(p);
		if ( neg_p )
		{
			b->remove_unknown_lit(neg_p);
			b->add_known_lit(neg_p);
		}
*/
	}
	else
	{
#ifdef DEBUG
		std::cout << "\t\tTheory is UNSAT! (Fluent is known)";
		sm_task.print_fluent( p );
		std::cout << std::endl;
#endif
		// Adds literal to Belief
		b->add_known_lit( p );
		b->remove_unknown_lit( p );
		b->remove_unknown_lit( neg_p);
		
	}
	solver.retract_last_fact();
	if ( test_literal )
	{ 
		solver.add_fact( -test_literal );
		theory_step->add_fact( -test_literal );
	}
	return true;
}

void   SearchNode::print_clauses()
{
        std::cout << "PRINTING THE THEORY STEP..... " << std::endl;
        std::cout << "::FACTS " << std::endl;
        assert( theory_step != NULL );
        for ( unsigned k = 0; k < theory_step->unit_clauses().size(); k++ )
                {
                        Minisat::Lit l =theory_step->unit_clauses()[k];
                        Minisat::Var x = Minisat::var(l);
			bool	is_cond_var = false;
			int fluent_index = theory_step->recover_fluent(x);
			int cond_index = theory_step->recover_cond_index(x);
			int timestep;
			if ( fluent_index > 0 || cond_index >= 0 )
			{
				if ( fluent_index == 0 )
					is_cond_var = true;
				timestep = this->timestep;
			}
			else
			{
				fluent_index = father->theory_step->recover_fluent(x);
				timestep = father->timestep;
			}

                        bool sgn = Minisat::sign(l);
                        std::cout << ( sgn ? "~" : "" );
			if ( is_cond_var )
				std::cout << "(EFF_CON_" << fluent_index << ")";
			else
				sm_task.print_fluent( fluent_index, std::cout );
                        std::cout << "_" << timestep;
                        std::cout << std::endl;
                }
        std::cout << "::CLAUSES " << std::endl;
        for ( unsigned k = 0; k < theory_step->clauses().size(); k++ )
                {
                	Minisat::Clause& c = theory_step->get_clause(k);

                        for ( unsigned i = 0; i < c.size(); i++ ) 
                                {
                                        Minisat::Lit l = c[i];
					Minisat::Var x = Minisat::var(l);
					bool	is_cond_var = false;
					int fluent_index = theory_step->recover_fluent(x);
					int cond_index = theory_step->recover_cond_index(x);
					int timestep;
					if ( fluent_index > 0 || cond_index >= 0 )
					{
						if ( fluent_index == 0 )
							is_cond_var = true;
						timestep = this->timestep;
					}
					else
					{
						fluent_index = father->theory_step->recover_fluent(x);
						timestep = father->timestep;
					}
					bool sgn = Minisat::sign(l);
					std::cout << ( sgn ? "~" : "" );
					if ( is_cond_var )
						std::cout << "(EFF_CON_" << fluent_index << ")";
					else
						sm_task.print_fluent( fluent_index, std::cout );

                                        std::cout << "_" << timestep;
                                        if ( i < c.size() -1 )
                                                std::cout << " v ";                  
                                }               
				std::cout << std::endl;
                }


}

void SearchNode::make_root_clauses()
{
	Theory_Fragment* current = theory_step;
	for(std::vector<unsigned>::iterator it = b->known_vec().begin(); 
		it!= b->known_vec().end(); it++)      
	{
		theory_step->add_fact( sm_task.fluents()[*it]->is_pos() ?
					current->fluent_var(*it) :
					-current->fluent_var( sm_task.not_equivalent(*it) ) );
	}

	for ( unsigned p = 1; p < sm_task.fluent_count()-1; p++ )
	{
		if ( b->known_set().isset(p) ) continue;
		if ( b->unknown_set().isset(p) ) continue;
		theory_step->add_fact( sm_task.fluents()[p]->is_pos() ?
					-current->fluent_var(p) :
					current->fluent_var( sm_task.not_equivalent(p) ) );
	}

	// In case we are at Init, add the clauses of the problem
	for(std::vector<std::vector<unsigned> >::iterator cl = b->clauses_vec().begin(); 
		cl != b->clauses_vec().end(); cl++)      
	{
		bool absorbed = false;
		for ( std::vector<unsigned>::iterator it = (*cl).begin(); it != (*cl).end(); it++)
		{ 
			int lit = sm_task.fluents()[*it]->is_pos() ?
					current->fluent_var(*it) : 
					-current->fluent_var( sm_task.not_equivalent(*it) );
			if ( !theory_step->add_lit( lit ) )
			{
				absorbed = true;
				break;
			}
		}
		if (!absorbed)
			theory_step->finish_clause();
	}
/*
#ifdef DEBUG
	print_clauses();
#endif
*/	
	return;

}

void SearchNode::make_clauses()
{
	PDDL::Operator* op_ptr = sm_task.useful_ops()[op];
	Theory_Fragment* current = NULL;
	Theory_Fragment* next = NULL;
	
	current = father->theory_step;
	next = theory_step;
	for(std::vector<unsigned>::iterator it = b->known_vec().begin(); 
		it!= b->known_vec().end(); it++)      
	{
		theory_step->add_fact( sm_task.fluents()[*it]->is_pos() ?
					next->fluent_var(*it) :
					-next->fluent_var( sm_task.not_equivalent(*it) ) );
   	}

	for ( unsigned p = 1; p < (unsigned)(sm_task.fluent_count()-1); p++ )
	{
		if ( b->known_set().isset(p) ) continue;
		if ( b->unknown_set().isset(p) ) continue;
		theory_step->add_fact( sm_task.fluents()[p]->is_pos() ?
					-next->fluent_var(p) :
					next->fluent_var( sm_task.not_equivalent(p) ) );
	}

	std::vector<PDDL::Operator*>& effects = op_ptr->same_op_vec();
	/* Effect axioms */
	for(std::vector<PDDL::Operator*>::iterator it = effects.begin(); it!= effects.end(); it++)
	{  

		std::vector<unsigned>& precs = (*it)->prec_vec();
		std::vector<unsigned>& dels  = (*it)->del_vec();
		std::vector<unsigned>& adds = (*it)->add_vec();
	
		/* Added literals */
		for (std::vector<unsigned>::iterator a = adds.begin(); a != adds.end(); a++ )
		{

			/* Optimization: if lit == not-lit, then we have the same clause
			* as a del one */
			if ( !( (sm_task.fluents())[ *a ])->is_pos() )
				continue;

			int p1 = next->fluent_var(*a);
			if ( !theory_step->add_lit(p1) )
			{
				continue; // clause is absorbed
			}
			/* If action is executable, we don't need preconditions */
			/* Conditions */
			bool	absorbed = false;
			for (std::vector<unsigned>::iterator c = precs.begin(); 
				c != precs.end(); c++ )
			{
				int lit = sm_task.fluents()[ *c ]->is_pos() ?
						-current->fluent_var(*c) :
						current->fluent_var( sm_task.not_equivalent(*c) );
				if ( !theory_step->add_lit(lit) )
				{
					absorbed = true;
					break;
				}
			}
			if ( !absorbed )
				theory_step->finish_clause();
		}

		/* Deleted literals */
		for (std::vector<unsigned>::iterator d = dels.begin(); d != dels.end(); d++ )
		{
			/* Optimization: if lit == not-lit, then we have the same clause
			* as an add one */
			if ( !sm_task.fluents()[ *d ]->is_pos() )
				continue;

			int p1 = next->fluent_var( *d );
			if( !theory_step->add_lit( -p1 ) )
				continue;

			/* Preconditions */
			/* Conditions */
			bool absorbed = false;
			for (std::vector<unsigned>::iterator c = precs.begin(); c != precs.end(); c++ )
			{
				int lit = sm_task.fluents()[*c]->is_pos() ?
						-current->fluent_var(*c) :
						current->fluent_var( sm_task.not_equivalent(*c) );
				if (! theory_step->add_lit(lit) )
				{
					absorbed = true;
					break;
				}
			}
			if (!absorbed)
				theory_step->finish_clause();
		} // end for dels
	}
	
	// Positive Frame Axioms for p
	std::vector<bool>	cond_eq_required(effects.size());
	for ( int p = 1; p < (int)sm_task.fluent_count()-1; p++ )
	{
		if ( !sm_task.fluents()[p]->is_pos() ) continue;

		int p0 = current->fluent_var(p);
		int p1 = next->fluent_var(p);
		
		if ( !theory_step->add_lit(-p0) )
			continue; // absorbed
		if ( !theory_step->add_lit(p1) )
			continue; // absorbed;

		std::vector<int>	relevant_dels_vars;
		for( unsigned k = 0; k < effects.size(); k++)
		{ 
			if ( effects[k]->dels().isset(p) )
			{
				relevant_dels_vars.push_back( theory_step->cond_var(k) );
				cond_eq_required[k] = true;
			}
		}
		for ( unsigned k = 0; k < relevant_dels_vars.size(); k++ )
			theory_step->add_lit( relevant_dels_vars[k] );
		theory_step->finish_clause();
		
	}
	// Negative Frame Axioms for p
	for ( int p = 1; p < (int)sm_task.fluent_count()-1; p++ )
	{
		if ( !sm_task.fluents()[p]->is_pos() ) continue;

		int p0 = current->fluent_var(p);
		int p1 = next->fluent_var(p);
		
		if (!theory_step->add_lit(p0))
			continue; // absorbed
		if (!theory_step->add_lit(-p1))
			continue; // absorbed

		std::vector<int>	relevant_adds_vars;
		for( unsigned k = 0; k < effects.size(); k++)
		{ 
			if ( effects[k]->adds().isset(p) )
			{
				relevant_adds_vars.push_back( theory_step->cond_var(k) );
				cond_eq_required[k] = true;
			}
		}
		for ( unsigned k = 0; k < relevant_adds_vars.size(); k++ )
			theory_step->add_lit( relevant_adds_vars[k] );
		theory_step->finish_clause();
	}
	// Introduce Condition equality clauses
	for ( unsigned k = 0; k < effects.size(); k++ )
	{
		if ( !cond_eq_required[k] ) continue;
		Atom_Vec& precs = effects[k]->prec_vec();
		int cond_var = theory_step->cond_var(k);
		if (!theory_step->add_lit(cond_var))
			continue; // absorbed
		bool absorbed = false;
		for ( unsigned i = 0; i < precs.size(); i++ )
		{
			int ci = precs[i];
			int lit_ci = sm_task.fluents()[ci]->is_pos() ?
						-current->fluent_var(ci)
						: current->fluent_var( sm_task.not_equivalent(ci) );
			if (!theory_step->add_lit(lit_ci))
			{
				absorbed = true;
				break;
			}
		}
		if (!absorbed)
			theory_step->finish_clause();
		for ( unsigned i = 0; i < precs.size(); i++ )
		{
			int ci = precs[i];
			int lit_ci = sm_task.fluents()[ci]->is_pos() ?
						current->fluent_var(ci)
						: -current->fluent_var( sm_task.not_equivalent(ci) );
			if (!theory_step->add_lit(lit_ci))
				continue;
			if (!theory_step->add_lit(-cond_var))
				continue;
			theory_step->finish_clause();
		}

	}
/*
#ifdef DEBUG
	print_clauses();
#endif
*/
}     

void	SearchNode::describe()
{
	std::cout << "Node description:" << std::endl;
	std::cout << "\tg(n)=" << gn << std::endl;
//	std::cout << "\t# dead models =" << num_dead_models << std::endl;
	std::cout << "\th_S(n)=" << hS << std::endl;
	std::cout << "\th_X(n)=" << hX << std::endl;
	std::cout << "\th(n)=" << hn << std::endl;
	std::cout << "\ttb(n)=" << tb << std::endl;
	std::cout << "\tHA_{S}(n)={";
	for ( unsigned k = 0; k < m_hS_HA.size(); k++ )
	{
		sm_task.print_operator( sm_task.prototype_ops()[m_hS_HA[k]]->get_idx(), std::cout );
		if ( k < m_hS_HA.size()-1 ) std::cout << ", ";
	}
	std::cout << "}" << std::endl;
	std::cout << "\tHA_{X}(n)={";
	for ( unsigned k = 0; k < m_hX_HA.size(); k++ )
	{
		sm_task.print_operator( sm_task.prototype_ops()[m_hX_HA[k]]->get_idx(), std::cout );
		if ( k < m_hX_HA.size()-1 ) std::cout << ", ";
	}
	std::cout << "}" << std::endl;
	std::cout << "\t\\pi={";
	sm_task.print_operator( op, std::cout );
	SearchNode* tmp = father;
	while ( tmp != NULL )
	{	
		sm_task.print_operator( tmp->op, std::cout );
		tmp = tmp->father;
	}
	std::cout << "}" << std::endl;
	b->print( std::cout );
	std::cout << std::endl;
	for ( int i = 0; i < 10; i++ )
		std::cout << "=";
	std::cout << std::endl;
}
 
// void	SearchNode::compute_hash()
// {
// 	Hash_Key k;
// 	for ( unsigned f = 1; f < sm_task.fluent_count()-1; f++ )
// 	{
// 		k.add( f );
// 		unsigned count_pos = 0;
// 		unsigned count_neg = 0;
// 		for ( unsigned j = 0; j < models.size(); j++ )
// 		{
// 			if ( models[j][f] > 0 )
// 				count_pos++;
// 			else
// 				count_neg++;
// 		}
// 		k.add( count_pos );
// 		k.add( count_neg );
// 		if ( count_pos == 0 || count_neg == 0 )
// 		{
// #ifdef DEBUG
// 			std::cout << "Fluent: ";
// 			sm_task.print_fluent(f, std::cout );
// 			std::cout << " is potentially known " << (count_pos ? "to be TRUE":"to be FALSE") << std::endl;
// #endif
// 			potentially_known[f] = true;
// 		}
// 		else
// 			potentially_known[f] = false;
// 	}
// 	m_hash_key = k;	
// }

void	SearchNode::compute_hash(std::vector<std::vector<int> >& models)
{
	Hash_Key k;
	for ( unsigned f = 1; f < sm_task.fluent_count()-1; f++ )
	{
		k.add( f );
		unsigned count_pos = 0;
		unsigned count_neg = 0;
		for ( unsigned j = 0; j < models.size(); j++ )
		{
			if ( models[j][f] > 0 )
				count_pos++;
			else
				count_neg++;
		}
		k.add( count_pos );
		k.add( count_neg );
		if ( count_pos == 0 || count_neg == 0 )
		{
#ifdef DEBUG
			std::cout << "Fluent: ";
			sm_task.print_fluent(f, std::cout );
			std::cout << " is potentially known " << (count_pos ? "to be TRUE":"to be FALSE") << std::endl;
#endif
			potentially_known[f] = true;
		}
		else
			potentially_known[f] = false;
	}
	m_hash_key = k;	
} 


bool	SearchNode::same_models( SearchNode& o, std::vector<std::vector<int> >& models )
{
        std::vector<unsigned>                   prefix_1;
        std::vector<unsigned>                   prefix_2;
        SearchNode * n = this;

        // 1. gets the two prefixes
        while( n != NULL )
        {
                prefix_1.push_back(n->op);
                n = n->father;
        }
        n = &o;
        while( n != NULL )
        {
                prefix_2.push_back(n->op);
                n = n->father;
        }

	for ( unsigned k = 0; k < models.size(); k++ )
	{
                // 2. progress the 2 models and
                std::vector<int> model_1 = models[k];
                std::vector<int> model_2 = models[k];
                for (int p = (prefix_1.size()-1); p >= 0; p--)
                {
                        std::vector<PDDL::Operator*>& ops = 
                                sm_task.useful_ops()[prefix_1[p]]->same_op_vec();
                        update_single_model(ops, model_1 );	
                } 

                for (int p = (prefix_2.size()-1); p >= 0; p--)
                {
                        std::vector<PDDL::Operator*>& ops = 
                                sm_task.useful_ops()[prefix_2[p]]->same_op_vec();
                        update_single_model(ops, model_2 );	
                } 


                // 3. compare them

		for ( unsigned f = 1; f < (unsigned)(sm_task.fluent_count()-1); f++ )
		{
			if ( std::sgn(model_1[f]) != std::sgn(model_2[f]) )
				return false;
		}
	}
	return true;
}

bool   SearchNode::is_equal( SearchNode& o, std::vector< std::vector<int> >& models )
{
        Planner_Stats& stats = Planner_Stats::instance();
        //AA: no SAT calls to check entailment
        if ( NO_SAT_CALLS && !stats.is_width_1()) 
                return false;
        
        if ( *b != *(o.b) ) return false;
        if ( !same_models( o, models ) ) return false;
        if ( stats.is_width_1() ) return true; 
        
        return check_entailment(o);
}

// bool	SearchNode::same_models( SearchNode& o )
// {
// 	for ( unsigned k = 0; k < models.size(); k++ )
// 	{
// 		for ( unsigned f = 1; f < (unsigned)(sm_task.fluent_count()-1); f++ )
// 		{
// 			if ( std::sgn(models[k][f]) != std::sgn(o.models[k][f]) )
// 				return false;
// 		}
// 	}
// 	return true;
// }

bool	SearchNode::check_entailment( SearchNode& o )
{
#ifdef DEBUG
	std::cout << "CHECKING ENTAILMENT to AVOID repeated STATES" << std::endl;
#endif
	if ( gn == 0 && o.gn == 0 ) return true;

	Planner_Stats&	stats = Planner_Stats::instance();

	MiniSAT solver( this, &o );
	stats.notify_entailment_test();
	if ( solver.solve() ) return false;
	return true;
}

}
