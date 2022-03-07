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
#ifndef __NFF_BELIEF__
#define __NFF_BELIEF__

#include "PDDL.hxx"
#include <vector>
#include "pddl_fluent_set.hxx"
#include <iostream>
#include <algorithm>

namespace NFF
{
  

  class Dijkstra;
  //  class Operator;

  class Belief
  {
    
  public:
    explicit Belief();
    Belief( std::vector<unsigned>& known,
	    std::vector<unsigned>& unknown,
	    PDDL::Fluent_Set&      set1, 
	    PDDL::Fluent_Set&      set2   );
    Belief( std::vector<unsigned>& known,
	    std::vector<unsigned>& unknown,
	    PDDL::Fluent_Set&      set1, 
	    PDDL::Fluent_Set&      set2, 
	    std::vector<std::vector<unsigned> >& clauses );
    Belief( std::vector<unsigned>& known,
	    std::vector<unsigned>& unknown,
	    std::vector<std::vector<unsigned> >& clauses );
    /*
    Belief( std::vector<unsigned>* known,
	    std::vector<unsigned>* unknown, 
	    std::vector<std::vector<unsigned> >* clauses );  
    */
    Belief( Belief& b );
    Belief( Belief* b );
    ~Belief();

    static Belief* make_initial_belief();
    //    static Belief* make_goal_state();

    std::vector<unsigned>&	known_vec();
    std::vector<unsigned>&	unknown_vec();
    PDDL::Fluent_Set&	known_set();
    PDDL::Fluent_Set&	unknown_set();
    std::vector<std::vector<unsigned> >&  clauses_vec();

    void       print( void );
	void	print( std::ostream& os );

	bool       	can_apply( PDDL::Operator* op );
	bool       	can_apply( unsigned op_idx );
	bool       	has_no_effects( PDDL::Operator* op );
	bool		check_consistency();
	bool		effect_executes( PDDL::Operator* op );
    //    Belief*           apply( PDDL::Operator* op );
    //    Belief*	      apply( unsigned op_idx );

        bool		operator!=(Belief& o );
        bool            operator==( Belief& o );
	unsigned	hash();

    void              add_known_lit(unsigned l); //PDDL::Fluent* lit);
    void              add_unknown_lit(unsigned l);//PDDL::Fluent* lit);
    void              add_clause( std::vector<unsigned> c ); //std::vector<PDDL::Fluent*>& cl );
    void              remove_unknown_lit(unsigned l); 
    void              remove_known_lit ( unsigned lit );
    void              clear();

	bool		entails_goal();
	bool		is_true( unsigned f );
	bool		is_false( unsigned f );
	unsigned	size();
  protected:

    std::vector<unsigned>	m_known;
    std::vector<unsigned>	m_unknown;
    std::vector<std::vector<unsigned> > m_clauses;

    static PDDL::Task&		task();
    static PDDL::Task&	        sm_task;

    PDDL::Fluent_Set*   	m_unkset;
    PDDL::Fluent_Set*           m_knowset;
  };

	inline bool	Belief::is_true( unsigned f )
	{
		return known_set().isset(f);
	}	
	
	inline bool	Belief::is_false( unsigned f )
	{
		return !known_set().isset(f) && !unknown_set().isset(f);
	}

  inline PDDL::Task&		Belief::task()
  {
    return sm_task;
  }

  inline std::vector<unsigned>&	Belief::known_vec()
  {
    return m_known;
  }

  inline std::vector<unsigned>&	Belief::unknown_vec()
  {
    return m_unknown;
  } 

  inline  std::vector<std::vector<unsigned> >&	Belief::clauses_vec()
  {
    return m_clauses;
  } 

  inline PDDL::Fluent_Set&	Belief::known_set()
  {
    return *m_knowset;
  }

  inline PDDL::Fluent_Set&	Belief::unknown_set()
  {
    return *m_unkset;
  }

	inline bool Belief::operator!=( Belief& o )
	{
		return !(this->operator==( o ) );
	}

inline bool Belief::operator==( Belief& o )
{
     	unsigned i;
	if ( o.known_vec().size() != known_vec().size() )
		return false;
	if ( o.unknown_vec().size() != unknown_vec().size() )
		return false;
	
	for ( i = 0; i < o.known_vec().size(); i++ )
	{
		if ( !known_set().isset( o.known_vec()[i] ) )
			return false;
	}

	for ( i = 0; i < o.unknown_vec().size(); i++ )
	{
		if ( !unknown_set().isset( o.unknown_vec()[i] ) )
			return false;
	}

	return true;
}

}

#endif // nff_belief.hxx
