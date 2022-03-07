/*
  Alexandre Albore, Miquel Ramirez, Hector Geffner
  T1: A conformant planner 
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
#include "nff_bsearch.hxx"
#include "nff_options.hxx"
#include "nff_log.hxx"
#include "nff_ipc.hxx"
#include "utils.hxx"
#include "nff_planner_stats.hxx"
#include <cassert>

#define DEBUG_SEARCH 0


namespace NFF
{

PDDL::Task& BeliefSearch::sm_task = PDDL::Task::instance();
  
BeliefSearch::BeliefSearch()
	:  expanded(0), evaluated(0), m_failed (true), m_root (SearchNode::root()),
	m_bfs_closed(5113)
{
}

BeliefSearch::~BeliefSearch()
{
        delete heuristics;
}


std::vector<std::vector<int> >&        BeliefSearch::models()
{
        return m_models;
}

unsigned    BeliefSearch::ehc_get_node_h( SearchNode* n )
{
	NFF_Options& opt = NFF_Options::instance();
	return ( opt.ehc_heuristic() == OPT_EHC_USES_HX ? n->hX : n->hS );
}

void	BeliefSearch::ehc_get_helpful_actions( SearchNode* n, std::vector< PDDL::Operator* >& helpful )
{
	NFF_Options& opt = NFF_Options::instance();
	Operator_Vec& ops = ( opt.ehc_heuristic() == OPT_EHC_USES_HX ? 
				n->hX_HA() : n->hS_HA() );

	for ( unsigned i = 0; i < ops.size(); i++ )
		helpful.push_back( sm_task.prototype_ops()[ ops[i] ] );		
}

SearchNode* BeliefSearch::improve_heuristic( SearchNode* n )
{
	Planner_Stats&		stats = Planner_Stats::instance();	
	EHC_Sorted_Open_List	OPEN;
	Closed_List		CLOSED(5113);
	unsigned		h0 = ehc_get_node_h(n);
        CLOSED.add_element(n);
	stats.notify_expanded_ehc();	
	std::vector< PDDL::Operator* > po_vec;
	ehc_get_helpful_actions( n, po_vec );
	for (unsigned i = 0; i < po_vec.size(); i++)
	{
      		PDDL::Operator* o = po_vec[i];
		if ( n->b->can_apply(o) )
		{
			SearchNode* next = n->successor( o->get_idx() );
			evaluated++;
			OPEN.push( next );
			stats.notify_generated_ehc();
		}	
	}
	while ( !OPEN.empty() )
	{
		SearchNode* lss_n = OPEN.top();
		OPEN.pop();
                std::vector<std::vector<int> > lmodels = m_models;
		lss_n->do_belief_tracking( lmodels );
		if ( CLOSED.get_element(lss_n) != NULL )
		{
			// Already explored
			delete lss_n;
			continue;
		}
		lss_n->evaluate(heuristics, lmodels);
		if ( ehc_get_node_h(lss_n) == infty() )
		{
			// Dead end
			delete lss_n;
			continue;
		}
#ifdef DEBUG
		std::cout << "EHC expanding node:" << std::endl;
		lss_n->describe();
#endif
		if ( ehc_get_node_h(lss_n) < h0 )
		{
#ifdef DEBUG 
			std::cout << "Heuristic Improved!!!" << std::endl;
			lss_n->describe();
#endif
			// MRJ: Clean up OPEN & CLOSED
			while ( !OPEN.empty() )
			{
				SearchNode* tmp = OPEN.top();
				OPEN.pop();
				delete tmp;
			}
			SearchNode* tmp = lss_n->father;
			while ( tmp != n ) 
			{
				CLOSED.remove_element(tmp);
				assert( CLOSED.get_element(tmp) == NULL );
				tmp = tmp->father;
			}
			CLOSED.remove_element(n);
			assert( CLOSED.get_element(tmp) == NULL );
			return lss_n;
		}
		CLOSED.add_element(lss_n);
		stats.notify_expanded_ehc();
		std::vector< PDDL::Operator* > po_vec;
		ehc_get_helpful_actions( n, po_vec );
		for (unsigned i = 0; i < po_vec.size(); i++)
		{
			PDDL::Operator* o = po_vec[i];
	
			if ( lss_n->b->can_apply(o) )
			{
				SearchNode* next = lss_n->successor( o->get_idx() );
				OPEN.push( next );
				stats.notify_generated_ehc();
			}	
		}
	}
        if ( n->is_equal( *root(), models() ) )
		CLOSED.remove_element(n);

	return NULL;	
}

SearchNode* BeliefSearch::enforced_hill_climbing()
{

	SearchNode* n = root();

	while ( !n->b->entails_goal() )
	{
#ifdef DEBUG
		std::cout << "(START) EHC step:" << std::endl;
		report_memory_usage();
#endif
		std::cout << "EHC tries to improve heuristic: " << ehc_get_node_h(n) << std::endl;
		#ifdef DEBUG
		std::cout << "START NODE" << std::endl;
		n->describe();
		#endif

		n = improve_heuristic(n);
#ifdef DEBUG
		std::cout << "(END) EHC step:" << std::endl;
		report_memory_usage();
#endif

		if ( n == NULL )
			break;
		std::cout << "Success! new hS(n)=" << ehc_get_node_h(n) << " expanded: " << expanded << " evaluated: " << evaluated << std::endl;

	}

	return n;
}

SearchNode* BeliefSearch::best_first_search ( )
{
	NFF_Options& opt = NFF_Options::instance();
	Planner_Stats&		stats = Planner_Stats::instance();	
	Combined_Open_List OPEN;
	OPEN.push( root() );

	SearchNode* g = NULL;
        std::vector< std::vector<int> > models = m_models;

	while ( !OPEN.empty() )
	{
                std::vector<std::vector<int> > lmodels;
		SearchNode* n = OPEN.top();
                assert( n != NULL);// This shouldn't be happening
		OPEN.pop();
		stats.notify_expanded_bfs();

#ifdef DEBUG
		std::cout << "Node: Considering expansion" << std::endl;
		std::cout << "|OPEN|=" << OPEN.size() << " nodes" << std::endl;
		std::cout << "Expanded so far:" << expanded << std::endl;
		n->describe();
#endif
                lmodels = m_models;
                n->do_belief_tracking( lmodels );

		if ( in_closed(n) )
		{
			delete n;
			continue;
		}

		if ( goal_achieved(n) ) {
			std::cout << "Plan found!" << std::endl;
			g = n;
			break;
		}

                n->evaluate( heuristic(), lmodels );
                if ( n->hS == infty() )
                {
                        delete n;
                        continue;
                }

		close(n);
		for (unsigned i = 0; i < sm_task.prototype_ops().size(); i++)
	  	{
			PDDL::Operator* o = sm_task.prototype_ops()[i];

			if ( n->b->can_apply(o) )
			{
				SearchNode* next = n->successor( o->get_idx() );
/*
                                lmodels = m_models;
                                next->do_belief_tracking( lmodels );
				next->evaluate( heuristic(), lmodels );
*/ 
                                if ( opt.ehc_heuristic() == OPT_EHC_USES_HS )
                                {
                                        next->fn = n->hS;
                                        next->queue = (unsigned)hS_HA;
                                } else 
                                {
                                        next->fn = n->hX;
                                        next->queue = (unsigned)hX_HA ;
                                }  
				if ( next->fn == infty() )
				{
					delete next;
					continue;
                                }
				OPEN.push(next);
				stats.notify_generated_bfs();
			}	
		}
	}
	return g;
}


