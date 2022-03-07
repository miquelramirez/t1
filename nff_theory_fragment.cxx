#include "nff_theory_fragment.hxx"
#include "PDDL.hxx"

namespace NFF
{
	Theory_Fragment::Theory_Fragment()
		: m_allocator( 1024 ), m_num_lits(0)
	{
		PDDL::Task&	task = PDDL::Task::instance();
		m_first_var = 0;
		m_first_fluent_var = m_first_var;
		m_last_var = m_first_fluent_var + (task.fluent_count()-1);
		m_nVars = m_last_var + 1;
		m_model.growTo( m_nVars );
		for ( int i = 0; i < m_model.size(); i++ )
			m_model[i] = Minisat::l_Undef;
	}

	Theory_Fragment::Theory_Fragment( int op, Theory_Fragment* prev_layer )
		: m_allocator( 1024 ), m_num_lits(0)
	{
		PDDL::Task&	task = PDDL::Task::instance();
		unsigned num_conds = task.useful_ops()[op]->same_op_vec().size();	
		m_first_var = prev_layer->nVars();
		m_first_fluent_var = m_first_var + num_conds;
		m_last_var = m_first_fluent_var + (task.fluent_count()-1);
		m_nVars = m_last_var + 1;
		m_model.growTo( m_nVars );
		assert( m_nVars > prev_layer->nVars() );
		for ( int i = 0; i <= prev_layer->nVars(); i++ )
			m_model[i] = prev_layer->model(i);
		for ( int i = 0; i < m_model.size(); i++ )
			m_model[i] = Minisat::l_Undef;

	}

	Theory_Fragment::Theory_Fragment( int op, int nVars, int num_lits )
		: m_allocator( 1024 ), m_num_lits(0)
	{
		PDDL::Task&	task = PDDL::Task::instance();
		unsigned num_conds = task.useful_ops()[op]->same_op_vec().size();	
		m_first_var = nVars;
		m_first_fluent_var = m_first_var + num_conds;
		m_last_var = m_first_fluent_var + (task.fluent_count()-1);
		m_nVars = m_last_var + 1;
		m_model.growTo( m_nVars );
		for ( int i = 0; i < m_model.size(); i++ )
			m_model[i] = Minisat::l_Undef;
	}

	Theory_Fragment::~Theory_Fragment()
	{
	}

	unsigned	Theory_Fragment::size()
	{
		unsigned numBytes = sizeof(m_nVars);
		numBytes += sizeof(m_unit_clauses);
		numBytes += sizeof(m_clauses);
		numBytes += sizeof(m_allocator);
		numBytes += sizeof(m_lits);
		numBytes += sizeof(m_first_var);
		numBytes += sizeof(m_first_fluent_var);
		numBytes += sizeof(m_last_var);
		numBytes += sizeof(m_model);
		numBytes += sizeof(m_num_lits);
		numBytes += m_unit_clauses.size()*sizeof(Minisat::Lit);
		numBytes += m_clauses.size()*sizeof(Minisat::Lit);
		numBytes += m_model.size()*sizeof(Minisat::lbool);
		numBytes += m_allocator.get_capacity()*sizeof(unsigned);
		return numBytes;
	}	

}
