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
#ifndef __PDDL__
#define __PDDL__

#include "pddl_basic_types.hxx"
#include "pddl_string_table.hxx"
#include "pddl_fluent_set.hxx"
#include <iostream>
#include <fstream>
#include <set>
#include <cassert>
#include <minisat/core/SolverTypes.h>

namespace NFF
{

        class Belief;
        class Mutexes;
        typedef std::vector<unsigned>   Atom_Vec;
}

namespace PDDL
{

class Fluent // A grounded predicate
{
public:
  
  explicit Fluent();
  Fluent( int code );
  Fluent( int code, bool sign );
  ~Fluent();
  
  int       code() { return m_code; }
  bool     is_pos() { return m_sign; }
protected:
  int		      m_code;
  bool                m_sign; // true if positive
};

class Operator
{
public:

	explicit Operator();
	Operator( int code );

	~Operator();

	Fluent_Set&		preconds() 	{ return *m_precondition; }
	Fluent_Set&		adds() 		{ return *m_adds; }
	Fluent_Set&		dels() 		{ return *m_dels; }
       	Fluent_Set&		hard_preconds()	{ return *m_hard_precs; }
	std::vector<unsigned>&	prec_vec()	{ return m_prec_vec; }
	std::vector<unsigned>&	hard_prec_vec()	{ return m_hard_prec_vec; }
	std::vector<unsigned>&	add_vec()	{ return m_add_vec; }
	std::vector<unsigned>&	del_vec()	{ return m_del_vec; }
        std::vector<Operator*>&  same_op_vec()   { return m_same_op_vec;}  
        std::map<unsigned, unsigned>& prec_layer()    { return m_prec_layer; }
        bool                    prec_ordered()  {return !m_prec_layer.empty();}  
	int			code()		{ return m_code; }
	float			metric_cost() { return m_metric; }
	void			set_metric_cost( float cost ) { m_metric = cost; }
        void                    set_same_op_vec( std::vector<Operator*>& s)   { m_same_op_vec = s;}
        unsigned                get_pc_index(unsigned fluent);
        unsigned                get_idx(){return m_idx;}
        void                    set_idx(unsigned i){m_idx = i;}
	void			set_prototype(	unsigned idx ) { m_prot_index = idx; }
	unsigned		prototype() { return m_prot_index; }

protected:
	

        void                    propagate_precedence(std::map<unsigned, NFF::Atom_Vec>& precedence, unsigned prec  );

        /* setup the CNF for the following SAT calls.   */
        void generate_clauses();

	std::vector<unsigned>	m_prec_vec;
	std::vector<unsigned>	m_add_vec;
	std::vector<unsigned>	m_del_vec;
        std::vector<Operator*>  m_same_op_vec;
	Fluent_Set*		m_precondition;
	Fluent_Set*		m_adds;
	Fluent_Set*		m_dels;
	int			m_code;
	float			m_metric;
        unsigned                m_idx;
        std::map<unsigned, unsigned> m_prec_layer; 
  
        /**Conditional Effects: Conditions and Preconditions are togheter
         * in prec_vec. The following vector is to distinguish those that are 
         * compulsory (prec). Specilly useful to check if action is applicable.**/
  
        std::vector<unsigned>	m_hard_prec_vec;
        Fluent_Set*		m_hard_precs;
	unsigned		m_prot_index;
};


class Task
{
	
private:
	Task();
public:
	~Task();
	static Task& instance();

	void setup();
	
	std::string	domain_name() { return m_domain_name; }
	std::string	problem_name() { return m_problem_name; }

	void		set_domain_name( std::string name )
	{
		m_domain_name = name;
	}

	void		set_problem_name( std::string name )
	{
		m_problem_name = name;
	}
	int 			fluent_count() { return (int)m_fluent_table.size(); }
	std::vector<Fluent*>&	fluents() { return m_fluent_table; }
	std::vector<Operator*>& useful_ops() { return m_useful_operator_table; }

	std::vector<Operator*>& prototype_ops() { return m_prototype_ops; }

        NFF::Belief&            initial_belief() {return *m_initial_belief; }
	std::vector<unsigned>&	initial_state() { return m_initial_state; }
	std::vector<unsigned>&	initial_unknown() { return m_initial_unknown; }
        std::vector<std::vector <unsigned> >& initial_clauses() { return m_initial_clauses;}
        std::vector<std::vector <unsigned> >& oneofs() { return m_initial_oneofs;}

	std::vector<unsigned>& 	goal_state() { return m_goal_state; }