void BeliefSearch::make_initial_CNF( DNNF::CNF_Theory& initial )
{
	DNNF::Clause	c;

	/* Initially known fluents */
	for ( unsigned i = 0; i <  root()->b->known_vec().size(); i++)
	{
                unsigned f = root()->b->known_vec()[i];
                unsigned not_f = sm_task.not_equivalent( f );

                if ( sm_task.fluents()[ f ]->is_pos() ) 
                        c.insert( f );
                else if ( not_f ) 
                {
                        c.insert( -not_f );
                } else {
                        assert( 0 );
                        c.insert( f ); // NB: the latter should never appear
                }
		initial.add_clause( c );
		c.clear();
	}
	for ( unsigned i = 1; i < sm_task.fluents().size(); i++ )
	{
		if ( root()->b->known_set().isset(i)
			|| root()->b->unknown_set().isset(i) )
			continue;
                if ( sm_task.fluents()[ i ]->is_pos() ) 
                        c.insert( -i );
                else
                        c.insert( sm_task.not_equivalent( i ) );

		initial.add_clause(c);
		c.clear();
	} 

	for ( unsigned f = 1; f < sm_task.fluents().size()-1; f++ )
	{
		unsigned not_f = sm_task.not_equivalent( f );
		if ( not_f )
		{
			c.insert( f );
			c.insert( not_f );
			initial.add_clause(c);
			c.clear();
			c.insert( -f );
			c.insert( -not_f );
			initial.add_clause(c);
			c.clear();
		}	
	}

 
	// insert initial clauses
	for ( unsigned i = 0; i < root()->b->clauses_vec().size(); i++ )
	{
		for ( std::vector<unsigned>::iterator j = root()->b->clauses_vec()[i].begin(); 
			j < root()->b->clauses_vec()[i].end(); j++  ) 
		{
			assert( *j != 0 );
                        if ( sm_task.fluents()[ *j ]->is_pos() ) 
                                c.insert( *j );
                        else
                                c.insert( -sm_task.not_equivalent( *j ) );
		}
	
		initial.add_clause( c );
		c.clear();
	}

	//AA. static clauses??
}


double BeliefSearch::compile( DNNF::CNF_Theory& cnf, DNNF::DNNF_Theory& dnnf )
{
	DNNF::CNF_DNNF_Compiler compiler;
	compiler.set_compiler_path( "libdnnf/c2d_linux" );
	compiler.compile( cnf, dnnf );
	std::cout << "Time spent compiling CNF into DNNF: " << compiler.total_time() << std::endl;
		
	return compiler.total_time();
}

double BeliefSearch::count_models( DNNF::CNF_Theory& cnf )
{
	DNNF::CNF_DNNF_Compiler compiler;
	compiler.set_compiler_path( "libdnnf/c2d_linux" );
	return compiler.count_models(cnf);
}

void	BeliefSearch::initialize_dnnf_lit_weights( DNNF::DNNF_Theory& ctau )
{
	for ( unsigned f = 1; f < (unsigned)sm_task.fluent_count(); f++ )
	{
		ctau.set_lw( f, 0 );
		ctau.set_lw( -f, 0 );
	}
}

void	BeliefSearch::decode_dnnf_model( char* dnnf_model, std::vector<int>& state )
{
	for ( unsigned f = 1; f < state.size(); f++ )
	{
		if ( !sm_task.fluents()[f]->is_pos() ) continue;
		int not_f = sm_task.not_equivalent(f);
		state[f] = std::sgn( dnnf_model[f] )*f;
		if ( not_f ) 
		{
			state[not_f] = -std::sgn( dnnf_model[f] )*not_f;
		}
	}	
}

bool	BeliefSearch::already_in_approx( std::vector<int>& state )
{
	for ( unsigned k = 0; k < models().size(); k++ )
	{
		std::vector<int>& b_state = models()[k];
		bool different = false;
		for ( unsigned i = 1; i < (unsigned)sm_task.fluent_count(); i++ )
			if ( std::sgn( b_state[i] ) != std::sgn( state[i] ) )
			{
				different = true;
				break;
			}

		if ( !different ) return true;
	}

	return false;
}

void BeliefSearch::print_model( std::vector<int> &model, DNNF::DNNF_Theory &initial_dnnf )
{

	for ( unsigned l = 1; l < model.size(); l++ ) 
	{
		unsigned f = std::abs( model[l]);
		int fsgn = std::sgn( model[l] );
		if ( sm_task.fluents()[f]->is_pos() )
		{
			std::cout << ( fsgn > 0 ? " " : " -" );
			sm_task.print_fluent(f, std::cout );
		       	std::cout << ", w = " <<  initial_dnnf.lw( f ) << std::endl;
		}
	  
	}
	std::cout << std::endl;
}

void	BeliefSearch::compute_relevant_prec_and_goals( Atom_Vec& r_fluents, PDDL::Fluent_Set& r_fluents_set )
{

        //AA: Builds up the set of lit relevant to a prec or a goal
	for ( int pi = 1; pi < sm_task.fluent_count()-1; pi++ )
        {
		if ( !sm_task.is_prec_or_goal(pi) )
			continue;

                for (unsigned i = 0; i < sm_task.relevant_to(pi).size(); i++)
                {
                        unsigned lit = sm_task.relevant_to(pi).at(i);
//                        unsigned not_lit = sm_task.not_equivalent( lit );

                        if ( r_fluents_set.isset( lit ) )	
                                continue;
                        r_fluents_set.set( lit );
                        r_fluents.push_back( lit );
                }
        }
}

void	BeliefSearch::set_lit_weights_to( DNNF::DNNF_Theory& t, Atom_Vec& L, int w )
{
       for ( unsigned r = 0; r < L.size(); r++ )
        {
                if ( sm_task.fluents()[ L[r]]->is_pos() )         
                        t.set_lw( L[r], w); 
                else
                {
                        int not_l = (int)sm_task.not_equivalent( L[r] );
                        if ( not_l )
                                t.set_lw( -not_l, w); 
                }
        } 	
}

        void	BeliefSearch::determine_problem_width( DNNF::DNNF_Theory& t, 
                                                       std::vector<bool>& fluent_width, 
                                                       std::vector<bool>& zero_width )
{
	Planner_Stats&	stats = Planner_Stats::instance();
	Belief&	b0 = *(root()->b);
	bool width_leq_1 = true;

	for ( int pi = 1; pi < sm_task.fluent_count()-1; pi++ )
	{
		if ( !sm_task.is_prec_or_goal(pi) )
			continue;

                PDDL::Fluent_Set pi_relevant_set = ( sm_task.fluent_count()+1 );
		Atom_Vec&        Rpi = sm_task.relevant_to(pi);
		Atom_Vec	 unknown_Rpi;

#ifdef DEBUG
		std::cout << "Checking width( ";
		sm_task.print_fluent( pi, std::cout );
		std::cout << ")" <<  std::endl;
#endif
		for ( unsigned j = 0; j < Rpi.size(); j++ )
		{
			int rel = Rpi[j];	
			if ( b0.unknown_set().isset(rel) )
				unknown_Rpi.push_back( rel );
		}	

		if ( unknown_Rpi.empty() ) 
		{
			fluent_width[pi] = true;
                        zero_width[pi] = true;
			continue;
		}

		set_lit_weights_to( t, unknown_Rpi, 1);

		std::vector<int>	S_set;
		Atom_Vec		S_fluents;
		char* 	best_model;
		int	w = std::numeric_limits<int>::max();  

                best_model = t.best_model(w);

                if ( w == 0 )
		{
			fluent_width[pi] = true;
                        zero_width[pi] = true;
			set_lit_weights_to( t, unknown_Rpi, 0);
#ifdef DEBUG
                        std::cout << "Initial Model for ";
                        sm_task.print_fluent( pi, std::cout );
                        std::cout << " - rank = " << w << std::endl;
#endif
			continue;
		}

                zero_width[pi] = false;

                for ( unsigned j = 0; j < unknown_Rpi.size(); j++ )
		{
			int	rel = unknown_Rpi[j];
			int	not_rel = sm_task.not_equivalent( rel );
	             
                        if ( sm_task.fluents()[ rel ]->is_pos()  )
			{
                                best_model = t.best_model_given_lit( rel, w );

#ifdef DEBUG
				std::cout << "rank( I + ";
				sm_task.print_fluent( rel, std::cout );
				std::cout << ") = " << w << std::endl;
#endif
				if ( w == 1 ) 
				{
					S_set.push_back( -rel );
					S_fluents.push_back( unknown_Rpi[j] );
				}
			}
                        else if ( not_rel )
			{
                                best_model = t.best_model_given_lit( -not_rel, w );
#ifdef DEBUG
				std::cout << "rank( I + ~";
				sm_task.print_fluent( not_rel, std::cout );
				std::cout << ") = " << w << std::endl;
#endif
				if ( w == 1 ) 
				{
					S_set.push_back( not_rel );
					S_fluents.push_back( unknown_Rpi[j] );
				}
			}
		}		

		if ( S_set.empty() )
		{
			width_leq_1 = false;
			set_lit_weights_to( t, unknown_Rpi, 0);
#ifdef DEBUG
			std::cout << "|S| = 0, width(";
			sm_task.print_fluent( pi, std::cout );
			std::cout << ") > 1" << std::endl;
#endif
			continue;
		}

 		best_model = t.best_model_given_lits( S_set, w );

		if ( w != std::numeric_limits<int>::max() )
		{
			width_leq_1 = false;
			set_lit_weights_to( t, unknown_Rpi, 0 );
#ifdef DEBUG
			std::cout << "I doesn't entail OR " << std::endl;
			for ( unsigned k = 0; k < S_fluents.size(); k++ )
			{
				std::cout << "\t";
				sm_task.print_fluent( S_fluents[k], std::cout );
				std::cout << std::endl;
			}
#endif	
			continue;
		}	
		fluent_width[pi] = true;

		set_lit_weights_to( t, unknown_Rpi, 0);
        }

	if ( width_leq_1 )
	{
		std::cout << "Problem has WIDTH <= 1" << std::endl;
		stats.notify_width_1();
	}
	else
	{
		std::cout << "Problem has WIDTH > 1" << std::endl;
	}
}

