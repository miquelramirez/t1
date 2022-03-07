#include <iostream>
#include <fstream>
#include <libdnnf/cnf_theory.hxx>
#include <libdnnf/cnf2dnnf.hxx>
#include <libdnnf/dnnf_theory.hxx>
#include "nff_dijkstra.hxx"



// MRJ: Matrix for holding the states we sample from
//	the initial belief
std::vector< std::vector< int > > models;
int ALL_MODS;

void make_initial_belief( CNF_Theory& initial )
{
	Clause	c;
	for ( int i = 1; i <= 9; i++ )
		c.insert( i ); // inserting literal xi in c1

	initial.add_clause( c );
	c.clear();
	for ( int i = 1; i <= 9; i++ )
		for ( int j = i+1; j <= 9; j++ )
		{
			c.insert( -i );
			c.insert( -j );
			initial.add_clause( c );
			c.clear();
		}

	// AA: Simple example: NO BALLS

	/*	for ( int i = 10; i <= 12; i++ )
		c.insert( i );
	initial.add_clause( c );
	c.clear();
	for ( int i = 10; i <= 12; i++ )
		for ( int j = i+1; j <= 12; j++ )
		{
			c.insert( -i );
			c.insert( -j );
			initial.add_clause( c );
			c.clear();
		}
	for ( int i = 13; i <= 15; i++ )
		c.insert(i);
	initial.add_clause(c);
	c.clear();
	for ( int i = 13; i <= 15; i++ )
		for ( int j = i+1; j <= 15; j++ )
		{
			c.insert( -i );
			c.insert( -j );
			initial.add_clause( c );
			c.clear();
		}
	*/
}


// MRJ: Do not pass vectors by value: they're not pointers! Actually, do not pass objects
// in general by value! :)
NFF::Dijkstra::Rule make_state_rule ( std::vector<NFF::Dijkstra::BeliefLiteral>& head, NFF::Dijkstra::BeliefLiteral& p, std::string n )
{
  NFF::Dijkstra::Rule r;
  r.head = head;
  r.effect = p;
  r.set_cost( 1 );
  // AA: DEBUG PURPOSE
  //  strcpy( r.name, n );
  r.name = std::string(n);  
 return r;
}

// MRJ: My suggestion for making merge rules
NFF::Dijkstra::Rule make_merge_rule( std::vector<NFF::Dijkstra::BeliefLiteral>& head, int index, int universal_index )
{
	NFF::Dijkstra::Rule r;
	r.head = head;
	r.effect = NFF::Dijkstra::BeliefLiteral( index, universal_index);
	r.set_cost( 0 );
	// MRJ: You shouldn't be using raw char*, prefer std::string
	//AA: DEBUG PURPOSE
	// strcpy( r.name, "MERGE\0" );
	r.name = std::string("MERGE"); 
	return r;
}

NFF::Dijkstra::Rule make_merge_rule ( std::vector<NFF::Dijkstra::BeliefLiteral>& head, NFF::Dijkstra::BeliefLiteral& p )
{
  NFF::Dijkstra::Rule r;
  r.head = head;
  r.effect = p;
  r.set_cost( 0 );
  //AA: DEBUG PURPOSE
  r.name = std::string("MERGE"); 
  //  strcpy( r.name, "MERGE\0" ); 
  return r;
}

// MRJ: A more effective way of doing this by using one of
// STL generic algorithms, like this. A const reference on
// a primitive type is very much like passing it by value 
// (the integer will be a double word, very much like the reference
// which is basically a pointer with some restrictions)
//bool instate ( const int& value, std::vector<int>& myvector )
bool instate( int value, std::vector<int>& myvector )
{
	//std::vector<int>::iterator first = myvector.begin();
	//std::vector<int>::iterator last = myvector.end();

	//for ( ;first!=last; first++)
	//   	if ( *first==value ) return true;
	//return false;

	// MRJ: The std::find() function implements a simple linear search over
	// the elements in the container, returning the end() iterator if the
	// value searched for isn't amongst the elements, otherwise it returns
	// the iterator pointing to the position where it was found
	return std::find( myvector.begin(), myvector.end(), value) != myvector.end();
}