        std::vector<std::vector<unsigned> >&	pos_invariants_table() { return  m_pos_invariants; }
        std::vector<unsigned>&   pos_invariant_lits( unsigned op ) { return m_pos_invariants[op]; }
        std::vector<std::vector<unsigned> >&	neg_invariants_table() { return  m_neg_invariants; }
        std::vector<unsigned>&  neg_invariant_lits( unsigned op ) { return m_neg_invariants[op]; }
        std::vector<unsigned>&	not_equiv_table() { return  m_not_equivalence_table; }
        unsigned	        not_equivalent( unsigned i ) { return m_not_equivalence_table.at(i); }
        std::vector<unsigned>&	ff_equiv_table() { return  m_ff_equivalence_table; }
        unsigned	        ff_equivalent( unsigned i ) { return m_ff_equivalence_table.at(i); } 
	unsigned		start() { return m_start_id; }
	unsigned		end() { return m_end_id; }
        unsigned		no_op() { return m_no_op_id; }
        unsigned                dummy_goal() {return m_dummy_goal_id;}
        // operators with fluent f in their preconditions
	std::vector<unsigned>&	required_by( unsigned f ) { return m_required_by[f]; }
	// operators with fluent f in their add list
	std::vector<unsigned>&	added_by( unsigned f ) { return m_added_by[f]; }
	// operators with fluent f in their del list
	std::vector<unsigned>&	deleted_by( unsigned f ) { return m_deleted_by[f]; }
	// operators that e-delete f ( either q \in Add(o), h²( f, q ) = inf, or r \in Pre(o), r \notin Add(o)
	// h²(p,r) = inf
	// NOTE: This info becomes available through the seq_h2 or par_h2 compute() methods.
	std::vector<unsigned>&	e_deleted_by( unsigned f ) { return m_e_deleted_by[f]; }
	PDDL::Fluent_Set&	fast_op_edeletes( unsigned op ) { return *(m_fast_op_edeletes[op]); }

	std::vector<unsigned>&	op_edeletes( unsigned op ) { return m_op_edeletes[op]; }

	std::vector< std::vector<unsigned> >& landmarks_table() { return m_op_landmarks; }
	std::vector<unsigned>&  landmarks_for( unsigned op ) { return m_op_landmarks[op]; }
	
        NFF::Mutexes&           mutex_list() { return *m_mutexes; }

        void                    set_mutexes( NFF::Mutexes* m) { m_mutexes = m; }

        void                    calculate_relevance();

	bool			useful( unsigned op ) { return m_is_useful[op]; }

	bool			reachable( unsigned op ) { return m_is_reachable[op]; }
	unsigned		start_time_lb( unsigned op ) { return m_start_times_lb[op]; }
	std::vector<unsigned>&	start_time_lbs() { return m_start_times_lb; }

	void 			set_start_time_lb( std::vector<unsigned>& stimes );


	float		metric_start_time_lb( unsigned op ) { return m_metric_start_times_lb[op]; }
	std::vector<float>&	metric_start_time_lbs() { return m_metric_start_times_lb; }

	void 			set_metric_start_time_lb( std::vector<float>& stimes );

	void			print_fluents( std::ostream& os );
	void			print_fluent( Fluent* f, std::ostream& os );
	void			print_fluent( unsigned f, std::ostream& os )
	{
	  //assert( f > 0 && f < fluents().size() );
		if ( f == 0 )
		{
			os << "(GOAL)";
			return;
		}
		print_fluent( fluents()[f], os );
	}

	void			print_fluent( unsigned f )
	{
	  //		assert( f > 0 && f < fluents().size() );
		print_fluent( fluents()[f], std::cout );
                //       std::cout << std::endl;
  	}


        Fluent_Set& precs_goals_set()    
        {
                return *m_prec_goal_fluents_set;
        }

        bool is_prec_or_goal( unsigned f )      
        { 
                return  m_prec_goal_fluents_set->isset( f ); 
        }
      
        std::string           get_fluent_name( unsigned f );

	void		      print_operator( Operator* o, std::ostream& os );
        void                  print_same_operators( Operator* oi, std::ostream& os );
	void		      print_operator_full_notab( Operator* o, std::ostream& os );
	void		      print_operator_full( Operator* o, std::ostream& os );
        void		      print_operator( unsigned o)
	{
		assert( o < useful_ops().size() );
		print_operator( useful_ops()[o], std::cout );
	}
        void		      print_operator( unsigned o, std::ostream& os )
	{
		assert( o < useful_ops().size() );
		print_operator( useful_ops()[o], os );
	}
	void		      print_operators( std::ostream& os );

	void		      print_initial_state( std::ostream& os );
        void                  print_initial_unknown_atoms( std::ostream& os );
	void		      print_goal_state( std::ostream& os );

	String_Table&	      str_tab() { return m_string_table; }

