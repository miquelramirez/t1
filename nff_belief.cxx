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

#include "PDDL.hxx"
#include <vector>
//#include "pddl_fluent_set.hxx"
#include <iostream>
#include <algorithm>
#include "nff_belief.hxx"
#include "hash_table.hxx"
#include "nff_planner_stats.hxx"

namespace NFF
{
 

  PDDL::Task& Belief::sm_task = PDDL::Task::instance();
  
  Belief::Belief()
  {
    m_knowset = new PDDL::Fluent_Set( task().fluents().size()+1 );
    m_unkset = new PDDL::Fluent_Set( task().fluents().size()+1);
  }
 
 Belief::Belief( std::vector<unsigned>& known,
		  std::vector<unsigned>& unknown, 
		  PDDL::Fluent_Set& kset,
		  PDDL::Fluent_Set& uset ) 
  {
    // Makes a copy of the vector for this belief state
    m_known = known; 
    m_unknown = unknown;
    
    m_knowset = new PDDL::Fluent_Set( task().fluents().size()+1 );	
    for ( unsigned i = 0; i < known_vec().size(); i++ )
      if ( kset.isset( known_vec()[i] ) )
	known_set().set( known_vec()[i] );
    m_unkset = new PDDL::Fluent_Set( task().fluents().size()+1 );	
    for ( unsigned i = 0; i < unknown_vec().size(); i++ )
      if ( uset.isset( unknown_vec()[i] ) )
	unknown_set().set( unknown_vec()[i] );
    clauses_vec().clear();
  }


 
Belief::Belief( 	std::vector<unsigned>& known,
		  	std::vector<unsigned>& unknown, 
		  	std::vector<std::vector<unsigned> >& clauses) 
{
	// Makes a copy of the vector for this belief state
	m_known = known; 
	m_unknown = unknown;
	// Doing the sorting to better compare them later
	sort( m_known.begin(), m_known.end() );
	sort( m_unknown.begin(), m_unknown.end() );
	for ( std::vector<std::vector<unsigned> >::iterator it = clauses.begin(); it < clauses.end(); it++ )
	{
		sort ( (*it).begin(), (*it).end() );
		clauses_vec().push_back( *it );
	}

	m_knowset = new PDDL::Fluent_Set( task().fluents().size()+1 );	
	for ( unsigned i = 0; i < known_vec().size(); i++ )
		known_set().set( known_vec()[i] );
	m_unkset = new PDDL::Fluent_Set( task().fluents().size()+1 );	
	for ( unsigned i = 0; i < unknown_vec().size(); i++ )
		unknown_set().set( unknown_vec()[i] );
	
#ifdef DEBUG
	for ( unsigned k = 0; k < known_vec().size(); k++ )
	{
		assert( !unknown_set().isset( known_vec()[k] ) );
		unsigned not_p = sm_task.not_equivalent( known_vec()[k] );
		if ( not_p )
			assert( !unknown_set().isset( not_p ) );
	}
	for ( unsigned k = 0; k < unknown_vec().size(); k++ )
	{
		assert( !known_set().isset( unknown_vec()[k] ) );
		unsigned not_p = sm_task.not_equivalent( unknown_vec()[k] );
		if ( not_p )
			assert( !known_set().isset( not_p ) );
	}
	
#endif
}

unsigned	Belief::size()
{
	unsigned numBytes = sizeof(m_known);
	numBytes += sizeof(m_unknown);
	numBytes += sizeof(m_clauses);
	numBytes += sizeof(sm_task);
	numBytes += sizeof(m_unkset);
	numBytes += sizeof(m_knowset);
	numBytes += m_known.size()*sizeof(unsigned);
	numBytes += m_unknown.size()*sizeof(unsigned);
	numBytes += m_unkset->bits().npacks()*sizeof(unsigned);
	numBytes += m_knowset->bits().npacks()*sizeof(unsigned);
	for ( unsigned k = 0; k < m_clauses.size(); k++ )
	{
		numBytes += sizeof(m_clauses[k]);
		numBytes += m_clauses[k].size()*sizeof(unsigned);
	}
	return numBytes;
}

bool	Belief::check_consistency()
{
	
	bool inconsistent = false;
	for ( unsigned k = 0; k < known_vec().size(); k++ )
	{
		if ( unknown_set().isset( known_vec()[k] ) )
		{
			inconsistent = true;
#ifdef DEBUG
			std::cout << "Fluent "; sm_task.print_fluent( known_vec()[k] );
			std::cout << " found in both Known and Unknown" << std::endl;
#endif
			break;
		}
		unsigned neg_p = sm_task.not_equivalent( known_vec()[k] );
		if ( neg_p )
		{
			if ( unknown_set().isset( neg_p ) )
			{
#ifdef DEBUG
				std::cout << "Fluent "; sm_task.print_fluent( neg_p );
				std::cout << " found in both Known and Unknown" << std::endl;
#endif

				inconsistent = true;
				break;
			}
		}
	}
	for ( unsigned k = 0; k < unknown_vec().size(); k++ )
	{
		unsigned neg_p = sm_task.not_equivalent( unknown_vec()[k] );
		if ( neg_p )
		{
			if ( !unknown_set().isset( neg_p ) )
			{
#ifdef DEBUG
				std::cout << "Fluent  "; sm_task.print_fluent( neg_p, std::cout );
				std::cout << "NOT found in Unknown but "; sm_task.print_fluent( unknown_vec()[k], std::cout );
				std::cout << " was there" << std::endl;
#endif

				inconsistent = true;
				break;
			}
		}
		
	}
	return inconsistent;
}
 
