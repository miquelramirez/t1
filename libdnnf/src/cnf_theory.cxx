#include <fstream>
#include <cassert>
#include <libdnnf/string_ops.hxx>
#include <libdnnf/cnf_theory.hxx>
#include <queue>
#include <cstdlib>
#include <cstring>


namespace DNNF {

CNF_Theory::CNF_Theory()
	: m_clause_count( 0 ), m_num_vars( 0 ), m_first_msh( 0 ), m_factor( 1 ), m_file_vars( 0 )
{
	
}

CNF_Theory::CNF_Theory( const CNF_Theory& other )
{
	m_clause_count = other.m_clause_count;
	m_num_vars = other.m_num_vars;
	m_file_vars = other.m_file_vars;
	m_vars = other.m_vars;
	m_literals = other.m_literals;
	m_clauses = other.m_clauses;
	m_weights = other.m_weights;
	m_lw = other.m_lw;
	m_first_msh = other.m_first_msh;
	m_valid = other.m_valid;
	m_is_projected = other.m_is_projected;
	m_factor = other.m_factor;
}

const CNF_Theory& CNF_Theory::operator=( const CNF_Theory& other )
{
	m_clause_count = other.m_clause_count;
	m_num_vars = other.m_num_vars;
	m_file_vars = other.m_file_vars;
	m_vars = other.m_vars;
	m_literals = other.m_literals;
	m_clauses = other.m_clauses;
	m_weights = other.m_weights;
	m_lw = other.m_lw;
	m_first_msh = other.m_first_msh;
	m_valid = other.m_valid;
	m_is_projected = other.m_is_projected;
	m_factor = other.m_factor;
	
	return *this;
}
	

CNF_Theory::~CNF_Theory()
{
}

void CNF_Theory::add_clause( const Clause& c )
{
	if ( c.size() > 0 )
	{
		for ( Clause::const_iterator li = c.begin();
			li != c.end(); li++ )
		{
			std::pair<Schema::iterator, bool> res = m_vars.insert( abs(*li) );
			if ( res.second ) m_num_vars++;
		}
	}

	m_clauses.push_back( c );
	m_clause_count++;
}

bool CNF_Theory::load_from_weighted_DIMACS( std::ifstream& in )
{
	
	if ( in.fail() ) return false;

	char line_buffer[2048];
	std::string line;
	m_clause_count = 0;
	bool prolog_parsed = false;
	int  expected_clauses = 0;

	std::set<Clause> unique_clauses;
	int hard_clause_value = -1;
	m_file_vars = 0;

	while( !in.eof() )
	{
		in.getline( line_buffer, 2047, '\n' );
		line.assign( line_buffer );
		TokenList tokens = split( line, ' ' );
		
		if ( tokens.empty() ) continue; // empty line	
		if ( tokens[0][0] == 'c' ) continue;// comment found
		
		if ( tokens[0][0] == 'p' ) // prolog found
		{
			if ( tokens.size() < 4 )
				return false;

			if ( tokens[1] != "wcnf" )
				return false;	

			//from_string( m_num_vars, tokens[2], std::dec ); 			
			//from_string( expected_clauses, tokens[3], std::dec );
			m_num_vars = atoi( tokens[2].c_str() );
			m_file_vars = m_num_vars;
			expected_clauses = atoi( tokens[3].c_str() );
			if ( tokens.size() == 5 )
				set_top( atoi( tokens[4].c_str() ) );

			prolog_parsed = true;
		}
		else // clause
		{
			if ( !prolog_parsed )
				return false;

			TokenList tokens = split( line, ' ' );
			// Last token should be zero for marking 
			// end of clause
			Clause new_clause;
			for ( unsigned k = 1; k < tokens.size()-1; k++ )
			{
				int literal;
				//from_string( literal, tokens[k], std::dec );
				literal = atoi( tokens[k].c_str() );
				m_vars.insert( abs( literal ) );
				m_literals.insert( literal );
				new_clause.insert( literal );
			}
			bool trivial = false;
			for ( Clause::iterator li = new_clause.begin();
				li != new_clause.end(); li++ )
			{
				if ( new_clause.find( -1* (*li ) ) != new_clause.end() )
				{
					std::cout << "trivial clause... "; 
					for ( unsigned k = 0; k < tokens.size()-1; k++ )
						std::cout << tokens[k] << " ";
					std::cout << " skipping " << std::endl;
					trivial = true;
				}
			}
			if ( !trivial )
			{
				int w;
				from_string( w, tokens[0], std::dec );
				
				if ( new_clause.size() == 1 ) // a single literal
				{
					int lit = *new_clause.begin();
					set_weight( -lit, w );
				}
				else
				{
					unique_clauses.insert( new_clause );
					m_weights[new_clause] +=  w;
	
					m_clause_count++;
				}
			}
		}
	}

	m_clauses.assign( unique_clauses.begin(), unique_clauses.end() );
	
	prepare_projected_info();
	return true;
	
}

void CNF_Theory::check_valid()
{
	if ( (int)m_valid.size() < max_var_name()+1 )
		m_valid.resize( max_var_name()+1 );
	
	
	memset( &m_valid[0], 0, sizeof(int)*m_valid.size() );
	for ( unsigned k = 0; k < clauses().size(); k++ )
	{
		Clause& c = clauses()[k];
		for ( Clause::iterator li = c.begin();
			li != c.end(); li++ )
			m_valid[abs(*li)] = 1;
	}	
}

int CNF_Theory::make_Max_SAT( int last_old )
{
	std::vector<Clause> orig_C( clauses().begin(), clauses().end() );
	int varname = last_old;
	m_clauses.clear();	
	m_literals.clear();
	m_vars.clear();
	m_num_vars = 0;	
	m_clause_count = 0;

	for ( unsigned k = 0; k < orig_C.size(); k++, varname++ )
	{
		// Xi v Ci
		Clause new_ci( orig_C[k].begin(), orig_C[k].end() );
		new_ci.insert(varname);
		add_clause( new_ci );
		set_weight(varname, m_weights[orig_C[k]]);
	}
	m_weights.clear();	
	return varname;
}

bool CNF_Theory::load_from_MinCostSAT( std::ifstream& in )
{
	if ( in.fail() ) return false;

	char line_buffer[1024*1024];
	std::string line;
	m_clause_count = 0;
	bool prolog_parsed = false;
	int  expected_clauses = 0;
	std::set<Clause> unique_clauses;
	
	while( !in.eof() )
	{
		in.getline( line_buffer, (1024*1024) - 1, '\n' );
		line.assign( line_buffer );
		TokenList tokens = split( line, ' ' );
		
		if ( tokens.empty() )
		{
			tokens = split( line, '\t' );
			if ( tokens.empty() )
				continue; // empty line	
		}
		if ( tokens[0][0] == 'c' ) continue;// comment found
		
		if ( tokens[0] == "min:" )
		{
			for ( unsigned i = 1; i < tokens.size()-2; i+=2 )
			{
				int lit = atoi( tokens[i].c_str() );
			       	int w = atoi( tokens[i+1].c_str() );
				set_weight( lit, w );
			}
			continue;
		}
		
		if ( tokens[0][0] == 'p' ) // prolog found
		{
			if ( tokens.size() != 4 )
				return false;

			if ( tokens[1] != "cnf" )
				return false;	

			//from_string( m_num_vars, tokens[2], std::dec ); 			
			//from_string( expected_clauses, tokens[3], std::dec );
			m_num_vars = atoi( tokens[2].c_str() );
			m_file_vars = m_num_vars;
			expected_clauses = atoi( tokens[3].c_str() );

			prolog_parsed = true;
		}
		else // clause
		{
			if ( !prolog_parsed )
				return false;

			TokenList tokens = split( line, ' ' );
			if ( tokens.empty() ) 
				tokens = split( line, '\t' );
			// Last token should be zero for marking 
			// end of clause
			Clause new_clause;
			for ( unsigned k = 0; k < tokens.size()-1; k++ )
			{
				int literal;
				//from_string( literal, tokens[k], std::dec );
				literal = atoi( tokens[k].c_str() );
				if ( literal == 0 )
					return false;	
				m_vars.insert( abs( literal ) );
				m_literals.insert( literal );
				new_clause.insert( literal );
			}
			bool trivial = false;
			for ( Clause::iterator li = new_clause.begin();
				li != new_clause.end(); li++ )
			{
				if ( new_clause.find( -1* (*li ) ) != new_clause.end() )
				{
					std::cout << "trivial clause... "; 
					for ( unsigned k = 0; k < tokens.size()-1; k++ )
						std::cout << tokens[k] << " ";
					std::cout << " skipping " << std::endl;
					trivial = true;
				}
			}

			if (!trivial)
			{
			
				unique_clauses.insert( new_clause );
				m_weights[new_clause] = m_weights[new_clause]+1;
				m_clause_count++;
			}
		}
	}

	m_clauses.assign( unique_clauses.begin(), unique_clauses.end() );
	prepare_projected_info();
	return true;
	
}

bool CNF_Theory::load_from_DIMACS( std::ifstream& in )
{
	if ( in.fail() ) return false;

	char line_buffer[2048];
	std::string line;
	m_clause_count = 0;
	bool prolog_parsed = false;
	int  expected_clauses = 0;
	std::set<Clause> unique_clauses;
	
	while( !in.eof() )
	{
		in.getline( line_buffer, 2047, '\n' );
		line.assign( line_buffer );
		TokenList tokens = split( line, ' ' );
		
		if ( tokens.empty() )
		{
			tokens = split( line, '\t' );
			if ( tokens.empty() )
				continue; // empty line	
		}
		if ( tokens[0][0] == 'c' ) continue;// comment found
		
		if ( tokens[0][0] == 'p' ) // prolog found
		{
			if ( tokens.size() != 4 )
				return false;

			if ( tokens[1] != "cnf" )
				return false;	

			//from_string( m_num_vars, tokens[2], std::dec ); 			
			//from_string( expected_clauses, tokens[3], std::dec );
			m_num_vars = atoi( tokens[2].c_str() );
			m_file_vars = m_num_vars;
			expected_clauses = atoi( tokens[3].c_str() );

			prolog_parsed = true;
		}
		else // clause
		{
			if ( !prolog_parsed )
				return false;

			TokenList tokens = split( line, ' ' );
			if ( tokens.empty() ) 
				tokens = split( line, '\t' );
			// Last token should be zero for marking 
			// end of clause
			Clause new_clause;
			for ( unsigned k = 0; k < tokens.size()-1; k++ )
			{
				int literal;
				//from_string( literal, tokens[k], std::dec );
				literal = atoi( tokens[k].c_str() );
				if ( literal == 0 )
					return false;	
				m_vars.insert( abs( literal ) );
				m_literals.insert( literal );
				new_clause.insert( literal );
			}
			bool trivial = false;
			for ( Clause::iterator li = new_clause.begin();
				li != new_clause.end(); li++ )
			{
				if ( new_clause.find( -1* (*li ) ) != new_clause.end() )
				{
					std::cout << "trivial clause... "; 
					for ( unsigned k = 0; k < tokens.size()-1; k++ )
						std::cout << tokens[k] << " ";
					std::cout << " skipping " << std::endl;
					trivial = true;
				}
			}

			if (!trivial)
			{
			
				unique_clauses.insert( new_clause );
				m_weights[new_clause] = m_weights[new_clause]+1;
				m_clause_count++;
			}
		}
	}

	m_clauses.assign( unique_clauses.begin(), unique_clauses.end() );
	prepare_projected_info();
	return true;
}

void CNF_Theory::store_as_DIMACS( std::ofstream& os )
{
	// prolog
	os << "p cnf " << max_var_name() << " " << clauses().size();
	os << std::endl;

	for ( Clause_Set::iterator ci = m_clauses.begin();
		ci != m_clauses.end(); ci++ )
	{
		for ( Clause::iterator l = ci->begin();
			l != ci->end(); l++ )
			os << *l << " ";
		os << "0" << std::endl;
	}
}

void CNF_Theory::store_as_weighted_DIMACS( std::ofstream& os )
{
	os << "p wcnf " << max_var_name() << " " << num_clauses();
	os << std::endl;

	for ( Clause_Set::iterator ci = m_clauses.begin();
		ci != m_clauses.end(); ci++ )
	{
		os << m_weights[*ci] << " ";
		for ( Clause::iterator l = ci->begin();
			l != ci->end(); l++ )
			os << *l << " ";
		os << "0" << std::endl;
	}

}

void CNF_Theory::store_for_C2D( std::ofstream& os )
{
	os << "c variable names changed to fit c2d requirements" << std::endl;
	os << "p cnf " << max_var_name() << " " << m_clauses.size();
	os << std::endl;

	// search for biggest variable idx
	int max_v_idx = 0;
	
	for ( Schema::iterator vi = m_vars.begin();
		vi != m_vars.end(); vi++ )
		if ( *vi > max_v_idx ) max_v_idx = *vi;
	assert( max_v_idx != 0 );

	int c2d_idx = 1;
	int* trans = new int[max_v_idx+1];
	for ( Schema::iterator vi = m_vars.begin();
		vi != m_vars.end(); vi++ )
		trans[*vi] = c2d_idx++;


	for ( Clause_Set::iterator ci = m_clauses.begin();
		ci != m_clauses.end(); ci++ )
	{
		for ( Clause::iterator l = ci->begin();
			l != ci->end(); l++ )
		{
			int c2d_v_i = trans[ abs(*l ) ];
			if ( *l < 0 )
				os << -1*c2d_v_i << " ";
			else
				os << c2d_v_i << " ";
		}
		os << "0" << std::endl;
	}

	delete [] trans;

}

void CNF_Theory::inherit_literal_weights( CNF_Theory& father )
{
	for ( Literal_Set::iterator li = literals().begin();
		li != literals().end(); li++ )
		set_weight( *li, father.weight( *li ) );
}

int CNF_Theory::max_var_name()
{
	int max_var_name = 0;

	for ( Schema::iterator vi = vars().begin();
		vi != vars().end(); vi++ )
		if ( *vi > max_var_name ) max_var_name = *vi;	

	return max_var_name;
}


bool CNF_Theory::unit_propagation( int l, CNF_Theory& tau )
{
	std::queue<int> unit_clauses;
	unit_clauses.push(l);
	CNF_Theory temp = *this;

	std::set<int> uc;
	get_unit_clauses( uc );

	while ( !unit_clauses.empty() )
	{
		int elim = 0;
		int simp = 0;
		int unchanged = 0;
		int wuc = 0;
		int literal = unit_clauses.front();
		unit_clauses.pop();
		for ( Clause_Set::iterator ci = temp.clauses().begin();
			ci != temp.clauses().end(); ci++ )
		{
			if ( ci->find( literal ) != ci->end() )
			{
				// clause satisfied - it is eliminated
				elim++;
				continue;
			}
			else if ( ci->find( -1*literal ) != ci->end() )
			{
				// remove literal
				Clause new_c( *ci );
				new_c.erase(-1*literal);
				if (new_c.size() == 1 )
				{
					int nuc =  *new_c.begin();
					if ( !temp.is_weighted(nuc) )
					{
						wuc++;
						unit_clauses.push( nuc );
					}
				}
				tau.add_clause( new_c );
				simp++;
			}
			else
			{
				tau.add_clause( *ci );
				unchanged++;
			}
		}
		temp = tau;
		tau.clear();
	}

	tau = temp;	
	
	for ( Literal_Set::iterator li = tau.m_literals.begin();
	 	li != tau.m_literals.end(); li++ )
		tau.set_weight( *li, weight(*li) );
	return true;	
}

void CNF_Theory::load_projection_info( std::string fname )
{
	prepare_projected_info();
	std::ifstream infile(fname.c_str() );

	if ( infile.fail() )
	{
		std::cerr << "Could not load projection info from file " << fname << std::endl;
		std::cerr << "Aborting..." << std::endl;
		std::exit(1);
	}

	int nvars_proj;
	infile >> nvars_proj;
	int c = 0;
	while (!infile.eof())
	{
		int x;
		infile >> x;
		set_projected(x, 1);
		c++;
	}
	std::cout << c << " variables are going to be projected" << std::endl;
}

void CNF_Theory::prepare_projected_info()
{
	int max_var = max_var_name();
	m_is_projected.resize( max_var + 1 );
	memset( &m_is_projected[0], 0, sizeof(int)*m_is_projected.size() );
}

bool CNF_Theory::unweighted()
{
	return m_lw.size() == 0;	
}
}
