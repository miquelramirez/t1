#ifndef __DNNF_THEORY__
#define __DNNF_THEORY__

#include <iosfwd>
#include <vector>
#include <libdnnf/cnf_theory.hxx>
#include <cmath>
#include <libdnnf/ext_math.hxx>

namespace DNNF
{

class DNNF_Theory
{
public:
	typedef enum
	{
		AND = 0,
		OR,
		LITERAL
	} Node_Type;

	struct Node
	{
		Node_Type          type;
		int                conf_var;
		int                children[2];
		int                l;
		int                w;
		int                w_0;
		int                min_child;
	};

	typedef std::vector<Node>  Vertex_List;
	
	DNNF_Theory();
	DNNF_Theory( const DNNF_Theory& t );
//	const DNNF_Theory& operator=( const DNNF_Theory& t );
	~DNNF_Theory();

	int num_vars() const; 
	int node_count() const; 
	int edge_count() const;
	int num_leaves() const;

	bool has_var( int vid );
	
	std::vector<Node>& nodes();

	bool load_from( std::ifstream& is );
	void store_on( std::ostream& os );
        void print_weights( std::ostream& os );

	Schema& vars();	

	bool var_present( int x );	
	
	int       compute_weight();
	int       compute_weight( int* e );
	int       fast_compute_weight( int* e );
	void      compute_bound_conflict( int* e, int UB );
	void      update_w0();
	void      compute_unsat_conflict( int* e );
	int       compute_best_model( int* e, bool recompute_weight = true );
	void      mark_best_leaves( int* e );

	char*	  best_model(int &w );
	char*	  best_model_given_lit( int l, int& w );
	char*	  best_model_given_lits( std::vector<int>& lits, int& w );

	char* get_bound_conflict();
	char* get_best_model();

	char* get_best_leaves(); 
	
	void assign_weights();
	void allocate_weights();
	bool weights_compacted();
	
	std::vector<int>& or_nodes();
	
	Node& at( int idx );
	Node& root( );
	Node& child( Node& n, int ci );
	Node& child( int idx, int ci );
	
	int  lw( int lit );

	void set_lw( int lit, int w );

	int* lws();
	int  lws_sz();

	bool no_models();

	void set_model_count( double mc );	
	double	model_count() const;
	
protected:

	int                  m_var_count;
	int*                 m_cw;
	int                  m_cw_sz;
	Schema	             m_variables;
	Vertex_List          m_dnnf_graph;
	int                  m_edges;
	std::vector<int>     m_or_nodes;
	bool                 m_weights_compacted;
	int                  m_leaves_sz;
	char*                m_best_leaves;
	char*                m_mod_mask;  // pending-to-update flags for DNNF nodes (weight update)
	char*                m_var_present;
	int                  m_var_sz;
	char*                m_best_model;
       	int                  m_inf;
	char*                m_bound_conflict;
	bool                 m_no_models;
	int*		     m_cond_lits;	
	double		     m_model_count;
};

inline 	void	DNNF_Theory::set_model_count( double mc )
{
	m_model_count = mc;
}

inline	double	DNNF_Theory::model_count() const
{
	return m_model_count;
}

inline	int	DNNF_Theory::num_vars() const
{
	return m_var_count;
}

inline int	DNNF_Theory::node_count() const
{
	return m_dnnf_graph.size();
}

inline int	DNNF_Theory::edge_count() const
{
	return m_edges;
}

inline int	DNNF_Theory::num_leaves() const
{
	return m_leaves_sz;
}

inline	bool	DNNF_Theory::has_var( int vid )
{
	return m_variables.find( vid ) != m_variables.end();
}

inline	std::vector<DNNF_Theory::Node>&	DNNF_Theory::nodes()
{
	return m_dnnf_graph;
}

inline	Schema&	DNNF_Theory::vars()
{
	return m_variables;
}

inline	bool	DNNF_Theory::var_present( int x )
{
	return m_var_present[x] == 1;
}

inline	char*	DNNF_Theory::get_bound_conflict()
{
	return m_bound_conflict;
}

inline	char*	DNNF_Theory::get_best_model()
{
	return m_best_model;
}

inline	char*	DNNF_Theory::get_best_leaves()
{
	return m_best_leaves;
}

inline	bool	DNNF_Theory::weights_compacted()
{
	return m_weights_compacted;
}

inline	std::vector<int>&	DNNF_Theory::or_nodes()
{
	return m_or_nodes;	
}

inline	DNNF_Theory::Node&	DNNF_Theory::at(int idx ) 
{
	return m_dnnf_graph[idx];
}

inline	DNNF_Theory::Node&	DNNF_Theory::root()
{
	return m_dnnf_graph.back();	
}

inline	DNNF_Theory::Node&	DNNF_Theory::child( DNNF_Theory::Node& n, int ci )
{
	return at(n.children[ci]);
}

inline	DNNF_Theory::Node&	DNNF_Theory::child( int idx, int ci )
{
	return m_dnnf_graph[ m_dnnf_graph[idx].children[ci] ];	
}

inline	int	DNNF_Theory::lw( int lit ) 
{
	return ( lit > 0 ? m_cw[ abs(lit)*2 ] : m_cw[ abs(lit)*2 + 1 ] );
}

inline	void	DNNF_Theory::set_lw( int lit, int w )
{
	m_cw[ ( lit > 0 ? abs(lit)*2 : abs(lit)*2 + 1) ] = w;
}

inline	int*	DNNF_Theory::lws()
{
	return m_cw;
}

inline	int	DNNF_Theory::lws_sz()
{
	return m_cw_sz;
}

inline	bool	DNNF_Theory::no_models()
{
	return m_no_models;	
}

}
#endif // dnnf_theory.hxx