  Belief::Belief( std::vector<unsigned>& known,
		  std::vector<unsigned>& unknown, 
		  PDDL::Fluent_Set& kset,
		  PDDL::Fluent_Set& uset,
		  std::vector<std::vector<unsigned> >& clauses) 
  {
    // Makes a copy of the vector for this belief state
    m_known = known; 
    m_unknown = unknown;

    m_knowset = new PDDL::Fluent_Set( task().fluents().size()+1 );	
    for ( unsigned i = 0; i < known_vec().size(); i++ )
      if ( kset.isset( known_vec()[i] ) )
	known_set().set( known_vec()[i] );
    m_unkset = new PDDL::Fluent_Set( task().fluents().size()+1 );	
    for ( unsigned i = 0; i < unknown_vec().size(); i++ )
      if ( uset.isset( unknown_vec()[i] ) )
	unknown_set().set( unknown_vec()[i] );

  }

  Belief::Belief( Belief::Belief& s )
  {
    m_known = s.known_vec();
    m_unknown = s.unknown_vec();
    for ( std::vector<std::vector<unsigned> >::iterator it =
	    s.clauses_vec().begin(); it < s.clauses_vec().end(); it++ )
      clauses_vec().push_back( *it );

    m_knowset = new PDDL::Fluent_Set( task().fluents().size()+1 );	
    for ( unsigned i = 0; i < known_vec().size(); i++ )
      if ( s.known_set().isset( known_vec()[i] ) )
	known_set().set( known_vec()[i] );
    m_unkset = new PDDL::Fluent_Set( task().fluents().size()+1 );	
    for ( unsigned i = 0; i < unknown_vec().size(); i++ )
      if (s.unknown_set().isset( unknown_vec()[i] ))
	unknown_set().set( unknown_vec()[i] );
  }

 Belief::Belief( Belief::Belief* s )
  {
    m_known = s->known_vec();
    m_unknown = s->unknown_vec();
    for ( std::vector<std::vector<unsigned> >::iterator it =
	    s->clauses_vec().begin(); it < s->clauses_vec().end(); it++ )
      clauses_vec().push_back( *it );
    
    m_knowset = new PDDL::Fluent_Set( task().fluents().size()+1 );	
    for ( unsigned i = 0; i < known_vec().size(); i++ )
      if ( s->known_set().isset( known_vec()[i] ) )
	known_set().set( known_vec()[i] );
    m_unkset = new PDDL::Fluent_Set( task().fluents().size()+1 );	
    for ( unsigned i = 0; i < unknown_vec().size(); i++ )
      if (s->unknown_set().isset( unknown_vec()[i] ))
	unknown_set().set( unknown_vec()[i] );
  }
  
  Belief::~Belief()
  {
    delete m_unkset;
    delete m_knowset;
  }
  
  void Belief::clear( void )
  {
    known_vec().clear();
    unknown_vec().clear();
    clauses_vec().clear(); 
    known_set().reset();
    unknown_set().reset();
  }
  
  Belief::Belief* Belief::make_initial_belief()
  {

    return new Belief( task().initial_state(),
		       task().initial_unknown(), 
		       task().initial_clauses() );
  }
  

  void Belief::add_known_lit ( unsigned lit ) //PDDL::Fluent* lit )
  {
    if ( !known_set().isset( lit ) )
      {
	known_vec().push_back( lit );
	known_set().set( lit );
      }
  }

  void Belief::remove_known_lit ( unsigned lit )
  {
    if ( known_set().isset( lit ) )
	known_set().unset( lit );

    std::vector<unsigned>::iterator it = std::find( known_vec().begin(), known_vec().end(), lit );
    if ( it != known_vec().end() ) 
            known_vec().erase( it );
  }

  void Belief::add_unknown_lit ( unsigned lit ) //(PDDL::Fluent* lit )
  {
    if ( !(unknown_set().isset( lit )) )
      {
	unknown_vec().push_back( lit );
	unknown_set().set( lit );
      }
  }
   