double	BeliefSearch::approximate_initial_belief()
{
	NFF_Options&   opt   = NFF_Options::instance();
	Planner_Stats&	stats = Planner_Stats::instance();
	DNNF::CNF_Theory	initial;
	make_initial_CNF( initial );

        DNNF::DNNF_Theory	initial_dnnf;
        double compilation_time = compile( initial, initial_dnnf );

	if ( opt.only_initial_node() )
	{ 
		double mc = count_models( initial );
		stats.notify_b0_mc( mc );
	}

	initialize_dnnf_lit_weights(initial_dnnf);

	Atom_Vec preconds_and_goals_relevants;
	PDDL::Fluent_Set preconds_and_goals_relevant_set ( sm_task.fluent_count()+1 );
	compute_relevant_prec_and_goals( preconds_and_goals_relevants, preconds_and_goals_relevant_set );

        std::vector<bool>	fluent_width( sm_task.fluent_count()+1 ); //fluent_width: true if w(L) <= 1 
        std::vector<bool>	zero_width ( sm_task.fluent_count()+1 );  //zero_width: true if w(L) == 0 

	determine_problem_width(initial_dnnf, fluent_width, zero_width );

	int num_queries = 0;
	int w = std::numeric_limits<int>::max();  
        std::vector<int> lits_true_in_a_minimal_model = std::vector<int>(sm_task.fluent_count(), std::numeric_limits<int>::max()); 
        std::vector<bool> has_already_a_model( sm_task.fluent_count()+1 );

	std::vector<int> state;
	state.resize( sm_task.fluent_count() );

	Belief&	b0 = *(root()->b);

//	initialize_dnnf_lit_weights(initial_dnnf);
 
	for ( int pi = 1; pi < sm_task.fluent_count()-1; pi++ )
	{
		if ( !sm_task.is_prec_or_goal(pi) )	
			continue;

                PDDL::Fluent_Set pi_relevant_set = ( sm_task.fluent_count()+1 );
		Atom_Vec&        Rpi = sm_task.relevant_to(pi);
                char* best_model ;

                //AA: Sets w(L')= 10000 for L' -> pi
		for ( unsigned r = 0; r < Rpi.size(); r++ )
                {
                        if ( !b0.unknown_set().isset( Rpi[r] ) )
                                continue;

                        if (sm_task.fluents()[ Rpi[r] ]->is_pos()  )
                        {
                                        initial_dnnf.set_lw( Rpi[r], initial_dnnf.lw( Rpi[r]) + 10000);
                        }
                        else 
                        {
                                int not_r = (int)sm_task.not_equivalent( Rpi[r]);
                                if (not_r)
                                        initial_dnnf.set_lw( -not_r, 
                                                     initial_dnnf.lw( -not_r ) + 10000 ) ;
                        }
                        pi_relevant_set.set(  Rpi[r] );
                }

                /** Models for literals with width = 0 **/
                if ( zero_width[ pi ] ) 
                {              
                        if ( has_already_a_model[pi] )
                        {
#ifdef DEBUG
                                sm_task.print_fluent( pi, std::cout);
                                std::cout << " has already an usable model." << std::endl;
#endif
                                for ( unsigned r = 0; r < Rpi.size(); r++ ) 
                                {                  
                                        if ( !(b0.unknown_set().isset( Rpi[r] )) )
                                                continue;
                                        int lit;
                                        if ( sm_task.fluents()[ Rpi[r] ]->is_pos() )
                                                lit = (int)Rpi[r];
                                        else if ( sm_task.not_equivalent( Rpi[r]) )
                                                lit = -(int)sm_task.not_equivalent( Rpi[r] );
                                        else assert( false );
                                        
                                        assert( initial_dnnf.lw( lit ) - 10000 >= 0 );
                                        initial_dnnf.set_lw( lit, initial_dnnf.lw( lit ) - 10000 );
                                }
                                continue;
                        }

                        for ( int pi_0 = 1; pi_0 < sm_task.fluent_count()-1; pi_0++ )
                        {
                                if ( !sm_task.is_prec_or_goal(pi_0) )	
                                        continue;
                                if ( !zero_width[ pi_0 ] )
                                        continue;
                                if ( pi_0 == pi )
                                        continue;
                                Atom_Vec&  Rpi_0 = sm_task.relevant_to(pi_0);
                                
                                //AA: Sets w(L')= 1 for L' -> pi_0
                                for ( unsigned r = 0; r < Rpi_0.size(); r++ )
                                {
                                        unsigned ll = Rpi_0[r];
                                        if (sm_task.fluents()[ ll ]->is_pos()  )
                                                initial_dnnf.set_lw( ll, initial_dnnf.lw( ll ) + 1);
                                        else if ( sm_task.not_equivalent( ll ) )
                                                initial_dnnf.set_lw( -(int)sm_task.not_equivalent( ll ), 
                                                                     initial_dnnf.lw( -(int)sm_task.not_equivalent(ll)) + 1 ) ;
                                        
                                        pi_relevant_set.set( ll );
                                }
                        }

                        best_model = initial_dnnf.best_model(w);

                        num_queries++;
#ifdef DEBUG
                        std::cout << "First model with weight: " << w << std::endl;

//                        if (w < 10000) ...             

                        std::cout << "Width of ";
                        sm_task.print_fluent( pi, std::cout );
                        std::cout << " is zero" << std::endl;
#endif

                        decode_dnnf_model( best_model, state );
#ifdef DEBUG
                        if ( w > 10000)
                                print_model( state , initial_dnnf );
                        assert ( w < 10000 );
#endif
                        if ( already_in_approx( state ) )
                        {
#ifdef DEBUG
                                std::cout << "\t This model was already retrieved!" << std::endl;
#endif
                                for ( unsigned r = 0; r < Rpi.size(); r++ ) 
                                {                  
                                        if ( !(b0.unknown_set().isset( Rpi[r] )) )
                                                continue;
                                        int lit;
                                        if ( sm_task.fluents()[ Rpi[r] ]->is_pos() )
                                                lit = (int)Rpi[r];
                                        else if ( sm_task.not_equivalent( Rpi[r]) )
                                                lit = -(int)sm_task.not_equivalent( Rpi[r]);
                                        
                                        assert( initial_dnnf.lw( lit ) - 10000 >= 0 );
                                        initial_dnnf.set_lw( lit, initial_dnnf.lw( lit ) - 10000 );
                                }
                                continue;
                        }

                        models().push_back( state );

#ifdef DEBUG
                        print_model(models()[ models().size()-1 ], initial_dnnf );
                        assert ( w < 10000 );
#endif
                        for ( unsigned i = 0; i < state.size(); i++ )
                        {
                                if ( state[i] > 0)
                                {
                                        //AA: Increases True lits weight to obtain samples as different as possible.
                                       if ( preconds_and_goals_relevant_set.isset( state[i]) )
                                        {
                                                int nw = initial_dnnf.lw( state[i] ) + 1;
                                                initial_dnnf.set_lw( state[i], nw ); 
                                        }
                                } 
                                else if  ( zero_width[ abs(state[i]) ] )
                                {
                                        unsigned lit = abs(state[i]);
                                        if    (sm_task.is_prec_or_goal( lit ) )
                                        { // Checks if the model can be used for this literal too
                                                bool reusable_model = true;                                
                                                Atom_Vec&  rel_lit = sm_task.relevant_to( lit );
                                
                                                for ( unsigned ri = 0; ri < rel_lit.size(); ri++ )
                                                {
                                                        unsigned ll = rel_lit[ri];
                                                        if (sm_task.fluents()[ ll ]->is_pos() )
                                                        {
                                                                if ( state[ll] > 0 )
                                                                {
                                                                        reusable_model = false;
                                                                        break;
                                                                }
                                                        }
                                                        else if (  sm_task.not_equivalent( ll ) && 
                                                                   state[ sm_task.not_equivalent( ll ) ] < 0 )    
                                                        {
                                                                reusable_model = false;
                                                                break;
                                                        }
                                                }
                                                if ( reusable_model )
                                                {
                                                        // Saves the model also for lit.
                                                        has_already_a_model[ lit ] = true;
                                                }
                                        }	
                                        
                                }
                                
                                state[i] = 0;
                        }
                        for ( unsigned r = 0; r < Rpi.size(); r++ ) 
                        {                  
                                if ( !(b0.unknown_set().isset( Rpi[r] )) )
                                        continue;

                                int lit;
                                if ( sm_task.fluents()[ Rpi[r] ]->is_pos() )
                                        lit = (int)Rpi[r];
                                else if ( sm_task.not_equivalent( Rpi[r]) )
                                        lit = -(int)sm_task.not_equivalent( Rpi[r]);
                                                         
                                assert( initial_dnnf.lw( lit ) - 10000 >= 0 );
                                initial_dnnf.set_lw( lit, initial_dnnf.lw( lit ) - 10000 );
                        }

                        continue;
                        
                }
                /** Models for literals with width > 0 **/

                //AA: Sets w(L'')= 1 for L'' -> Prec + Goal (different from pi)
                for ( unsigned r = 0; r < preconds_and_goals_relevants.size(); r++ )
                {
                        //AA: Relevant to current pi
                        if ( pi_relevant_set.isset( preconds_and_goals_relevants[r] ) )
                                continue;
                        //AA: weigth already raised
                        if (  initial_dnnf.lw( preconds_and_goals_relevants[r] ) > 1 )
                                continue;
                        if ( sm_task.fluents()[ preconds_and_goals_relevants[r]]->is_pos() )         
                                initial_dnnf.set_lw( preconds_and_goals_relevants[r], 1); 
                        else
                        {
                                int not_l = (int)sm_task.not_equivalent( preconds_and_goals_relevants[r] );
                                if ( not_l )
                                        initial_dnnf.set_lw( -not_l, 1 ); 
                        }
                }
                // sL = lits relevant to pi and initially unknown
                PDDL::Fluent_Set sL_set ( sm_task.fluent_count()+1 );
                std::vector<unsigned> sL_vec, relevant_lits;
		for ( unsigned j = 0; j < Rpi.size(); j++ )
		{
			int rel = Rpi[j];
			if ( b0.unknown_set().isset(rel) )
			{
                                sL_set.set( rel );
                                sL_vec.push_back( rel );
			}
                }

                // search for a smaller clause subset of sL
                int best_clause = -1;
                unsigned best_sz = sL_vec.size();
                for (unsigned j = 0; j < sm_task.initial_clauses().size(); j++)
                {
                        bool is_subset = true;
                        std::vector <unsigned> clause = sm_task.initial_clauses().at(j);
                        for ( unsigned cl = 0; cl < clause.size(); cl++ )
                               if ( !sL_set.isset( clause[ cl ] ) )
                                {
                                        is_subset = false;
                                        break;
                                }

                        if (is_subset && clause.size() < best_sz)
                        {
                                best_clause = j;
                                best_sz = clause.size();
                        }
                }
                if (best_clause > -1)
                {
#ifdef DEBUG
                        std::cout << "We've got a better clause: " << std::endl;
                        std::vector <unsigned> bclause = sm_task.initial_clauses().at( best_clause );

                        for (unsigned j = 0; j < bclause.size(); j++)
                        {
                                sm_task.print_fluent( bclause[j] );
                                if ( j != bclause.size()-1)
                                        std::cout << ", ";
                        }
                        std::cout << std::endl;
#endif
                        relevant_lits = sm_task.initial_clauses().at( best_clause );
                }
                else
                        relevant_lits = sL_vec;


                // AA: Getting the model I + L'
		for ( unsigned j = 0; j < relevant_lits.size(); j++ )
		{
                        char* best_model;
			int rel = relevant_lits[j];
                        int not_rel = sm_task.not_equivalent( rel );
			assert( sL_set.isset(rel) );

                        if ( sm_task.fluents()[ rel ]->is_pos()  )
                                best_model = initial_dnnf.best_model_given_lit( rel, w );
                        else if ( not_rel )
                                best_model = initial_dnnf.best_model_given_lit( -not_rel, w );

                        num_queries++;

#ifdef DEBUG
                        std::cout << "Getting model for I conjoined with ";
                        sm_task.print_fluent(rel, std::cout);
                        std::cout << " :found model with cost: " << w << std::endl;
#endif

                        //AA Optimization: removes samples with rank > 1 for lits of width = 1
                        //AA: This optimization is buggy. Recheck the theory.
//                        if ( fluent_width[ pi ] && w >= 20000 )
//                                continue;

                        // MRJ: Check that we found a model (i.e. with non infty cost)
                        if ( w == std::numeric_limits<int>::max() )
			{
				std::cerr << "Unreachable precondition or goal relevant atom: ";
				sm_task.print_fluent( rel, std::cerr );
				std::cerr << std::endl;
                                continue;
			}
                        // AA: Check if there is not already a minimal model for rel.
			if ( w >= lits_true_in_a_minimal_model[ rel ] )
                        {
#ifdef DEBUG
                                std::cout << "  We already have a minimal model for fluent ";
                                sm_task.print_fluent(rel, std::cout);
                                std::cout << " with cost: " << lits_true_in_a_minimal_model[ rel ] << std::endl;
#endif
                                continue;
                        }
			decode_dnnf_model( best_model, state );
			if ( already_in_approx( state) )
			{
#ifdef DEBUG
				std::cout << "\t This model was already retrieved!" << std::endl;
#endif
			}
			else
			{
				models().push_back( state );
#ifdef DEBUG
                                print_model( state, initial_dnnf);
#endif
                                for ( unsigned i = 0; i < state.size(); i++ )
                                {
                                        //AA: Increases True lits weight to obtain samples as different as possible.

                                        if ( (state[i] > 0) 
                                             && preconds_and_goals_relevant_set.isset(state[i]) )
                                        {
                                                int nw = initial_dnnf.lw( state[i] ) + 1;
                                                initial_dnnf.set_lw( state[i], nw ); 
                                                if ( lits_true_in_a_minimal_model[ state[i] ] > w )
                                                        lits_true_in_a_minimal_model[ state[i] ] = w;
                                        }

                                        state[i] = 0;
                                }
                                
			}
                }
                
		for ( unsigned r = 0; r < Rpi.size(); r++ ) 
                {
                        if ( !(b0.unknown_set().isset( Rpi[r] )) )
                                continue;

                        int lit;
                        if ( sm_task.fluents()[ Rpi[r] ]->is_pos() )
                                lit = (int)Rpi[r];
                        else if ( sm_task.not_equivalent( Rpi[r]) )
                                lit = -sm_task.not_equivalent( Rpi[r]);

                        assert( initial_dnnf.lw( lit ) - 10000 >= 0 );
                        initial_dnnf.set_lw( lit, initial_dnnf.lw( lit ) - 10000 );
                }
		/*	
		for ( unsigned f = 1; f < sm_task.fluent_count(); f++ )
			lits_true_in_a_minimal_model[f] = std::numeric_limits<int>::max();
		*/
        }


        if ( !stats.is_width_1() )
        {
/*
  una manera de generar un numero de samples N,  ej 100 es:

  0 comenzando con weights 1 en todos los lits relevantes a algun prec/goal
  1 sample: min model of I and keep it as sample if it's new (not repeated)
  2 incrementar el peso w de los true lits in previous sample by 1
  3 go back to 1 .. until N diff samples generated
*/
                const unsigned max_samples = 20;
                initialize_dnnf_lit_weights(initial_dnnf);
                                
                for ( unsigned r = 0; r < preconds_and_goals_relevants.size(); r++ )
                {
                        if ( sm_task.fluents()[ preconds_and_goals_relevants[r]]->is_pos() )         
                                initial_dnnf.set_lw( preconds_and_goals_relevants[r], 1); 
                        else
                        {
                                int not_l = (int)sm_task.not_equivalent( preconds_and_goals_relevants[r] );
                                if ( not_l )
                                        initial_dnnf.set_lw( -not_l, 1); 
                        }
                }
                                
                for ( unsigned k = 0; k < models().size(); k++ )
                        m_models.erase(models().begin(), models().end());
                
                while ( models().size() < max_samples ) {
                        char* best_model = initial_dnnf.best_model(w);
                                
                        num_queries++;
#ifdef DEBUG
                        std::cout << "w > 1 First model with weight: " << w << std::endl;
#endif
                        decode_dnnf_model( best_model, state );
                        if ( already_in_approx( state) )
                        {
#ifdef DEBUG
                                std::cout << "\t w > 1 This model was already retrieved!" << std::endl;
#endif 
                                break;
                        }
                        models().push_back( state );

#ifdef DEBUG
                        print_model(models()[models().size()-1 ], initial_dnnf );
#endif

                        for ( unsigned i = 0; i < state.size(); i++ )
                        {
                                //AA: Increases True lits weight to obtain samples as different as possible.
                                if ( (state[i] > 0) && preconds_and_goals_relevant_set.isset(state[i]) )
                                {
                                        int nw = initial_dnnf.lw( state[i] ) + 1;
                                        initial_dnnf.set_lw( state[i], nw ); 
                                        lits_true_in_a_minimal_model[ state[i] ] = w;  
                                }
                                
                                state[i] = 0;
                        }                       
                }
        }

	return compilation_time;
 }
        

