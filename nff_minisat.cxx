#include "nff_search_node.hxx"
#include "nff_minisat.hxx"
#include "nff_options.hxx"
#include <map>

MiniSAT::MiniSAT( NFF::SearchNode* n )
	: m_solver( NULL )
{
	m_solver = new Minisat::Solver();
	prepare_solver( n );
}

MiniSAT::MiniSAT( NFF::SearchNode* n, NFF::SearchNode* n2 )
	: m_solver( NULL )
{
	m_solver = new Minisat::Solver();
	prepare_solver( n, n2 );
}

MiniSAT::~MiniSAT()
{
	delete m_solver;
}

int	MiniSAT::solve( std::vector<int>& model )
{
	assert( m_solver != NULL );
	NFF::Planner_Stats&	stats = NFF::Planner_Stats::instance();
	stats.notify_num_clauses( m_solver->nClauses() );

	if( !(m_solver->simplify()) )
	{
    		return 0; // formula is insatisfiable
      	}
	stats.notify_sat_solver_call();
	lbool res = m_solver->solveLimited( m_facts );

	if ( res == Minisat::l_False ) 
	{
		return 0; // formula is insatisfiable
	}
	NFF_Options& opt = NFF_Options::instance();
	
	if ( opt.dynamic_samples() )
	{
		for (int i = 0; i < m_solver->nVars(); i++)
			if (m_solver->model[i] != Minisat::l_Undef)
				model.push_back( ( m_solver->model[i] == Minisat::l_False ? -i : i ) );
			else
				model.push_back( 0 );
	}

	return 1;
}


void MiniSAT::prepare_solver( NFF::SearchNode* n )
{
	// Add vars to solver
	while ( m_solver->nVars() <= n->theory_step->nVars() )
		m_solver->newVar();

	// Now attach clauses and stack facts
	NFF::SearchNode* tmp = n;
	while ( tmp != NULL )
	{
		assert( tmp->theory_step != NULL );
		for ( unsigned k = 0; k < tmp->theory_step->unit_clauses().size(); k++ )
			m_facts.push( tmp->theory_step->unit_clauses()[k] );
		for ( unsigned k = 0; k < tmp->theory_step->clauses().size(); k++ )
			tmp->theory_step->attach_clause_to_solver( k, m_solver );
		tmp = tmp->father;
	}	
}

void MiniSAT::prepare_solver( NFF::SearchNode* n, NFF::SearchNode* n2 )
{
	// Add vars to solver
	int nVarsA = n->theory_step->nVars();
	while ( m_solver->nVars() <= nVarsA )
		m_solver->newVar();

	// Now attach clauses and stack facts
	NFF::SearchNode* tmp = n;
	while ( tmp != NULL )
	{
		assert( tmp->theory_step != NULL );
		for ( unsigned k = 0; k < tmp->theory_step->unit_clauses().size(); k++ )
			m_facts.push( tmp->theory_step->unit_clauses()[k] );
		for ( unsigned k = 0; k < tmp->theory_step->clauses().size(); k++ )
			tmp->theory_step->attach_clause_to_solver( k, m_solver );
		tmp = tmp->father;
	}
	int nVarsB = n2->theory_step->nVars();
	while ( m_solver->nVars() <= (nVarsA + nVarsB + 1) )
		m_solver->newVar();
	tmp = n2;
	Minisat::vec<Minisat::Lit> tmpLits;
	while ( tmp != NULL )
	{
		assert( tmp->theory_step != NULL );
		for ( unsigned k = 0;  k < tmp->theory_step->unit_clauses().size(); k++ )
		{
			Minisat::Lit l = tmp->theory_step->unit_clauses()[k];
			Minisat::Lit lp = Minisat::mkLit( Minisat::var(l)+(nVarsA+1), Minisat::sign(l) );
			m_facts.push( lp );
		}
		for ( unsigned k = 0; k < tmp->theory_step->clauses().size(); k++ )
		{
			Minisat::Clause& c = tmp->theory_step->get_clause(k);
			for ( int i = 0; i < c.size(); i++ )
			{
				Minisat::Lit l = c[i];
				tmpLits.push( Minisat::mkLit( Minisat::var(l)+(nVarsA+1), Minisat::sign(l) ) );
			}
			m_solver->addClause(tmpLits);
			tmpLits.clear();
		}
		tmp = tmp->father;
	}
	PDDL::Task&	task = PDDL::Task::instance();
	for ( unsigned f = 1; f < task.fluent_count()-1; f++ )
	{
		if ( !task.fluents()[f]->is_pos() ) continue;

		int p1 = f;
		int p2 = f + (nVarsA+1);
		
		// p1 -> p2
		tmpLits.push( Minisat::mkLit( p1, true ) );
		tmpLits.push( Minisat::mkLit( p2 ) );
		m_solver->addClause(tmpLits);
		tmpLits.clear();
		// p2 -> p1
		tmpLits.push( Minisat::mkLit( p2, true ) );
		tmpLits.push( Minisat::mkLit( p1 ) );
		m_solver->addClause(tmpLits);
		tmpLits.clear();
	}
	Minisat::vec<Minisat::Lit> eqVars;
#ifdef DEBUG
	std::map<int,int> eqFluents;
#endif 
	for ( unsigned f = 1; f < task.fluent_count()-1; f++ )
	{
		if ( n->b->known_set().isset(f) && n2->b->known_set().isset(f) )
			continue; // f is known to be true in both
		if ( !n->b->unknown_set().isset(f) && !n2->b->unknown_set().isset(f) )
			continue; // f is known to be false in both
		
		if ( !task.fluents()[f]->is_pos() ) continue;
		int p_k = n->theory_step->fluent_var(f);//(n->timestep*task.fluent_count())+f;
		int p_m = (nVarsA+1) + n2->theory_step->fluent_var(f);//(n2->timestep*task.fluent_count()+f);
		
		Minisat::Var eqVar = m_solver->newVar();
#ifdef DEBUG
		eqFluents[eqVar] = f;
#endif
		eqVars.push( Minisat::mkLit( eqVar, true ) );		

		// p_k & p_m -> eq_p
		tmpLits.push( Minisat::mkLit( p_k, true ) );
		tmpLits.push( Minisat::mkLit( p_m, true ) );
		tmpLits.push( Minisat::mkLit( eqVar ) );
		m_solver->addClause(tmpLits);
		tmpLits.clear();

		// p_k & ~p_m -> ~eq_p
		tmpLits.push( Minisat::mkLit( p_k, true  ) );
		tmpLits.push( Minisat::mkLit( p_m ) );
		tmpLits.push( Minisat::mkLit( eqVar, true ) );
		m_solver->addClause(tmpLits);
		tmpLits.clear();
				
		// ~p_k & p_m -> ~eq_p
		tmpLits.push( Minisat::mkLit( p_k) );
		tmpLits.push( Minisat::mkLit( p_m, true ) );
		tmpLits.push( Minisat::mkLit( eqVar, true ) );
		m_solver->addClause(tmpLits);
		tmpLits.clear();
				
		// ~p_k & ~p_m -> eq_p
		tmpLits.push( Minisat::mkLit( p_k ) );
		tmpLits.push( Minisat::mkLit( p_m ) );
		tmpLits.push( Minisat::mkLit( eqVar ) );
		m_solver->addClause(tmpLits);
		tmpLits.clear();
	}
	m_solver->addClause( eqVars );
}
