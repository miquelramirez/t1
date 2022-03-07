#ifndef __THEORY_FRAGMENT__
#define __THEORY_FRAGMENT__

#include <minisat/core/Solver.h>
#include <minisat/mtl/Vec.h>
#include <minisat/mtl/Sort.h>
#include <cassert>

namespace NFF
{

	class Theory_Fragment
	{
	public:
		Theory_Fragment();
		Theory_Fragment( int op, Theory_Fragment* prev_layer );
		Theory_Fragment( int op, int nVars, int num_lits );
		~Theory_Fragment();

		int 	nVars() const;

		int				cond_var( unsigned i ) const;
		int				fluent_var( unsigned f ) const;
		unsigned			recover_fluent( int x ) const;
		unsigned			recover_cond_index( int x ) const;
		Minisat::vec<Minisat::Lit>&	unit_clauses();
		Minisat::vec<Minisat::CRef>&	clauses();
		Minisat::ClauseAllocator&	allocator() { return m_allocator; }
		Minisat::Clause&		get_clause( unsigned i );
		
		void				add_fact( int lit );

		bool				add_lit( int lit );
		void				finish_clause();
		void				cancel_clause();
		void				attach_clause_to_solver( unsigned k, Minisat::Solver* solv );
		unsigned			num_lits();
		unsigned			size();
	protected:
		
		void				add_to_vect( int lit, Minisat::vec<Minisat::Lit>& v );
		Minisat::lbool			model( int var );
	private:
		int				m_nVars;
		Minisat::vec<Minisat::Lit>	m_unit_clauses;
		Minisat::vec<Minisat::CRef>	m_clauses;
		Minisat::ClauseAllocator	m_allocator;
		Minisat::vec<Minisat::Lit>	m_lits; // temporary
		int				m_first_var;
		int				m_first_fluent_var;
		int				m_last_var;
		Minisat::vec<Minisat::lbool>	m_model;
		unsigned			m_num_lits;
	};

	inline	unsigned	Theory_Fragment::num_lits()
	{
		return m_num_lits;
	}

	inline	Minisat::lbool	Theory_Fragment::model( int var )
	{
		return m_model[var];
	}

	inline 	unsigned	Theory_Fragment::recover_cond_index( int x ) const
	{
		assert( x <= m_last_var );
		int idx = x - m_first_var;
		return idx;
	}

	inline	unsigned	Theory_Fragment::recover_fluent( int x ) const 
	{
		assert( x <= m_last_var );
		int f = x - m_first_fluent_var;
		if ( f <= 0 )
			return 0;
		return f;
	}

	inline	int		Theory_Fragment::cond_var( unsigned index ) const
	{
		assert( m_first_var != 0 );
		return m_first_var + index;
	}

	inline	int	Theory_Fragment::fluent_var( unsigned f ) const
	{
		assert( f > 0 );
		return m_first_fluent_var + f;
	}

	inline	int	Theory_Fragment::nVars() const
	{
		return m_nVars;
	}

	inline	Minisat::vec<Minisat::Lit>&	Theory_Fragment::unit_clauses()
	{
		return m_unit_clauses;
	}

	inline	Minisat::vec<Minisat::CRef>&	Theory_Fragment::clauses()
	{
		return m_clauses;
	}

	inline	Minisat::Clause& Theory_Fragment::get_clause( unsigned i )
	{
		return allocator()[ clauses()[i] ];
	}

	inline	void	Theory_Fragment::add_to_vect( int lit, Minisat::vec<Minisat::Lit>& v )
	{
		int var = abs(lit);
		assert( var <= m_last_var );	
		v.push( Minisat::mkLit( var, lit <= 0 ) );
	}

	inline	bool	Theory_Fragment::add_lit( int lit )
	{
		int var = abs(lit);
		assert( var <= m_last_var );

		Minisat::lbool value = ( lit > 0 ? Minisat::l_True : Minisat::l_False );
		if ( model( var  ) == value ) // absorbed
		{
			cancel_clause();
			return false;
		}
		m_lits.push( Minisat::mkLit( var, lit <= 0 ) );
		m_num_lits++;
		return true;
	}

	inline	void	Theory_Fragment::add_fact( int lit )
	{
		int var = abs(lit);
		assert( var <= m_last_var );
		m_unit_clauses.push( Minisat::mkLit( var, lit <= 0 ) );
		m_model[var ] = ( lit < 0 ? Minisat::l_False : Minisat::l_True );
	}
	
	inline 	void	Theory_Fragment::finish_clause( )
	{
		Minisat::sort(m_lits);
		assert( m_lits.size() > 1 );
		Minisat::CRef cr = m_allocator.alloc( m_lits, false );
		m_clauses.push(cr);	
		m_lits.clear();
	}

	inline	void	Theory_Fragment::cancel_clause() 
	{
		m_lits.clear();
	}

	inline void	Theory_Fragment::attach_clause_to_solver( unsigned k, Minisat::Solver* solv )
	{
		solv->addClause( allocator()[clauses()[k]] );
	}

}

#endif