SearchNode*	BeliefSearch::ff_search()
{
	
	NFF::Log& log = NFF::Log::instance();
	std::cout << "EHC starts!" << std::endl;
	SearchNode* g = enforced_hill_climbing();
	if ( g == NULL )
	{
		log.stream() << ";; ehc_failed=True" << std::endl;
		std::cout << "EHC failed! doing BFS" << std::endl;
		g = best_first_search();
	}
	else
		log.stream() << ";; ehc_failed=False" << std::endl;
	return g;
}	

SearchNode*	BeliefSearch::dual_queue_get_node_from_open()
{
	NFF_Options&		opt = NFF_Options::instance();

#ifdef DEBUG
	std::cout << "Queues status:" << std::endl;
	std::cout << "\tmin f(n) for hS: " << m_min_hS_fn << std::endl;
	std::cout << "\t|Open(HA_{hS})|=" << m_open_set[hS_HA].size() << std::endl;
	std::cout << "\tmin f(n) for hX: " << m_min_hX_fn << std::endl;
	std::cout << "\t|Open(HA_{hX})|=" << m_open_set[hX_HA].size() << std::endl;
	std::cout << "\tNon-helpful" << std::endl;
	std::cout << "\t|Open(not HA)|=" << m_open_set[not_HA].size() << std::endl;

#endif

	if ( m_open_counters[m_selected_open] > 0 )
	{
		unsigned next_queue = ( m_selected_open == hS_HA ? hX_HA : hS_HA);
		if ( m_open_set[m_selected_open].empty() )
		{
			if ( m_open_set[next_queue].empty() ) 
			{
				SearchNode* n = m_open_set[not_HA].top();
				m_open_set[not_HA].pop();
				return n;
			}
			SearchNode* n = m_open_set[next_queue].top();
			m_open_set[next_queue].pop();
			return n;		
		}
		SearchNode* n = m_open_set[m_selected_open].top();
		m_open_set[m_selected_open].pop();
		m_selected_open = next_queue;
		return n;	
	}

	if ( m_open_set[not_HA].empty() )
	{
		unsigned next_queue = ( m_selected_open == hS_HA ? hX_HA : hS_HA);	
		if ( m_open_set[m_selected_open].empty() )
		{
			if ( m_open_set[next_queue].empty() ) return NULL;
			SearchNode* n = m_open_set[next_queue].top();
			m_open_set[next_queue].pop();
			return n;
		}
		SearchNode* n = m_open_set[m_selected_open].top();
		m_open_set[m_selected_open].pop();
		m_selected_open = next_queue;
		return n;	
	}

	SearchNode* n = m_open_set[not_HA].top();
	m_open_set[not_HA].pop();
	m_open_counters[hS_HA] = m_open_counters[hX_HA] = opt.helpful_ratio();
	return n;
}

