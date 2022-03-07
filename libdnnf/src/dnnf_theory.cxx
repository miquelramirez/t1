#include <libdnnf/dnnf_theory.hxx>
#include <fstream>
#include <libdnnf/string_ops.hxx>
#include <queue>
#include <cstring>

namespace DNNF {
DNNF_Theory::DNNF_Theory()
	: m_var_count( 0 ), m_cw( NULL ), m_cw_sz( 0 ), m_weights_compacted( false ),
	m_mod_mask( NULL ), m_var_present( NULL ), m_var_sz( 0 ),
	m_best_model( NULL ), m_bound_conflict( NULL ), m_cond_lits( NULL ),
	m_model_count( 0.0 )
{
	m_inf = std::numeric_limits<int>::max();
}

DNNF_Theory::~DNNF_Theory()
{
	for ( unsigned k = 0; k < m_dnnf_graph.size(); k++ )
	{
		switch ( m_dnnf_graph[k].type )
		{
		case AND:
		case OR:
		{
			break;
		}
		case LITERAL:
			break;
		}
	}

	if ( m_cw != NULL )
		delete [] m_cw;

	if ( m_mod_mask != NULL )
		delete [] m_mod_mask;

	if ( m_var_present != NULL )
		delete [] m_var_present;

	if ( m_best_model != NULL )
		delete [] m_best_model;

	if ( m_bound_conflict != NULL )
		delete [] m_bound_conflict;

	if ( m_cond_lits != NULL )
		delete [] m_cond_lits;
}

bool DNNF_Theory::load_from( std::ifstream& in )
{

	if ( in.fail() ) return false;

	char line_buffer[256000];
	std::string line;
	m_var_count = 0;
	int vi = 0;
	std::vector<int> indices;
	m_leaves_sz = 0;
	m_no_models = false;
		
	while ( !in.eof() )
	{
		in.getline( line_buffer, 255999, '\n' );
		line.assign( line_buffer );
		if ( line.empty() ) continue;
		TokenList tokens = split( line, ' ' );
		if ( tokens.empty() ) continue;
		if ( tokens[0] == "nnf" ) // prolog
		{
			if ( tokens.size() != 4 ) return false;
			int v;
			from_string( v, tokens[1], std::dec );
			from_string( m_edges, tokens[2], std::dec );
			from_string( m_var_count, tokens[3], std::dec ); 
			indices.resize( v );
			
		}
		else if ( tokens[0] == "L" ) // literal node
		{
			int lit;
			lit = atoi( tokens[1].c_str() );
			m_variables.insert( abs(lit) );
			Node n;
			n.type = LITERAL;
			n.w = 0;
			n.l = lit;
			m_leaves_sz++;//.push_back( vi );
			m_dnnf_graph.push_back( n );
			indices[vi] = m_dnnf_graph.size()-1;
			vi++;
		}
		else if ( tokens[0] == "O" ) // OR node
		{
			int nc = atoi( tokens[2].c_str() );
			assert( nc >= 0 );
			if ( nc == 0 )
			{
				m_no_models = true;
				return true;	
			}
			std::vector<int> children( nc, 0 );
			for ( unsigned ti = 3, ci = 0; ti < tokens.size(); ti++, ci++ )
				children[ci] = atoi( tokens[ti].c_str() );
			Node n;
			n.type = OR;
			n.min_child = 0;
			n.conf_var = atoi( tokens[1].c_str() );
			n.w = 0;
			n.w_0 = 0;
			n.l = 0;
			int ci = children.size()-1;
			n.children[1] = indices[children[ci--]];
			n.children[0] = indices[children[ci--]];
			m_dnnf_graph.push_back(n);
			for ( ; ci >= 0; ci-- )
			{
				Node n;
				n.type = OR;
				n.min_child = 0;
				n.w = 0;
				n.w_0 = 0;
				n.l = 0;
				n.children[1] = m_dnnf_graph.size()-1;
				n.children[0] = indices[children[ci]];
				m_dnnf_graph.push_back(n);
				
			}
			indices[vi] = m_dnnf_graph.size()-1;
			vi++;	
		}
		else if ( tokens[0] == "A" ) // AND node
		{
			int nc = atoi( tokens[1].c_str() );
			assert( nc > 0 );
			std::vector<int> children( nc, 0 );
			for ( unsigned ti = 2, ci = 0; ti < tokens.size(); ti++, ci++ )
				children[ci] = atoi( tokens[ti].c_str() );
			Node n;
			n.type = AND;
			n.min_child = 0;
			n.w = 0;
			n.w_0 = 0;
			n.l = 0;
			int ci = children.size()-1;
			n.children[1] = indices[children[ci--]];
			n.children[0] = indices[children[ci--]];
			m_dnnf_graph.push_back(n);
			for ( ; ci >= 0; ci-- )
			{
				Node n;
				n.type = AND;
				n.min_child = 0;
				n.w = 0;
				n.w_0 = 0;
				n.l = 0;
				n.children[1] = m_dnnf_graph.size()-1;
				n.children[0] = indices[children[ci]];
				m_dnnf_graph.push_back(n);
			}
			indices[vi] = m_dnnf_graph.size()-1;
			vi++;	
		}
		else
		{
			return false; // Error
		}
		
	}	
	m_mod_mask = new char[m_dnnf_graph.size()];
	memset( m_mod_mask, 0, m_dnnf_graph.size() );
	m_best_leaves = new char[m_leaves_sz];
	memset( m_best_leaves, 0, m_leaves_sz );
	m_cond_lits = new int[num_vars()+1];
	memset( m_cond_lits, 0, (num_vars()+1)*sizeof(int) );
	return true;
}

void DNNF_Theory::store_on( std::ostream& os )
{
	// prolog
	os << "nnf " << m_dnnf_graph.size() << " "
	   << m_edges << " "
	   << num_vars() << std::endl;

	for ( unsigned k = 0; k < m_dnnf_graph.size(); k++ )
	{
		if ( m_dnnf_graph[k].type == LITERAL )
			os << "L " << m_dnnf_graph[k].l << std::endl;
		else 
		{
			os << (( m_dnnf_graph[k].type == OR ) ? "O " : "A ");
			
			if ( m_dnnf_graph[k].type == OR )
				os << 0 << " "; // << m_dnnf_graph[k].conf_var << " ";
		
			os << 2/*m_dnnf_graph[k].nc*/ << " ";
			os << m_dnnf_graph[k].children[0] << " ";
			os << m_dnnf_graph[k].children[1];
			os << std::endl;
		}
	}
}

void DNNF_Theory::assign_weights()
{
	for ( unsigned k = 0; k < m_dnnf_graph.size(); k++ )
	{
		Node& n = m_dnnf_graph[k];
		if ( n.type == LITERAL )
		{
			n.w = lw(n.l);
		}
	}	
}


void DNNF_Theory::print_weights( std::ostream& os )
{
	for ( unsigned k = 0; k < m_dnnf_graph.size(); k++ )
	{
		Node& n = m_dnnf_graph[k];
		if ( n.type == LITERAL )
		{
                        os << "lit: " << k << ", w = " << n.w << " " << lw(n.l) << std::endl; // lw(n.l)
		}
	}	
}

void DNNF_Theory::allocate_weights( )
{
	m_cw_sz = (num_vars() + 1)*2;
	m_cw = new int[m_cw_sz];
	for ( int i = 0; i < m_cw_sz; i++ )
		m_cw[i] = 0;

	m_var_sz = num_vars() + 1;
	m_best_model = new char[m_var_sz];
	m_bound_conflict = new char[m_var_sz];
	m_var_present = new char[m_var_sz];
	memset( m_var_present, 0, m_var_sz );
	memset( m_best_model, 0, m_var_sz );
	memset( m_bound_conflict, 0, m_var_sz );

	for ( Schema::iterator i = m_variables.begin(); 
		i != m_variables.end(); i++ )
		m_var_present[*i] = 1;
	
	m_weights_compacted = true;
	
}

int DNNF_Theory::fast_compute_weight( int* lits )
{
	assert( m_weights_compacted );
	for ( int k = 0; k < m_leaves_sz; k++ )
	{
		Node& nk = m_dnnf_graph[k];
		int nx = abs(nk.l);
		assert( nk.type == LITERAL );
		if ( lits[nx] == 0 )
		{
			nk.w = lw(nk.l);
		}
		else
		{	
			if ( nk.l == lits[nx]*nx ) // make node true
			{
				nk.w = lw(nk.l);
			}
			else // make node false
			{
				nk.w = std::numeric_limits<int>::max();
			}
		}

	}
	const int top_sz = m_dnnf_graph.size();
	for ( int k = m_leaves_sz; k < top_sz; k++ )
	{
		Node& n = m_dnnf_graph[k];
		//assert ( n.nc > 0 );
		if ( n.type == AND )
		{
			Node& c0 = at(n.children[0]);//m_dnnf_graph[n.children[0]];
			Node& c1 = at(n.children[1]);//m_dnnf_graph[n.children[1]];
			n.w = std::add( c0.w, c1.w );
		}
		else /*if ( n.type == OR )*/
		{
			Node& c0 = at(n.children[0]);//m_dnnf_graph[n.children[0]];
			Node& c1 = at(n.children[1]);//m_dnnf_graph[n.children[1]];
			n.w = ( c1.w < c0.w ? c1.w : c0.w );
		}
	}

	return m_dnnf_graph.back().w;
}

char*	DNNF_Theory::best_model( int& w )
{
	w = compute_weight( m_cond_lits );

        compute_best_model( m_cond_lits );
	char* best_model = get_best_model();

	return best_model;
}



char*	DNNF_Theory::best_model_given_lit( int l, int& w )
{
	m_cond_lits[abs(l)] = std::sgn(l);
	w = compute_weight( m_cond_lits );

        compute_best_model( m_cond_lits );
	char* best_model = get_best_model();
	m_cond_lits[abs(l)] = 0;

	return best_model;
}

char* DNNF_Theory::best_model_given_lits( std::vector<int>& lits, int& w )
{
	for ( unsigned k = 0; k < lits.size(); k++ )
                if ( m_cond_lits[abs(lits[k])] != 0 )
		{
			if ( m_cond_lits[abs(lits[k])] != std::sgn(lits[k]) )
			{
				w = std::numeric_limits<int>::max();

                                for ( unsigned ki = 0; ki < lits.size(); ki++ )
                                {
                                        m_cond_lits[abs(lits[ki])] = 0;
                                        
                                }
				return NULL;
			}
		}
		else 
			m_cond_lits[abs(lits[k])] = std::sgn(lits[k]);
	w = compute_weight( m_cond_lits );
	compute_best_model( m_cond_lits );
	char* best_model = get_best_model();	
	for ( unsigned k = 0; k < lits.size(); k++ )
	{
                m_cond_lits[abs(lits[k])] = 0;

        }

	return best_model;
}

int DNNF_Theory::compute_weight( int* lits )
{
	assert( m_weights_compacted );
	for ( int k = 0; k < m_leaves_sz; k++ )
	{
		Node& nk = m_dnnf_graph[k];
		int nx = abs(nk.l);
		assert( nk.type == LITERAL );
		if ( lits[nx] == 0 )
		{
			nk.w = lw(nk.l);
		}
		else
		{	
			if ( nk.l == lits[nx]*nx ) // make node true
			{
				nk.w = lw(nk.l);
			}
			else // make node false
			{
				nk.w = std::numeric_limits<int>::max();
			}
		}

	}
	const int top_sz = m_dnnf_graph.size();
	for ( int k = m_leaves_sz; k < top_sz; k++ )
	{
		Node& n = m_dnnf_graph[k];
		//assert ( n.nc > 0 );
		if ( n.type == AND )
		{
			Node& c1 = m_dnnf_graph[n.children[0]];
			Node& c2 = m_dnnf_graph[n.children[1]];
			n.w = std::add( c1.w, c2.w );
		}
		else /*if ( n.type == OR )*/
		{
			Node& c0 = m_dnnf_graph[n.children[0]];
			Node& c1 = m_dnnf_graph[n.children[1]];
			if ( c1.w < c0.w )
			{
				n.w = c1.w;
				n.min_child = 1;
			}
			else
			{
				n.w = c0.w;
				n.min_child = 0;
			}
		}
	}

	return m_dnnf_graph.back().w;
}

void DNNF_Theory::compute_unsat_conflict( int* lits )
{
	memset( m_bound_conflict, 0, m_var_sz );	
	memset( m_mod_mask, 0, m_dnnf_graph.size() );	
	m_mod_mask[m_dnnf_graph.size()-1] = 1;
	for ( int k = m_dnnf_graph.size()-1; k >= 0; k-- )
	{
		if ( !m_mod_mask[k] ) continue;
		Node& n = m_dnnf_graph[k];

		int nx = abs(n.l);
		if ( n.type == LITERAL )
		{
			m_bound_conflict[nx] = std::sgn(n.l);
		}
		else
		{
			if ( n.type == AND )
			{
				for ( int i = 0; i < 2; i++ )
				{
					Node& ci = m_dnnf_graph[n.children[i]];
					if ( ci.w == std::numeric_limits<int>::max() )
					{
						m_mod_mask[n.children[i]] = 1;
						break;
					}
				}
			}
			else
			{
				m_mod_mask[n.children[0]] = 1;
				m_mod_mask[n.children[1]] = 1;
			}
		}
		
		m_mod_mask[k] = 0;
	}

}

void DNNF_Theory::update_w0()
{
	for ( unsigned k = 0; k < m_dnnf_graph.size(); k++ )
	{
		m_dnnf_graph[k].w_0 = m_dnnf_graph[k].w;
	}
}

void DNNF_Theory::compute_bound_conflict( int* lits, int UB )
{
	memset( m_bound_conflict, 0, m_var_sz );
	memset( m_mod_mask, 0, m_dnnf_graph.size() );	
	m_mod_mask[m_dnnf_graph.size()-1] = 1;
	for ( int k = m_dnnf_graph.size()-1; k >= 0; k-- )
	{
		if ( !m_mod_mask[k] ) continue;
		Node& n = m_dnnf_graph[k];
		int nx = abs(n.l);
		if ( n.w_0 >= UB ) 
		{
			m_mod_mask[k] = 0;
			continue;
		}
		if ( n.type == LITERAL )
		{
			if ( n.w_0 < n.w )
				m_bound_conflict[nx] = 1;
		}
		else
		{
			if ( n.type == AND )
			{
				Node& n1 = m_dnnf_graph[n.children[0]];
				Node& n2 = m_dnnf_graph[n.children[1]];
				
				bool A = ( std::add(n1.w, n2.w_0) >= UB);
				bool B = ( std::add(n2.w, n1.w_0) >= UB);
				
				if ( !A && !B )
				{
					m_mod_mask[n.children[0]] = 1;
					m_mod_mask[n.children[1]] = 1;
				}
				else if ( A && B ) // multiple conflict reasons
				{
					m_mod_mask[n.children[0]] = 1;
				}
				else
				{
					if ( A ) m_mod_mask[n.children[0]] = 1;
					if ( B ) m_mod_mask[n.children[1]] = 1;
				}
			}
			else
			{
				Node& left = m_dnnf_graph[n.children[0]];
				Node& right = m_dnnf_graph[n.children[1]];
				if ( left.w_0 < n.w )	
					m_mod_mask[ n.children[0] ] = 1;
				if ( right.w_0 < n.w )
					m_mod_mask[ n.children[1] ] = 1;
			}
		}
		m_mod_mask[k] = 0;	
	}
	
}


int DNNF_Theory::compute_weight()
{
	assert( m_weights_compacted );

	for ( int k = 0; k < m_leaves_sz; k++ )
	{
		Node& nk = m_dnnf_graph[k];
		assert( nk.type == LITERAL );
		nk.w_0 = lw(nk.l);

	}

	for ( unsigned k = m_leaves_sz; k < m_dnnf_graph.size(); k++ )
	{
		Node& n = m_dnnf_graph[k];
		if ( n.type == AND )
		{
			Node& c1 = m_dnnf_graph[n.children[0]];
			Node& c2 = m_dnnf_graph[n.children[1]];
			n.w_0 = std::add( c1.w_0, c2.w_0 );
		}
		else
		{
			n.w_0 = m_dnnf_graph[n.children[0]].w_0;
			Node& c1 = m_dnnf_graph[n.children[1]];
			if ( c1.w_0 < n.w_0 )
				n.w_0 = c1.w_0;
		}

	}

	return m_dnnf_graph.back().w_0;
}

void DNNF_Theory::mark_best_leaves( int* e )
{
	memset( m_mod_mask, 0, m_dnnf_graph.size() );
	memset( m_best_leaves, 0, m_leaves_sz );
	m_mod_mask[m_dnnf_graph.size()-1] = 1;
	for ( int k = m_dnnf_graph.size()-1; k >= 0; k-- )
	{
		if ( !m_mod_mask[k] ) continue;
		Node& n = m_dnnf_graph[k];

		if ( n.type == LITERAL )
			m_best_leaves[k] = 1;
		else
		{
			if ( n.type == AND )
			{
				m_mod_mask[n.children[0]] = 1;
				m_mod_mask[n.children[1]] = 1;
			}
			else
			{
				Node& c1 = m_dnnf_graph[n.children[0]];
				Node& c2 = m_dnnf_graph[n.children[1]];
				if ( c1.w == c2.w )
				{
					m_mod_mask[n.children[0]] = 1;
					m_mod_mask[n.children[1]] = 1;
				}
				else
					m_mod_mask[n.children[n.min_child]] = 1;
			}
		}

		
		m_mod_mask[k] = 0;
	}
}

int DNNF_Theory::compute_best_model( int* e, bool recompute_weight )
{
	memset( m_best_model, 0, m_var_sz );	
	memset( m_mod_mask, 0, m_dnnf_graph.size() );
	int w = m_dnnf_graph.back().w;

	m_mod_mask[m_dnnf_graph.size()-1] = 1;
	for ( int k = m_dnnf_graph.size()-1; k >= 0; k-- )
	{
		if ( !m_mod_mask[k] ) continue;
		Node& n = m_dnnf_graph[k];

		int nx = abs(n.l);
		if ( n.type == LITERAL )
		{
			m_best_model[nx] = ( e[nx] == 0 ? std::sgn(n.l) : e[nx] );
		}
		else
		{
			if ( n.type == AND )
			{
				m_mod_mask[n.children[0]] = 1;
				m_mod_mask[n.children[1]] = 1;
			}
			else
				m_mod_mask[n.children[n.min_child]] = 1;
		}

		
		m_mod_mask[k] = 0;
	}

	return w;
}
}