// MRJ: When passing objects as arguments always prefer to use references rather than pointers
//      It's a matter of clarity.
// AA: OK, done! :-)
void print_all_rules ( std::vector<NFF::Dijkstra::Rule>& rules_vec )
{

  for ( unsigned i = 0; i < rules_vec.size(); i++ )
    {
      NFF::Dijkstra::Rule &r = rules_vec.at(i);
      std::cout << "Rule: " << i << "  " << r.name << std::endl;
      std::cout << "     Conditions: " ;
      for ( unsigned j = 0; j < r.head.size(); j++)
	std::cout << "<" << r.head[j].first << ", " << r.head[j].second << "> " ;
      std::cout << " Effect: " ;
	std::cout << "<" << r.effect.first << ", " << r.effect.second << "> " << std::endl;
    }

}


void make_rules( NFF::Dijkstra& h )
{
  /*
    Schema of the maze:
 
     1 | 2 | 3
     4 | 5 | 6
     7 | 8 | 9
  */

  
  // make the pairs, lit, ... and the initial_situation
  // AA: cuidado: fluents will not be defined nor the goal
  std::vector<NFF::Dijkstra::BeliefLiteral> head;
  NFF::Dijkstra::BeliefLiteral p;

  // AA: I don't understand why I've to declare ruli as a reference;
  // the compiler should do it for me...
  std::vector<NFF::Dijkstra::Rule> &ruli = h.states_rules();

  head.clear();


  // going left
  for ( int i = 2; i <= 9; i++ )
    {
      if (  i == 4 || i == 7)
	continue;
      head.push_back(  std::pair<int,int> ( i, ALL_MODS ) );
      p = std::pair<int,int> ( (i-1), ALL_MODS );
     
      ruli.push_back( make_state_rule( head, p, std::string("LEFT") ) );
      head.clear();
    }
  // going left for all models
  for ( int i = 2; i <= 9; i++ )
    {
      if (  i == 4 || i == 7)
	continue;
      for ( unsigned j = 0; j < models.size(); j++ )
	{
	  // only rules where condition is in the model
	  //	  if ( instate( i ,  models[j] ))
	    head.push_back(  std::pair<int,int> ( i, j ) );
	    // else	    continue;

	  p = std::pair<int,int> ( (i-1), j );
	  ruli.push_back( make_state_rule( head, p, std::string("LEFT")) );
	  head.clear();
	}
    }

 // going right 
  for ( int i = 1; i <= 8; i++ )
    {
    if ( i == 3 || i == 6)
	continue;
      head.push_back(  std::pair<int,int> ( i, ALL_MODS ) );
      p = std::pair<int,int> ( (i+1), ALL_MODS );
      ruli.push_back( make_state_rule( head, p, std::string("RIGHT") ));
      head.clear();
    }
  // going right for all models
  for ( int i = 1; i <= 8; i++ )
    {
      if ( i == 3 || i == 6)
	continue;

      for ( unsigned j = 0; j < models.size(); j++ )
	{
	  // only rules where condition is in the model
	  //  if ( instate( i ,  models[j] ))
	    head.push_back(  std::pair<int,int> ( i, j ) );
	    // else	    continue;

	  p = std::pair<int,int> ( (i+1), j );
	  ruli.push_back( make_state_rule( head, p,
					   std::string("RIGHT") ) );
	  head.clear();
	}
    }


 // going up 
  for ( int i = 4; i <= 9; i++ )
    {
      head.push_back(  std::pair<int,int> ( i, ALL_MODS ) );
      p = std::pair<int,int> ( (i-3), ALL_MODS );
      ruli.push_back( make_state_rule( head, p, std::string("UP") ) );
      head.clear();
    }
  // going up for all models
  for ( int i = 4; i <= 9; i++ )
    {
      for ( unsigned j = 0; j < models.size(); j++ )
	{
	  // only rules where condition is in the model
	  //  if ( instate( i ,  models[j] ))
	    head.push_back(  std::pair<int,int> ( i, j ) );
	    //	  else     continue;

	  p = std::pair<int,int> ( (i-3), j );
	  ruli.push_back( make_state_rule( head, p, std::string("UP")) );
	  head.clear();
	}
    }

 // going down
  for ( int i = 1; i <= 6; i++ )
    {
      head.push_back(  std::pair<int,int> ( i, ALL_MODS ) );
      p = std::pair<int,int> ( (i+3), ALL_MODS );
      ruli.push_back( make_state_rule( head, p, std::string("DOWN")) );
      head.clear();
    }
  // going down for all models
  for ( int i = 1; i <= 6; i++ )
    {
      for ( unsigned j = 0; j < models.size(); j++ )
	{
	  // only rules where condition is in the model
	  //	  if ( instate( i ,  models[j] ))
	    head.push_back(  std::pair<int,int> ( i, j ) );
	    //	  else	    continue;

	  p = std::pair<int,int> ( (i+3), j );
	  ruli.push_back( make_state_rule( head, p, std::string("DOWN") ));
	  head.clear();
	}
    }

  // AA: should add negative effects?? -> the same but with (neg p) as effect

  // Pickup 
  // Prec: at Cell & ball-at Cell
  // Effect: NOT ball-at Cell



  /** Merge Rules **/
  for (int i = 1; i <= 9; i++)
    {
     for ( unsigned j = 0; j < models.size(); j++ )
       head.push_back(  NFF::Dijkstra::BeliefLiteral ( i, j ) );
	// MRJ: I would suggest that rather than creating the merge rules like this
     	//h->merge_rules()->push_back( make_merge_rule( head,  NFF::Dijkstra::BeliefLiteral( i ,ALL_MODS )) );
	// you do it with the method I suggest (less temporary objects means less time, and there can be
	// quite a few of these rules)
	h.merge_rules().push_back( make_merge_rule( head, i, ALL_MODS) );
     head.clear();
    }
  /** Static rules **/
  // AA: none for now

  print_all_rules ( h.unify_all_rules() );
  
  return;
}