SearchNode*	BeliefSearch::multi_queue_get_node_from_open()
{
	NFF_Options&		opt = NFF_Options::instance();
	unsigned		primary_idx, secondary_idx;

#ifdef DEBUG
	std::cout << "Queues status:" << std::endl;
	std::cout << "\tmin f(n) for hS: " << m_min_hS_fn << std::endl;
	std::cout << "\thS score: " << m_hS_score << std::endl;
	std::cout << "\t|Open(HA_{hS})|=" << m_open_set[hS_HA].size() << std::endl;
	std::cout << "\tmin f(n) for hX: " << m_min_hX_fn << std::endl;
	std::cout << "\thX score: " << m_hX_score << std::endl;
	std::cout << "\t|Open(HA_{hX})|=" << m_open_set[hX_HA].size() << std::endl;
	std::cout << "\tNon-helpful" << std::endl;
	std::cout << "\t|Open(not HA)|=" << m_open_set[not_HA].size() << std::endl;

#endif

	if ( m_hS_score >= m_hX_score )
	{
		if ( !m_open_set[hS_HA].empty() )
		{
			m_selected_open = primary_idx = (unsigned)hS_HA;
			m_hS_score--;	
		}
		else
		{
			m_selected_open = primary_idx = (unsigned)hX_HA;
			m_hX_score--;
		}
	}
	else
	{
		if ( !m_open_set[hX_HA].empty() )
		{
			m_selected_open = primary_idx = (unsigned)hX_HA;
			m_hX_score--;
		}
		else
		{
			m_selected_open = primary_idx = (unsigned)hS_HA;
			m_hS_score--;
		}
	}
	Sorted_Open_List& primary_open = m_open_set[primary_idx];
	Sorted_Open_List& secondary_open = m_open_set[not_HA];

	if ( !primary_open.empty() )
	{
		if ( m_open_counters[primary_idx] == 0 ) // Take from secondary
		{
			m_open_counters[primary_idx] = opt.helpful_ratio();
			if ( !secondary_open.empty() )
			{
				SearchNode* n = secondary_open.top();
				secondary_open.pop();
				m_selected_open = not_HA;
				return n;
			}
		}
		m_open_counters[primary_idx]--;
		SearchNode* n = primary_open.top();
		primary_open.pop();
                return n;
	}

	if ( !secondary_open.empty() )
	{
		SearchNode* n = secondary_open.top();
		secondary_open.pop();
		m_selected_open = not_HA;
		return n;
	}

	return NULL;
}

void BeliefSearch::print_the_queue( unsigned h )
{
        std::vector< SearchNode* > queue_;
        unsigned i = 0;

       while ( m_open_set[h].size() > 0)
        {
                std::cout << "Node #"<< i++ << "  ";
                (m_open_set[h].top())->describe();
                queue_.push_back( m_open_set[h].top() );
                m_open_set[h].pop();
        }

       for (i = 0; i < queue_.size(); i++)
               m_open_set[h].push( queue_[i] );

}