  void Belief::remove_unknown_lit( unsigned lit )
 {
   // unknown_vec()
   // NB: the removed lit still stays in the vector, but not in the set.
   if ( unknown_set().isset( lit ) )
     unknown_set().unset( lit );
}


  void Belief::add_clause ( std::vector<unsigned> c )
  {
    clauses_vec().push_back( c );
  }

	void Belief::print( std::ostream& os ) 
	{
		os << "Belief State:" << std::endl; 
		os << "\tKnow Atoms:" << std::endl;
		for ( unsigned i = 0; i < known_vec().size(); i++ ) {
			os << " "<< task().get_fluent_name( known_vec().at( i ));
			if ( i && i % 4 == 0 )
				os << std::endl;
		}
    		os << std::endl;
		os << "\tUnknown Atoms:" << std::endl;
		for ( unsigned i = 0; i < unknown_vec().size(); i++ ) { 
			os << " "<< task().get_fluent_name( unknown_vec().at( i ));
			if ( i && i % 4 == 0 )
				os << std::endl;
		}
    		os << std::endl; 
		os << "\tOR Clauses:" << std::endl;
		for ( unsigned i = 0; i < clauses_vec().size(); i++ ) {
			for ( unsigned j = 0; j < clauses_vec().at(i).size(); j++ ) {
				os << task().get_fluent_name( (unsigned)clauses_vec()[i].at( j ));
				if ( j < clauses_vec().at(i).size()-1 )
					os << " v ";
			}
			os << std::endl;
		}
	}

  void Belief::print()
  {

    std::cout << "Belief State:" << std::endl; 
    std::cout << "\nKnow Atoms:\n" << std::endl;
    for ( unsigned i = 0; i < known_vec().size(); i++ ) 
      std::cout << " "<< task().get_fluent_name( known_vec().at( i )) << std::endl;
    
    std::cout << "\nUnknown Atoms:\n" << std::endl;
    for ( unsigned i = 0; i < unknown_vec().size(); i++ ) 
      if ( unknown_set().isset(  unknown_vec().at(i) ) )
	std::cout << " "<< task().get_fluent_name( unknown_vec().at( i )) << std::endl;
     
    std::cout << "\nOR Clauses:\n" << std::endl;
    for ( unsigned i = 0; i < clauses_vec().size(); i++ ) {
      std::cout << " OR:" << std::endl;;
      for ( unsigned j = 0; j < clauses_vec().at(i).size(); j++ ) 
	std::cout << "     " << task().get_fluent_name( (unsigned)clauses_vec()[i].at( j )) << std::endl;
    }

  }

	bool Belief::effect_executes( PDDL::Operator* op )
	{
		Atom_Vec& precs = op->prec_vec();
		for ( unsigned i = 0; i < precs.size(); i++ )
		{
			if ( !known_set().isset(precs[i])
				&& !unknown_set().isset(precs[i]) )
				return false;
		}
		return true;
	}

  bool Belief::can_apply( PDDL::Operator* op )
  {
 
	std::vector<unsigned>::iterator it;

	for ( it = op->hard_prec_vec().begin();
		it != op->hard_prec_vec().end(); it++ )
		if ( !known_set().isset(*it)  )
			return false;

	return true;	
  }

  bool Belief::can_apply( unsigned op_idx )
  {
    PDDL::Operator* op = task().useful_ops()[op_idx];
    
    return can_apply( op );	
  }
  
  /* Op has no effect if the conditions do not hold for sure */
  bool Belief::has_no_effects( PDDL::Operator* op )
  {
    std::vector<unsigned>::iterator it;

    for (  it = op->prec_vec().begin(); it < op->prec_vec().end(); it++)
      {
	if ( !known_set().isset( *it ) && !unknown_set().isset( *it) )
	  return true; 
      }
    
    return false;	
  }

	bool	Belief::entails_goal()
	{
		NFF::Atom_Vec&	goal_atoms = task().goal_state();
		
		for ( unsigned i = 0; i < goal_atoms.size(); i++ )
		{	
#ifdef DEBUG
			std::cout << "Checking goal: ";
			sm_task.print_fluent( goal_atoms[i], std::cout );
			std::cout << std::endl;
#endif
			if ( !m_knowset->isset(goal_atoms[i] ) )
				return false;
		}
		return true;		
	}

	unsigned Belief::hash()
	{
		std::sort( known_vec().begin(), known_vec().end() );
		std::sort( unknown_vec().begin(), unknown_vec().end() );
		Hash_Key k;
		k.add( known_vec() );
		for ( unsigned i = 0; i < unknown_vec().size(); i++ )
			k.add( unknown_vec()[i] + sm_task.fluent_count() + 1 );
		return k;
	}

}

