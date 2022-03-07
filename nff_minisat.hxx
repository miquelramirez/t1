#ifndef __NFF_MINISAT__
#define __NFF_MINISAT__

//MRJ: C++ class which encapsulates HLP C functions

#include <cassert>
#include <minisat/core/Solver.h>
#include <vector>
#include <iostream>
#include <cstdio>
#include "nff_planner_stats.hxx"

using Minisat::lbool;

namespace NFF 
{

class SearchNode;

}

class MiniSAT
{
public:

	MiniSAT( NFF::SearchNode* n );
	MiniSAT( NFF::SearchNode* n, NFF::SearchNode* n2 );
	~MiniSAT();

	// MRJ: Adds unit clause to theory
	void	add_fact( int lit );
	void	retract_last_fact();
	void	add_clause( std::vector<int>& lits );
	void	retract_last_clause();
 
	// MRJ: Invokes DPLL procedure, returns 0 if theory is UNSAT,
	// returns 1 and puts true literals in model in model vector if SAT
	int	solve( std::vector<int>& model );
	int	solve();
protected:

	void	prepare_solver( NFF::SearchNode* n );
	void	prepare_solver( NFF::SearchNode* n, NFF::SearchNode* n2 );
	void	add_to_vect(  int lit, Minisat::vec<Minisat::Lit>& v );

protected:

	Minisat::Solver*		m_solver; // Solver instance
	Minisat::vec<Minisat::Lit>	m_facts;
	Minisat::CRef			m_clause_added;
};

inline void	MiniSAT::add_clause( std::vector<int>& lits )
{
	Minisat::vec<Minisat::Lit> c_lits;
	for ( int i = 0; i < lits.size(); i++ )
	{
		int lit = lits[i];
		int var = abs(lit);
	
		while ( var >= m_solver->nVars() ) m_solver->newVar();
		c_lits.push( Minisat::mkLit( var, lit <= 0 ) );
	}
	bool r = m_solver->addClause( c_lits, m_clause_added );
	std::cout << "Ref from SAT solver: " << m_clause_added << std::endl;	
	assert(r);
}

inline void	MiniSAT::retract_last_clause()
{
	if ( m_clause_added != UINT32_MAX )
		m_solver->removeClause( m_clause_added );
}

inline	void	MiniSAT::add_fact( int lit )
{
	add_to_vect( lit, m_facts );
}

inline  void	MiniSAT::retract_last_fact()
{
	m_facts.pop();
}

inline	void	MiniSAT::add_to_vect( int lit, Minisat::vec<Minisat::Lit>& v )
{
	int var = abs(lit);

	while ( var >= m_solver->nVars() ) m_solver->newVar();
	v.push( Minisat::mkLit( var, lit <= 0 ) );
}


inline	int	MiniSAT::solve(  )
{
	assert( m_solver != NULL );
	NFF::Planner_Stats&	stats = NFF::Planner_Stats::instance();
	stats.notify_num_clauses( m_solver->nClauses() );

	if( !(m_solver->simplify()) )
	  {
	    // Solved by unit propagation
	    return 0; // formula is insatisfiable
	  }

	lbool res = m_solver->solveLimited( m_facts );
	stats.notify_sat_solver_call();

	if ( res == Minisat::l_False ) 
	{
		return 0; // formula is insatisfiable
	}

	return 1;
}


#endif // nff_minisat.hxx