void	BeliefSearch::dual_queue_expand_and_evaluate( SearchNode* n )
{
	Planner_Stats&		stats = Planner_Stats::instance();
        bool plan_found = false;
        std::vector< std::vector<int> > lmodels;
#ifdef DEBUG
	std::cout << "Generating successors for: " << std::endl;
	n->describe();
#endif

	for (unsigned i = 0; i < sm_task.prototype_ops().size(); i++)
	{
		PDDL::Operator* o = sm_task.prototype_ops()[i];
                lmodels = m_models;

		if ( n->b->can_apply(o) )
		{
			bool is_hS_HA = n->is_hS_HA(i);
			bool is_hX_HA = n->is_hX_HA(i);

			if ( is_hS_HA || is_hX_HA )
			{
#ifdef DEBUG
				std::cout << "Operator ";
				sm_task.print_operator(o, std::cout);
				std::cout << " is HA for hS and HA for hX" << std::endl;
#endif	
                                SearchNode* next = n->successor( o->get_idx() );
                                next->queue = (unsigned)hS_HA;

                                next->do_belief_tracking( lmodels );

                                if ( in_closed( next ) )
                                {
                                        delete next;
                                        continue;
                                }
                                if ( goal_achieved(next) )
                                {
                                        std::cout << "Plan found!" << std::endl;
                                        plan_found = true;
                                }
                                next->evaluate( heuristic(), lmodels );
                                if ( next->hS == infty() )
                                {
                                        delete next;
                                        continue;
                                }


                                m_open_set[hS_HA].push( next );
                                assert(next->queue == 0);
                                next->fn = n->hS;
                                stats.notify_generated_bfs();
                                stats.add_node_size( next->size() );

                                SearchNode* next2 = n->successor( o->get_idx() );
                                lmodels = m_models;
                                next2->do_belief_tracking( lmodels );

                                if ( in_closed( next2 ) )
                                {
                                        delete next2;
                                        continue;
                                }
                                if ( goal_achieved(next2) )
                                {
                                        std::cout << "Plan found!" << std::endl;
                                        plan_found = true;
                                }
                                next2->evaluate( heuristic(), lmodels );
                                if ( next2->hS == infty() )
                                {
                                        delete next2;
                                        continue;
                                }
                                next2->queue = (unsigned)hX_HA;
                                next2->fn = n->hX;
                                m_open_set[hX_HA].push( next2 );
                                stats.notify_generated_bfs();
                                stats.add_node_size( next2->size() );
                                if (plan_found)
                                        return;
                        }
                }
                else
                {
#ifdef DEBUG
                        std::cout << "Operator ";
                        sm_task.print_operator(o, std::cout);
                        std::cout << " is not HA for hS neither HA for hX" << std::endl;
#endif	
                        SearchNode* next = n->successor( o->get_idx() );
                        next->do_belief_tracking( lmodels );
                        
                        if ( in_closed( next ) )
                        {
                                delete next;
                                continue;
                        }
                        if ( goal_achieved(next) )
                        {
                                std::cout << "Plan found!" << std::endl;
                                plan_found = true;
                        }
                        next->evaluate( heuristic(), lmodels );
                        if ( next->hS == infty() )
                        {
                                delete next;
                                continue;
                        }
                        next->queue = not_HA;
                        next->fn = next->hS;
                        
                        m_open_set[not_HA].push( next );
                        stats.notify_generated_bfs();
                        stats.add_node_size( next->size() );
                        if (plan_found)
                                return;
                }
        }	
        
#ifdef DEBUG
        std::cout << "Queues after expansion" << std::endl;
        if ( !m_open_set[hS_HA].empty() )
                std::cout << "min f_hS(n) = " << m_open_set[hS_HA].top()->fn << "," << m_open_set[hS_HA].top()->hX << std::endl;
        if ( !m_open_set[hX_HA].empty() )
                std::cout << "min f_hX(n) = " << m_open_set[hX_HA].top()->fn << "," << m_open_set[hX_HA].top()->hS  << std::endl;
        if ( !m_open_set[not_HA].empty() )
                std::cout << "min f_~HA(n) = " << m_open_set[not_HA].top()->fn << std::endl;
#endif
        
}

void	BeliefSearch::dual_queue_expand( SearchNode* n )
{
	Planner_Stats&		stats = Planner_Stats::instance();
        
#ifdef DEBUG
	std::cout << "Generating successors for: " << std::endl;
	n->describe();
#endif
        
	for (unsigned i = 0; i < sm_task.prototype_ops().size(); i++)
	{
		PDDL::Operator* o = sm_task.prototype_ops()[i];
                
		if ( n->b->can_apply(o) )
		{
			bool is_hS_HA = n->is_hS_HA(i);
			bool is_hX_HA = n->is_hX_HA(i);
			if ( is_hS_HA || is_hX_HA )
			{
#ifdef DEBUG
				std::cout << "Operator ";
				sm_task.print_operator(o, std::cout);
				std::cout << " is HA for hS and HA for hX" << std::endl;
#endif	
                                SearchNode* next = n->successor( o->get_idx() );
                                next->queue = (unsigned)hS_HA;
                                next->fn = n->hS;
                                next->hX = n->hX;
                                next->tb = n->tb;
                                                               
                                m_open_set[hS_HA].push( next );
                                assert(next->queue == 0);
                                stats.notify_generated_bfs();
                                stats.add_node_size( next->size() );
                                
                                next = n->successor( o->get_idx() );
                                next->queue = (unsigned)hX_HA;
                                next->fn = n->hX;
                                next->hS = n->hS;
                                next->tb = n->tb;
                                m_open_set[hX_HA].push( next );
                                stats.notify_generated_bfs();
                                stats.add_node_size( next->size() );
                        }
                        else
                        {
#ifdef DEBUG
                                std::cout << "Operator ";
                                sm_task.print_operator(o, std::cout);
                                std::cout << " is not HA for hS neither HA for hX" << std::endl;
#endif	
                                SearchNode* next = n->successor( o->get_idx() );
                                next->queue = not_HA;
                                next->fn = n->hS;
                                next->hX = n->hX;
                                next->tb = n->tb;
                                m_open_set[not_HA].push( next );
                                stats.notify_generated_bfs();
                                stats.add_node_size( next->size() );

                        }
                }

                // Maintaining heap size
                if ( 0 && MAX_MEMORY < stats.node_megabytes() )
                {
 //                       unsigned delta =  m_open_set[hS_HA].size() - last_heap_size[hS_HA] +
//                                 m_open_set[hX_HA].size() - last_heap_size[hX_HA] +
//                                 m_open_set[not_HA].size() - last_heap_size[not_HA] ;
                        // UNCOMMENT in case of minmaxheap                
//                         m_open_set[hS_HA].maintain_size( last_heap_size[hS_HA] );
//                         m_open_set[hX_HA].maintain_size( last_heap_size[hX_HA] );
//                         m_open_set[not_HA].maintain_size( last_heap_size[not_HA] );
                        assert (stats.node_megabytes() > 0);
                }
                
                last_heap_size[hS_HA] =  m_open_set[hS_HA].size();
                last_heap_size[hX_HA] =  m_open_set[hX_HA].size();
                last_heap_size[not_HA] =  m_open_set[not_HA].size();
                
        } // end (for operators)	
        
         
//        print_the_queue(hS_HA);
        
          
#ifdef DEBUG
        std::cout << "Queues after expansion" << std::endl;
        if ( !m_open_set[hS_HA].empty() )
                std::cout << "min f_hS(n) = " << m_open_set[hS_HA].top()->fn << "," << m_open_set[hS_HA].top()->hX << std::endl;
        if ( !m_open_set[hX_HA].empty() )
                std::cout << "min f_hX(n) = " << m_open_set[hX_HA].top()->fn << "," << m_open_set[hX_HA].top()->hS  << std::endl;
        if ( !m_open_set[not_HA].empty() )
                std::cout << "min f_~HA(n) = " << m_open_set[not_HA].top()->fn << std::endl;
#endif
        
}


void	BeliefSearch::multi_queue_expand( SearchNode* n )
{
	Planner_Stats&		stats = Planner_Stats::instance();

#ifdef DEBUG
	std::cout << "Generating successors for: " << std::endl;
	n->describe();
#endif

	for (unsigned i = 0; i < sm_task.prototype_ops().size(); i++)
	{
		PDDL::Operator* o = sm_task.prototype_ops()[i];

		if ( n->b->can_apply(o) )
		{
			bool is_hS_HA = n->is_hS_HA(i);
			bool is_hX_HA = n->is_hX_HA(i);
			if ( is_hS_HA && is_hX_HA )
			{
#ifdef DEBUG
				std::cout << "Operator ";
				sm_task.print_operator(o, std::cout);
				std::cout << " is HA for hS and HA for hX" << std::endl;
#endif	
				SearchNode* next = n->successor( o->get_idx() );
				next->queue = (unsigned)hS_HA;
				next->fn = n->hS;
				m_open_set[hS_HA].push( next );
				stats.notify_generated_bfs();
				next = n->successor( o->get_idx() );
				next->queue = (unsigned)hX_HA;
				next->fn = n->hX;
				m_open_set[hX_HA].push( next );
				stats.notify_generated_bfs();
				stats.add_node_size( next->size() );
			}
			else if ( is_hS_HA && !is_hX_HA  )
			{
#ifdef DEBUG
				std::cout << "Operator ";
				sm_task.print_operator(o, std::cout);
				std::cout << " is HA for hS but not HA for hX" << std::endl;
#endif	

				SearchNode* next = n->successor( o->get_idx() );
				next->queue = hS_HA;
				next->fn = n->hS;
				m_open_set[hS_HA].push( next );
				stats.notify_generated_bfs();
				stats.add_node_size( next->size() );
			}
			else if ( !is_hS_HA && is_hX_HA  )
			{
#ifdef DEBUG
				std::cout << "Operator ";
				sm_task.print_operator(o, std::cout);
				std::cout << " is not HA for hS but is HA for hX" << std::endl;
#endif	

				SearchNode* next = n->successor( o->get_idx() );
				next->queue = hX_HA;
				next->fn = n->hX;
				m_open_set[hX_HA].push( next );
				stats.notify_generated_bfs();
				stats.add_node_size( next->size() );
			}
			else
			{
#ifdef DEBUG
				std::cout << "Operator ";
				sm_task.print_operator(o, std::cout);
				std::cout << " is not HA for hS neither HA for hX" << std::endl;
#endif	
				if ( n->queue != not_HA ) 
				{
					SearchNode* next = n->successor( o->get_idx() );
					next->queue = not_HA;
					next->fn = n->hS;
					m_open_set[not_HA].push( next );
					stats.notify_generated_bfs();
					stats.add_node_size( next->size() );
				}
			}
		}	
	}
}

