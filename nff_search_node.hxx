/*
  Alexandre Albore, Miguel Ramirez, Hector Geffner
  T1 conformant Planner
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
#ifndef __NFF_SEARCHNODE__
#define __NFF_SEARCHNODE__

#include "nff.hxx"
#include "nff_belief.hxx"
#include "PDDL.hxx"
//#include "jenkins_12bit.hxx"
#include "nff_minisat.hxx"
#include "nff_theory_fragment.hxx"


namespace NFF
{  
 	class Dijkstra;
 
	class SearchNode
	{
	public:

		SearchNode();
		~SearchNode();

		int           get_discount( Dijkstra* h );
//		unsigned      evaluate( Dijkstra* h );
                unsigned      evaluate( Dijkstra* heuristic, std::vector< std::vector<int> >& models );
		SearchNode*   get_father() { return father;}

		SearchNode*  successor( unsigned o );
		static SearchNode*		root();

		bool   is_equal( SearchNode& o, std::vector< std::vector<int> >& models );

		bool			operator==( SearchNode& o )
		{
			Planner_Stats& stats = Planner_Stats::instance();
                        //AA: no SAT calls to check entailment
                        if ( NO_SAT_CALLS && !stats.is_width_1()) 
                                return false;

			if ( *b != *(o.b) ) return false;
			if ( !same_models( o, *(sm_task.get_initial_models()) ) ) return false;
			if ( stats.is_width_1() ) return true; 

			return check_entailment(o);
		}
		unsigned 	hash();
//		void     	compute_hash(); 
		void     	compute_hash( std::vector<std::vector<int> >& m ); 
		bool 		can_apply( PDDL::Operator& op );
		bool 		same_models( SearchNode& o, std::vector<std::vector<int> > & m );	
		void 		print_f ( unsigned f, unsigned oo);
		void 		print_f ( unsigned f, unsigned off, bool s);
                void 		print_clauses();
		void 		describe();
//		void 		update_models( std::vector< PDDL::Operator* >& op_effs );
                void            update_single_model ( std::vector< PDDL::Operator* >& op_effs, 
                                                      std::vector< int >& lmodel );
                void	        update_models( std::vector< PDDL::Operator* >& op_effs, 
                                               std::vector< std::vector< int > >& mod);
		bool 		check_entailment( SearchNode& o );
//		bool		condition_holds_in_sample( Atom_Vec& c );
		void		k0_projection( std::vector< PDDL::Operator* >& op_effs );
		void		action_compilation_rule( std::vector< PDDL::Operator* >& op_effs );
                void            check_known_atoms_from_samples(std::vector<std::vector<int> >& samples);
		void		determine_testable_precs( std::vector<PDDL::Operator*>& ops );
		void		determine_testable_goals( Atom_Vec& tG );
		void		test_conjunction( MiniSAT& solver, Atom_Vec& conj );
		
		// I leave them public for ease of access
		Belief::Belief*			b;	// the belief state
	
	
		//heuristic values
		unsigned				gn;	// accumulated cost
		unsigned				hn;	// heuristic value
		unsigned				hS;
		unsigned				hX;
		unsigned				tb;
		unsigned				queue;
		unsigned				fn;	// evaluation function
		SearchNode*				father;
		unsigned		        	op;	// operator
		unsigned                    		timestep;
//		std::vector< std::vector< int > >	models;
		Theory_Fragment*			theory_step;
		std::vector<bool>			potentially_known;
//		std::vector<bool>			dead_model;
		unsigned				num_dead_models;
		std::vector<unsigned>&	hS_HA()
		{
			return m_hS_HA;
		}
		bool	is_hS_HA( unsigned op )
		{
			return m_hS_HA_set->isset(op);
		}
		std::vector<unsigned>&	hX_HA()
		{
			return m_hX_HA;
		}
		bool	is_hX_HA( unsigned op )
		{
			return m_hX_HA_set->isset(op);
		}

		bool				same_prefix( SearchNode& n );
		void				do_belief_tracking();
                void	                        do_belief_tracking(std::vector< std::vector<int> >& models);
                void                            belief_tracking_with_sat();
		void				check_dead_models();
		void				replace_dead_sample( std::vector<int>& model );
		unsigned			size();

	protected:
		std::vector<unsigned>		m_hS_HA;
		PDDL::Fluent_Set*		m_hS_HA_set; 
		std::vector<unsigned>		m_hX_HA;
		PDDL::Fluent_Set*		m_hX_HA_set;
		static	PDDL::Task&		sm_task;
		unsigned			m_hash_key;
		bool 				is_literal_known( MiniSAT& solver, unsigned p );
		void				compute_persist_set( Belief& b, 
									std::vector< PDDL::Operator*>& effs, 
									PDDL::Fluent_Set& not_affected );
		void				preserve_persisting_lits( Belief& b, Belief& bp, 
									PDDL::Fluent_Set& persist,
									PDDL::Fluent_Set& processed );
		void				retrieve_effect_conditions( std::vector<PDDL::Operator*>& effs,
										std::vector<Atom_Vec>& conds );

		void				check_support_rule( Belief& b, 
									std::vector<Atom_Vec>& conds,
									std::vector<bool>& results );
		void				check_cancellation_rule( Belief& b,
									std::vector<Atom_Vec>& conds,
									std::vector<bool>& results );
		void				check_action_compilation( Belief& b,
									  std::vector<Atom_Vec>& conds,
									  std::vector<PDDL::Operator*>& effs,
									  std::vector<bool>& result,
									  std::vector<unsigned>& result_lit );
		bool				condition_may_execute( Belief& b,
									Atom_Vec& condition );	
	public:

		void set_belief (Belief* bel ) { b = bel; }
		void set_op ( unsigned o )  { op = o ; }
	
		void make_root_clauses();
		void make_clauses ();
	}; // end class


	class RootNode : public SearchNode
	{
		std::vector< std::vector< int > >	models;
        };


	inline	unsigned	SearchNode::hash()
	{
		return m_hash_key;
	}
}

#endif // nff_search_node.hxx
