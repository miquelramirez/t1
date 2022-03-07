#ifndef __CNF_THEORY__
#define __CNF_THEORY__

#include <iosfwd>
#include <set>
#include <vector>
#include <map>
#include <cassert>
#include <cmath>


namespace DNNF
{

typedef std::set<int>            Schema;
typedef std::set<int>            Literal_Set;
typedef std::set<int>            Clause;
typedef std::vector<Clause>      Clause_Set;
typedef std::map<Clause, int>    Weight_Structure;
typedef std::map<int, int>       Literal_Weights;

class CNF_Theory
{


public:

	CNF_Theory();
	CNF_Theory( const CNF_Theory& other );
	const CNF_Theory& operator=(const CNF_Theory& );
	
	~CNF_Theory();

	int 			num_clauses() const;
	int 			file_nvars() const;
	void 			set_file_nvars( int nv );	
	int 			num_vars() const;
	double 			vcr() const;
	double 			cvr() const;
	bool 			is_weighted( int x ); 
	int 			weight( const Clause& c);
	void 			set_weight( const Clause& c, int w );
	void 			set_weight( int literal, int w );
	int  			weight( int literal );
	std::map<int,int>& 	weights();
	void 			set_weights( std::map<int,int>& ws );
	
	Clause_Set& 		clauses();

	Schema& 		vars();

	Literal_Set& 		literals();

	void 			add_clause( const Clause& c );

	bool 			unit_propagation( int literal, CNF_Theory& tau );	
	
	bool 			load_from_DIMACS( std::ifstream& is );
	bool 			load_from_weighted_DIMACS( std::ifstream& is );
	bool 			load_from_MinCostSAT( std::ifstream& is );

	void 			store_as_DIMACS( std::ofstream& os );
	void 			store_as_weighted_DIMACS( std::ofstream& os );
	void 			store_for_C2D( std::ofstream& os );
	// introduce dummy variable for each clause to make
	// the problem satisfiable
	int 			make_Max_SAT( int v0);
	int 			max_var_name();	
	void 			inherit_literal_weights( CNF_Theory& father );
	
	void 			check_valid();
	
	bool 			valid( int x ); // returns true if x appears in some clause

	unsigned 		unit_clauses(); 

	void 			get_unit_clauses( std::set<int>& unit_clauses );

	int 			max_clause_size();
	
	int 			first_handle();
	void 			clear();

	void  			prepare_projected_info();
	void  			set_projected( int x, int p );
	
	int   			is_projected( int x );
	bool  			var_is_weighted( int x );
	int 			weight_factor();
	void 			set_weight_factor( int w );

	void 			set_top( int top );
	int 			top( );
	bool 			unweighted();
	void 			load_projection_info( std::string fname );

	void 			add_pure_literal( int x );
	std::vector<int>& 	pure_literals();
		
protected:
	
	int                m_clause_count;
	int                m_num_vars;

	Schema             m_vars;
	Literal_Set        m_literals;
	Clause_Set         m_clauses;
	Weight_Structure   m_weights;
	Literal_Weights    m_lw;
	int                m_first_msh; // first Max-SAT clause handle
	std::vector<int>   m_valid;
	std::vector<int>   m_is_projected;
	std::vector<int>   m_pure_lits;
	int                m_factor;
	int                m_top_value;
	int                m_file_vars;
};

// In-line methods
inline int	CNF_Theory::num_clauses() const 
{ 
	return m_clause_count; 
}

inline int	CNF_Theory::file_nvars() const
{
	return m_file_vars;
}

inline void 	CNF_Theory::set_file_nvars( int nv )
{
	m_file_vars = nv;
}

inline	int	CNF_Theory::num_vars() const
{
	return m_num_vars;
}

inline	double	CNF_Theory::vcr() const
{
	return (double)m_num_vars/(double)m_clause_count;
}

inline double	CNF_Theory::cvr() const
{
	return (double)m_clause_count/(double)m_num_vars;
}

inline bool	CNF_Theory::is_weighted( int x )
{
	return weight(x) != 0 || weight(-x) != 0;
}

inline int	CNF_Theory::weight( const Clause& c )
{
	return m_weights[c];
}

inline void	CNF_Theory::set_weight( const Clause& c, int w )
{
	m_weights[c] = w;
}

inline void	CNF_Theory::set_weight( int literal, int w )
{
	m_lw[literal] = w;
}

inline int	CNF_Theory::weight( int literal ) 
{
	return m_lw[literal];
}

inline std::map<int,int>& CNF_Theory::weights()
{
	return m_lw;
}

inline	void	CNF_Theory::set_weights( std::map<int,int>& ws )
{
	m_lw = ws;
}

inline	Clause_Set&	CNF_Theory::clauses()
{
	return m_clauses;
}

inline	Schema&		CNF_Theory::vars()
{
	return m_vars;
}

inline	Literal_Set&	CNF_Theory::literals()
{
	return m_literals;
}

inline	bool		CNF_Theory::valid( int x )
{
	assert( x >= 0 && x < (int)m_valid.size() );
	return m_valid[x] > 0;
}

inline	unsigned	CNF_Theory::unit_clauses()
{
	unsigned nuc = 0;
	for ( Clause_Set::iterator ci = clauses().begin();
		ci != clauses().end(); ci++ )
		if ( ci->size() == 1 )
			nuc++;
	return nuc;
}

inline	void		CNF_Theory::get_unit_clauses( std::set<int>& unit_clauses )
{
	for ( Clause_Set::iterator ci = clauses().begin();
		ci != clauses().end(); ci++ )
		if ( ci->size() == 1 )
			unit_clauses.insert( *(ci->begin()) );
}

inline	int		CNF_Theory::max_clause_size()
{
	int max_clause_size = 0;
	for ( unsigned k = 0; k < clauses().size(); k++ )
		if ( (int)clauses()[k].size() > max_clause_size )
			max_clause_size = clauses()[k].size();
	return max_clause_size;
}

inline	int		CNF_Theory::first_handle()
{
	return m_first_msh;
}

inline	void		CNF_Theory::clear()
{
	m_clauses.clear();
	m_literals.clear();
	m_vars.clear();
	m_weights.clear();
	m_lw.clear();
	m_num_vars = 0;
	m_clause_count = 0;
}

inline	void		CNF_Theory::set_projected( int x, int p )
{
	m_is_projected[x] = p;
}

inline	int		CNF_Theory::is_projected( int x )
{
	if ( m_is_projected.size() == 0 ) return 0;
	return m_is_projected[x];
}

inline bool		CNF_Theory::var_is_weighted( int x ) 
{
	return !((fabs(weight(x) - 0.0) <= 1e-7) && (fabs(weight(-x) - 0.0 )) <= 1e-7);
}

inline	int		CNF_Theory::weight_factor()
{
	return m_factor;
}

inline	void		CNF_Theory::set_weight_factor( int w )
{
	m_factor = w;
}

inline	void		CNF_Theory::set_top( int top )
{
	m_top_value = top;
}

inline	int		CNF_Theory::top()
{
	return m_top_value;
}

inline	void		CNF_Theory::add_pure_literal( int x )
{
	m_pure_lits.push_back(x);
}

inline std::vector<int>&	CNF_Theory::pure_literals()
{
	return m_pure_lits;
}

}
#endif // cnf_theory.hxx