void compile( CNF_Theory& cnf, DNNF_Theory& dnnf )
{
	CNF_DNNF_Compiler compiler;
	compiler.set_compiler_path( "../../libdnnf/c2d_linux" );
	compiler.compile( cnf, dnnf );
	std::cout << "Time spent compiling CNF into DNNF: " << compiler.total_time() << std::endl;	
}

int main( int argc, char** argv )
{
	CNF_Theory	initial;

	// MRJ: See example 'cnf_usage' for an explanation
	// of what this function is doing
	make_initial_belief( initial );


	DNNF_Theory 	initial_dnnf;

	// MRJ: See example 'cnf2dnnf' for an explanation
	// of what this function is doing
	compile( initial, initial_dnnf );

	// MRJ: The total number of literals x, ~x is exactly
	//      2*N+2 where N is the number of variables. 
	//      Indices 0 and 1 are not legal (variables identifiers
	//	are in the range [1..N] as required by the DIMACS
	//	specification).
	const unsigned num_lits = (2*initial.num_vars()+2);
	// MRJ: Weight for covered lits is set to some arbitrarily
	//      high constant
	const int covered_weight = 10000;
	// MRJ: Weight for uncovered lits is set to some arbitrarily
	//	low constant
	const int uncovered_weight = 1;

	// MRJ: Entries in this array will be true for the unit
	//      literals present in the initial belief CNF theory
	//	(and their negations, since if we know x, we also
	//	know ~x).
	//	Note how we start from i = 2, since 0 and 1 are
	//	not valid
	std::vector<bool> known_lit( num_lits );
	for ( unsigned i = 2; i < known_lit.size(); i++ )
		known_lit[i] = false;

	// MRJ: Entries in this array will be true for those
	//	literals which have been marked as covered.
	//	Known literals are ignored.	
	std::vector<bool> covered_lit( num_lits );
	for ( unsigned i = 2; i < covered_lit.size(); i++ )
	{
		if ( known_lit[i] ) continue;
		covered_lit[i] = false;
		initial_dnnf.set_lw( ( i % 2 == 0 ? (i/2) : -(i/2)), uncovered_weight );
	}

	int num_queries = 0;

	// MRJ: Now for each literal	
	for ( unsigned current_lit = 2; current_lit < num_lits; current_lit++ )
	{
		// MRJ: We ignore known literals
		if ( known_lit[current_lit] ) continue;
		// MRJ: We ignored covered literals
		if ( covered_lit[current_lit] ) continue;
		// MRJ: Variable for keeping the cost of the best model,
		//	not necessary but helpful when debugging
		int w;
		// MRJ: CNF and DNNF libraries use signed integers for denote
		//	literals: where -3 means "the negated literal of var 3"
		int l = ( current_lit % 2 == 0 ? 1 : -1 )*(current_lit/2);
		// MRJ: We retrieve the best model for the theory DNNF(I) & l
		char* best_model = initial_dnnf.best_model_given_lit(l, w);
		num_queries++;
		std::cout << "Found model with cost: " << w << std::endl;
		// MRJ: We store the model as a vector signed integers. I am
		// 	not sure this will be the most convenient way of retrieving
		//	models for the planner, but it illustrates how to
		//	decode the model from the representation used by the DNNF	
		std::vector<int> model( initial.num_vars()+1 );
		for ( unsigned k = 1; k < model.size(); k++ )
		{
			// MRJ: in the char* array returned by best_model_given_lit()
			//	indices encode variable names, and the value of best_model(i)
			//	is 1, if the variable xi was assigned true, -1 otherwise
			model[k] = std::sgn(best_model[k])*k;
			// MRJ: We mark the literal as covered
			covered_lit[ ( best_model[k] > 0 ? 2*k : (2*k)+1 ) ] = true;
			// MRJ: We set the weight for the literal, so it is taken into
			//	account in the next query we make to the DNNF
			initial_dnnf.set_lw( model[k], covered_weight );
		}
		// MRJ: We store the model
		models.push_back( model );	
		
	} 
	
	// MRJ: Finally we print the approximation so we can take a look at it
	std::cout << "# Queries performed: " << num_queries << std::endl;
	std::cout << "States in b' ( " << models.size() <<  " )  " << std::endl;	
	for ( unsigned k = 0; k < models.size(); k++ )
	{
		std::cout << "s_" << k+1 << ": ";
		for ( unsigned l = 1; l < models[k].size(); l++ ) 
			std::cout << models[k][l] << " ";
		std::cout << std::endl;
	}

	NFF::Dijkstra 	heuristic; //( (int)models.size() );


	// Initializes the heuristic
	//	heuristic.set_states_rules(std::vector<NFF::Dijkstra::Rule> ());

	ALL_MODS = (int)models.size() ;


	// AA: makes the set of rules
	make_rules( heuristic );

	// AA Makes the initial situation
	// Init is empty: we don't know anything 
	// (just, maybe that balls are not in the middle row)
	// Goal: to be in cell 1
	std::vector<int> fl;
	for ( unsigned i = 1; i <= 9; i++ )
	  fl.push_back(i);
	heuristic.TEST_compute(  models, fl, NFF::Dijkstra::BeliefLiteral ( 1, ALL_MODS), std::vector<unsigned> (0));

	int hg = heuristic.h_value( NFF::Dijkstra::BeliefLiteral ( 1, ALL_MODS)  );;

	std::cout << " Distance from GOAL: " << hg << std::endl;

	exit(0);
	return 0;
}