        bool                  is_star_fluent(unsigned p) {return get_common_predicate_facts(p) != NULL; }    
        std::vector< unsigned >*  get_common_predicate_facts( unsigned p ) 
        {
	  std::string& pred_name = str_tab().get_token( fluents()[p]->code() );
	  return m_predicateName_facts.find( pred_name ) == m_predicateName_facts.end() ? NULL : &(m_predicateName_facts[pred_name]); 
        } 
  
	bool		      equal_effects( unsigned o1, unsigned o2 )
	{
		if ( o1 == o2 ) return true;
		Operator* o1ptr = useful_ops()[o1];
		Operator* o2ptr = useful_ops()[o2];
		
		if ( o1ptr->add_vec().size() != o2ptr->add_vec().size() ) return false;
		if ( o1ptr->del_vec().size() != o2ptr->del_vec().size() ) return false;

		for ( unsigned k = 0; k < o1ptr->add_vec().size(); k++ )
			if ( !o2ptr->adds().isset( o1ptr->add_vec()[k] ) ) return false;
		for ( unsigned k = 0; k < o2ptr->del_vec().size(); k++ )
			if ( !o2ptr->dels().isset( o1ptr->del_vec()[k] ) ) return false;
		
		return true;
	}
      
        bool                   neg_fluents(){return m_negation;}
        bool                   cond_eff(){return m_cond_effects;}
        void                   set_initial_models(std::vector<std::vector<int> >* m);
        std::vector<std::vector<int> >*               get_initial_models() { return m_initial_models; }
        std::vector<unsigned>&  relevant_to( unsigned i ) { return m_relevant_atoms[i]; }

	float                  op_cost( unsigned op ) { return m_op_costs[op]; }

        unsigned               persistent_effects(unsigned op, std::vector<unsigned>& eff, unsigned persistent_goal);
        unsigned               persistent_neg_effects(unsigned op, std::vector<unsigned>& neg_eff, unsigned persistent_goal);
        std::vector<unsigned>&      fluent2star_fluent(unsigned p){return m_fluents_generalized[p];}
protected:
	void			create_fluents();
	void 			create_init_and_goal();
	void			create_operators();
	void			create_invariants();
 
protected:
	std::string					m_domain_name;
	std::string					m_problem_name;
	std::vector< Fluent* >				m_fluent_table;
	std::vector< Operator* >			m_useful_operator_table;
	std::vector< Operator* >                        m_prototype_ops;
  //	std::vector< Operator* >			m_vplan_table;
        NFF::Belief*                                    m_initial_belief;
        NFF::Mutexes*                                   m_mutexes;
        std::vector< std::vector<unsigned> >            m_relevant_atoms;

        std::vector< std::vector<int> >*                m_initial_models;
        std::vector<unsigned>				m_initial_state;
        std::vector<unsigned>	                        m_initial_unknown;
        std::vector< std::vector<unsigned> >            m_initial_clauses;
        std::vector< std::vector<unsigned> >            m_initial_oneofs;
	std::vector<unsigned>				m_goal_state;
        std::vector< unsigned >                         m_ff_equivalence_table; 
        std::vector< unsigned >                         m_not_equivalence_table;
        std::vector< std::vector<unsigned> >            m_pos_invariants;
        std::vector< std::vector<unsigned> >            m_neg_invariants;
	Fluent_Set*	                                m_prec_goal_fluents_set;
	unsigned					m_start_id;
	unsigned					m_end_id;
	unsigned					m_no_op_id;
	unsigned					m_dummy_goal_id;
	std::vector< std::vector<unsigned> >		m_required_by;
	std::vector< std::vector<unsigned> >		m_added_by;
	std::vector< std::vector<unsigned> > 		m_deleted_by;
	std::vector< std::vector<unsigned> > 		m_e_deleted_by;
	std::vector< PDDL::Fluent_Set* >		m_fast_op_edeletes;
	std::vector< std::vector<unsigned> >		m_op_edeletes;
	std::vector< std::vector<unsigned> >		m_op_landmarks;
	std::vector<bool>				m_is_useful;
	std::vector<bool>				m_is_reachable;
	std::vector<unsigned>				m_start_times_lb;
	std::vector<float>				m_metric_start_times_lb;
	String_Table&					m_string_table;
	std::ofstream					m_glog;
	std::vector<bool>			 	m_negs_in_precs;
	std::vector<float>				m_op_costs;
        bool                                            m_negation;
        bool                                            m_cond_effects;
	std::vector< std::vector<unsigned> >		m_fluents_generalized;
	std::map<std::string, std::vector<unsigned> >   m_predicateName_facts;
        std::map<unsigned, unsigned>                    m_goal2index;
};

}
#endif // PDDL.hxx