void		BeliefSearch::initialize_multi_queue_search()
{
	NFF_Options&	opt = NFF_Options::instance();

	m_open_set.resize(3);
        last_heap_size.resize(3);
	m_open_counters.resize(3);
	m_open_counters[hS_HA] = opt.helpful_ratio();
	m_open_counters[hX_HA] = opt.helpful_ratio();
	m_open_counters[not_HA] = 1;
	
	m_hS_score = 1;
	m_min_hS_fn = root()->hS;
	m_hX_score = 1;
	m_min_hX_fn = root()->hX;
	m_selected_open = hS_HA;
}

void		BeliefSearch::report_queues_state()
{
//         if ( !fake_open_set.empty() )
//         {
//                 std::cout << "max fake_hS(n) = " << fake_open_set.findMax()->fn ;
//                 std::cout << "\nnodes in MaxMinHeap ";
//                 fake_open_set.printRaw(std::cout);
// 		std::cout << ", hX(n)=" << fake_open_set.findMax()->hX;
// 		std::cout << ", g(n)=" << fake_open_set.findMax()->gn;
// 		std::cout << ", tb(n)=" << fake_open_set.findMax()->tb << std::endl;

//                 std::cout << "min fake_hS(n) = " << fake_open_set.findMin()->fn ;
// 		std::cout << ", hX(n)=" << fake_open_set.findMin()->hX;
// 		std::cout << ", g(n)=" << fake_open_set.findMin()->gn;
// 		std::cout << ", tb(n)=" << fake_open_set.findMin()->tb << std::endl;
//         }
	if ( !m_open_set[hS_HA].empty() )
	{
		std::cout << "min f_hS(n) = " << m_open_set[hS_HA].top()->fn;
		std::cout << ", hX(n)=" << m_open_set[hS_HA].top()->hX;
		std::cout << ", g(n)=" << m_open_set[hS_HA].top()->gn;
		std::cout << ", tb(n)=" << m_open_set[hS_HA].top()->tb << std::endl;
	}
	if ( !m_open_set[hX_HA].empty() )
	{
		std::cout << "min f_hX(n) = " << m_open_set[hX_HA].top()->fn;
		std::cout << ", hX(n)=" << m_open_set[hX_HA].top()->hS;
		std::cout << ", g(n)=" << m_open_set[hX_HA].top()->gn;
		std::cout << ", tb(n)=" << m_open_set[hX_HA].top()->tb << std::endl;
	}
	if ( !m_open_set[not_HA].empty() )
	{
		std::cout << "min f_hNOT(n) = " << m_open_set[not_HA].top()->fn;
		std::cout << ", hX(n)=" << m_open_set[not_HA].top()->hX;
		std::cout << ", g(n)=" << m_open_set[not_HA].top()->gn;
		std::cout << ", tb(n)=" << m_open_set[not_HA].top()->tb << std::endl;
	}

}

SearchNode*    BeliefSearch::dual_queue_best_first_search()
{
	Planner_Stats&		stats = Planner_Stats::instance();	
	NFF_Options&		opt = NFF_Options::instance();
        std::vector<std::vector<int> > lmodels;

	initialize_multi_queue_search();	

        if ( !NO_SAT_CALLS || stats.is_width_1() ) 
                close(root());

//        std::cout << "Initial Node size: " << root()->size() << std::endl;

	if ( EVAL_ROOT_CHILDS ) dual_queue_expand_and_evaluate( root() );
        else dual_queue_expand( root() );

	while ( !search_space_exhausted(m_open_set) )
	{
		SearchNode*	n = dual_queue_get_node_from_open();
		assert( n != NULL ); // This shouldn't be happening
		stats.substract_node_size( n->size() );	

//                n->create_samples( models );

                lmodels = m_models;

                if (!EVAL_ROOT_CHILDS || (EVAL_ROOT_CHILDS && n->gn > 1))
                        n->do_belief_tracking( lmodels );


		if ( ( !NO_SAT_CALLS || stats.is_width_1() ) && in_closed( n ) )
		{
			delete n;
			continue;
		}
		if ( goal_achieved(n) )
		{
			std::cout << "Plan found!" << std::endl;
			return n;
		}

                if (!EVAL_ROOT_CHILDS || (EVAL_ROOT_CHILDS && n->gn > 1))
                        n->evaluate( heuristic(), lmodels );

		if ( n->hS == infty() )
		{
			delete n;
			continue;
		}
		if ( n->hS < m_min_hS_fn )
		{
			m_min_hS_fn = n->hS;
			m_hS_score += opt.improvement_reward();
#ifdef DEBUG
			std::cout << "hS improved: incrementing score: " << m_hS_score << std::endl;
#endif
		}
		if ( n->hX < m_min_hX_fn )
		{
			m_min_hX_fn = n->hX;
			m_hX_score += opt.improvement_reward();
#ifdef DEBUG
			std::cout << "hX improved: incrementing score: " << m_hS_score << std::endl;
#endif

		}
#ifdef DEBUG
		std::cout << "Considering expansion of node:" << std::endl;
		std::cout << "Queue: ";
		if ( n->queue == hS_HA ) std::cout << "hS_HA";
		else if ( n->queue == hX_HA ) std::cout << "hX_HA";
		else
			std::cout << "not_HA";
		std::cout << std::endl;
		n->describe();
		report_queues_state();
#endif
		stats.notify_expanded_bfs();
		if ( m_selected_open == hS_HA )
			stats.notify_primary_expansion();
		else if ( m_selected_open == hX_HA )
			stats.notify_secondary_expansion();
		else
			stats.notify_tertiary_expansion();

	        if ( !NO_SAT_CALLS || stats.is_width_1() ) 
                        close(n);


		dual_queue_expand(n);
#ifdef DEBUG
		std::cout << "Queues state after EXPANSION" << std::endl;
		report_queues_state();

                std::cout << "Expanded: " << stats.expanded_bfs() << std::endl;
                std::cout << "Generated: " << stats.generated_bfs() << std::endl;
		report_memory_usage();
                std::cout << std::endl;
#endif
	}

	return NULL;	
}



SearchNode*	BeliefSearch::multi_queue_best_first_search()
{
	Planner_Stats&		stats = Planner_Stats::instance();	
	NFF_Options&		opt = NFF_Options::instance();

	initialize_multi_queue_search();
        if ( !NO_SAT_CALLS || stats.is_width_1() ) 
		close(root());
        std::vector<std::vector<int> > lmodels;
	multi_queue_expand(root());

	while ( !search_space_exhausted(m_open_set) )
	{
		SearchNode*	n = multi_queue_get_node_from_open();
		assert( n != NULL ); // This shouldn't be happening

                lmodels = m_models;
		n->do_belief_tracking( lmodels );
		if ( ( !NO_SAT_CALLS || stats.is_width_1() )  && in_closed( n ) )
		{
			delete n;
			continue;
		}
		if ( goal_achieved(n) )
		{
			std::cout << "Plan found!" << std::endl;
			return n;
		}
		n->evaluate( heuristic(), lmodels );
		if ( n->hS == infty() )
		{
			delete n;
			continue;
		}
		if ( n->hS < m_min_hS_fn )
		{
			m_min_hS_fn = n->hS;
			m_hS_score += opt.improvement_reward();
		}
		if ( n->hX < m_min_hX_fn )
		{
			m_min_hX_fn = n->hX;
			m_hX_score += opt.improvement_reward();
		}
#ifdef DEBUG
		std::cout << "Considering expansion of node:" << std::endl;
		n->describe();
#endif
		stats.notify_expanded_bfs();
		if ( m_selected_open == hS_HA )
			stats.notify_primary_expansion();
		else if ( m_selected_open == hX_HA )
			stats.notify_secondary_expansion();
		else
			stats.notify_tertiary_expansion();
	        if ( !NO_SAT_CALLS || stats.is_width_1() ) 
                        close(n);
		multi_queue_expand(n);
	}

	return NULL;	
}

SearchNode* 	BeliefSearch::execute_search_strategy()
{
	NFF_Options& opt = NFF_Options::instance();
	if ( opt.search_strategy() == "ff" )
		return ff_search();
	if ( opt.search_strategy() == "ehc" )
		return enforced_hill_climbing();
	if ( opt.search_strategy() == "bfs" )
		return best_first_search();
	if ( opt.search_strategy() == "mqbfs" )
		return multi_queue_best_first_search();
	if ( opt.search_strategy() == "dqbfs" )
		return dual_queue_best_first_search();
	return NULL;
}

bool	BeliefSearch::solve()
{
        NFF_Options&        opts = NFF_Options::instance();
	NFF::Log&           log = NFF::Log::instance();
        NFF::Ipc&           ipc = NFF::Ipc::instance();
 	Planner_Stats&	    stats = Planner_Stats::instance();  
	SearchNode*         g = NULL;
	bool                plan_found = false;
        set_root( SearchNode::root() );	

	double t0 = time_used();
	double approx_time = approximate_initial_belief();

        if ( !NO_SAT_CALLS || stats.is_width_1() ) 
                root()->compute_hash( models() );	

	// MRJ: Finally we print the approximation so we can take a look at it
#ifdef DEBUG
	std::cout << "States in b' ( " << models().size() <<  " )  " << std::endl;	
	for ( unsigned k = 0; k < models().size(); k++ )
	{
		std::cout << "s_" << k << ": ";
		for ( unsigned l = 1; l < models()[k].size(); l++ ) 
		  {
                          unsigned f = std::abs(models()[k][l]);
                          int fsgn = std::sgn( models()[k][l] );
                          if ( sm_task.fluents()[f]->is_pos() )
                          {
                                  std::cout << ( fsgn > 0 ? " " : " -" );
                                  sm_task.print_fluent(f, std::cout );
                                  std::cout << std::endl;
                          }
		  }
		std::cout << std::endl;
	}
#endif

	heuristics =  new Dijkstra(models().size());

#ifdef DEBUG
	heuristics->print_rules( "all.rules.debug", heuristics->get_all_rules() );
#endif

	double tf = time_used();
	log.stream() << ";; size_of_b0_approx=" << models().size() << std::endl;
	log.stream() << ";; heuristic_setup_time=" << (tf-t0)+approx_time << std::endl;

        sm_task.set_initial_models( &(m_models) );

        root()->evaluate(heuristics, models());

    	// Gets the heuristic distance from the goal
	log.stream() << ";; initial_heuristic_value=" << root()->hn << std::endl;

#ifdef DEBUG
	heuristics->print_h_table_file("heuristic.table");
#endif

	stats.notify_initial_sample_heuristic( root()->hS );
	stats.notify_initial_card_heuristic( root()->hX );
	t0 = time_used();
	if ( root()->hn < infty() )
		g = execute_search_strategy();
	else 
		g = NULL;
	tf = time_used();
	log.stream() << ";; search_time="; report_interval( t0, tf, log.stream() );

	if ( g == NULL )
	{
		m_failed = true;
		log.stream() << ";; No plan found" << std::endl;
		return plan_found;
	}
	else
		plan_found = true;

	Operator_Vec plan;
	SearchNode* tmp = g;
	while ( tmp != NULL )
	{
		plan.push_back( tmp->op ); 
		tmp = tmp->father;
		/* if(tmp->op == 0)
			break;  */
	}	
	for ( unsigned k = 0; k < plan.size() / 2; k++ )
		std::swap( plan[k], plan[plan.size()-k-1] );

	log.stream() << ";; Plan found:" << std::endl;
        if (opts.output_ipc())
                ipc.stream() << "linear " << plan.size()-1 << " ";

	for ( unsigned k = 0; k < plan.size(); k++ )
	{
		if ( plan[k] == 0 ) continue;
		sm_task.print_operator( plan[k], log.stream());
                if (opts.output_ipc())
                        ipc.stream() <<  (sm_task.useful_ops()[plan[k]])->prototype() << " ";
		log.stream() << std::endl;
	}
	log.stream() << ";; plan_length=" << plan.size()-1 << std::endl;

        ipc.stream().close();

	return plan_found;
}

void	BeliefSearch::execute( std::vector<PDDL::Operator*>& prefix )
{
        set_root( SearchNode::root() );	
	approximate_initial_belief();
	Closed_List		CLOSED(5113);
	Planner_Stats&	stats = Planner_Stats::instance();

	std::cout << "FIRST ORDER APPROXIMATION" << std::endl;	
	std::cout << "States in b' ( " << models().size() <<  " )  " << std::endl;	
	for ( unsigned k = 0; k < models().size(); k++ )
	{
		std::cout << "s_" << k+1 << ": ";
		for ( unsigned l = 1; l < models()[k].size(); l++ ) 
		  {
		    std::cout << models()[k][l] << " " << (models()[k][l] > 0 ? " " : " -");
		    sm_task.print_fluent( models()[k][l] > 0 ? models()[k][l] : (unsigned)std::abs(models()[k][l]) );
		    std::cout << ", ";
		  }
		std::cout << std::endl;
	}
	heuristics =  new Dijkstra( models().size() );
	root()->evaluate( heuristics, models() );

	report_memory_usage();	
	std::cout << "ROOT Node:" << std::endl;
	root()->describe();
	CLOSED.add_element( root() );
	assert( CLOSED.get_element( root() ) != NULL );
	SearchNode* n = root();
	for ( unsigned k = 0; k < prefix.size(); k++ )
	{
		std::cout << "Applying operator ";
		sm_task.print_operator( prefix[k], std::cout );
		std::cout << std::endl;
		if ( !n->b->can_apply(prefix[k]) )
		{
			std::cerr << "Could not apply this operator!!!" << std::endl;
			std::exit(1);
		}
		std::cout << "SUCCESSOR" << std::endl;
		n = n->successor( prefix[k]->get_idx() );
		n->do_belief_tracking( m_models );
		n->evaluate( heuristic(), m_models );
		n->describe();
		stats.add_node_size(n->size());
#ifdef DEBUG
		report_memory_usage();
		std::cout << "Memory in nodes: " << stats.node_megabytes() << " MBytes" << std::endl;	
#endif
		if ( CLOSED.get_element( n ) != NULL )
		{
			std::cerr << "WHOOPS! Reported as duplicate!" << std::endl;
			std::cerr << "DUPLICATE reported:" << std::endl;
			CLOSED.get_element(n)->describe();
			std::exit(1);
		}
		CLOSED.add_element( n );
		assert( CLOSED.get_element(n) != NULL );
	}
	assert( CLOSED.get_element( root() ) != NULL );
}	

void	BeliefSearch::process_root_node()
{
	set_root( SearchNode::root() );
	approximate_initial_belief();
	std::cout << "FIRST ORDER APPROXIMATION" << std::endl;	
	std::cout << "States in b' ( " << models().size() <<  " )  " << std::endl;	
	for ( unsigned k = 0; k < models().size(); k++ )
	{
		std::cout << "s_" << k+1 << ": ";
		for ( unsigned l = 1; l < models()[k].size(); l++ ) 
		  {
		    std::cout << models()[k][l] << " " << (models()[k][l] > 0 ? " " : " -");
		    sm_task.print_fluent( models()[k][l] > 0 ? models()[k][l] : (unsigned)std::abs(models()[k][l]) );
		    std::cout << ", ";
		  }
		std::cout << std::endl;
	}
	std::cout << "ROOT Node:" << std::endl;
	std::cout << "Heuristic Info:" << std::endl;
	heuristics =  new Dijkstra( models().size() );
	root()->do_belief_tracking( models() );
	root()->evaluate( heuristics, models() );
	root()->describe();
	heuristics->print_rules( "state.rules", heuristics->states_rules() );
	heuristics->print_rules( "merge.rules", heuristics->merge_rules() );
	heuristics->print_rules( "static-or.rules", heuristics->get_static_rules() );
}

} // end namespace
