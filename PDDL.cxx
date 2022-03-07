/*
    Miguel Ramirez, Nir Lipovetzky, Hector Geffner
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
#include <iostream>
#include <cassert>
#include <string>
#include "utils.hxx"
#include "global_options.hxx"
#include "libff.h"
#include "nff.hxx"
#include "nff_belief.hxx"
#include "nff_mutexes.hxx"
#include <fstream>
#include <algorithm>
#include <limits>
#include "nff_options.hxx"

#define DEBUG_INVARIANT false

namespace PDDL
{

Fluent::Fluent( int code )
	: m_code( code )
{
  m_sign = true;
}

  Fluent::Fluent( int code, bool sign )
    : m_code( code ), m_sign( sign )
{
}

Fluent::~Fluent()
{
}

Operator::Operator()
  :  m_metric(0.0f)
{
	PDDL::Task& task = PDDL::Task::instance();
	m_precondition = new Fluent_Set( task.fluent_count() );
	m_adds = new Fluent_Set( task.fluent_count() );
	m_dels = new Fluent_Set( task.fluent_count() );
	m_hard_precs = NULL;
	if( gconditional_effects )
                m_hard_precs = new Fluent_Set( task.fluent_count() );
}

Operator::Operator( int code )
	: m_code( code ), m_metric(0.0f)
{
	PDDL::Task& task = PDDL::Task::instance();
	m_precondition = new Fluent_Set( task.fluent_count() );
	m_adds = new Fluent_Set( task.fluent_count() );
	m_dels = new Fluent_Set( task.fluent_count() );
	if( gconditional_effects )
                m_hard_precs = new Fluent_Set( task.fluent_count() );
}

//Recursively propagate precedences in layers
void  Operator::propagate_precedence(std::map<unsigned, NFF::Atom_Vec>& precedence, unsigned prec  )
{
  NFF::Atom_Vec& p = precedence[prec];
  
  for(unsigned i = 0; i < p.size(); i++)
  {
          m_prec_layer[ p[i] ] = m_prec_layer[ prec ] + 1;
          propagate_precedence(precedence, p[i] );
  }
}

unsigned
Operator::get_pc_index(unsigned fluent)
{
 for(unsigned i = 0; i < prec_vec().size(); i++) {
   if(prec_vec()[i] == fluent)
     return i;
 }
 return prec_vec().size();
}


Operator::~Operator()
{
	delete m_precondition;
	delete m_adds;
	delete m_dels;
	if( m_hard_precs != NULL ) delete m_hard_precs;
}

Task::Task()
        :  m_string_table( String_Table::instance() ), m_glog( "grounding.stats" ), m_negation(false)

{
}

Task::~Task()
{
//        if ( Global_Options::instance().precs_and_goals_mode() )
        delete m_prec_goal_fluents_set;
}

Task& Task::instance()
{
	static Task the_instance;
	return the_instance;
}

void Task::setup()
{
	Global_Options& opt = Global_Options::instance();

  
	float t0 = time_used();
	FF_parse_problem( opt.domain_filename().c_str(), opt.instance_filename().c_str() );
	float tf = time_used(); 
	m_glog << "Parsing="; report_interval( t0, tf, m_glog );
	
	t0 = time_used();
	FF_instantiate_problem();	
	tf = time_used();
	m_glog << "Instantiation="; report_interval( t0, tf, m_glog );

	t0 = time_used();
	create_fluents();
	tf = time_used();
	m_glog << "Fluent_Copying="; report_interval( t0, tf, m_glog );
//	if ( opt.precs_and_goals_mode() )
        m_prec_goal_fluents_set = new Fluent_Set( fluent_count()+1 );

	t0 = time_used();
	create_init_and_goal();
	tf = time_used();
	m_glog << "Init_And_Goal_Copying="; report_interval( t0, tf, m_glog );

	t0 = time_used();
	create_operators();
	tf = time_used();
	m_glog << "Operators_Copying="; report_interval( t0, tf, m_glog );

	// Setup costs
	m_op_costs.resize( useful_ops().size() );
	for ( unsigned o = 0; o < useful_ops().size(); o++ )
	    m_op_costs[o] = 1;
	
	//!!!!!IGNORING COST OF PDDL
	//  m_op_costs[o] = useful_ops()[o]->metric_cost();
	set_domain_name( FF::get_domain_name() );
	set_problem_name( FF::get_problem_name() );
}
  

        void Task::set_initial_models(std::vector<std::vector<int> >* m)
        {
                assert(m_initial_models == NULL);
                
                m_initial_models = m;
                
//                        std::cerr << "Trying to initialize twice the samples." << std::endl; 
        }


void Task::create_fluents()
{
	fluents().push_back( NULL );
	not_equiv_table().push_back( NULL );
	ff_equiv_table().resize( gnum_ft_conn );

	for ( int i = 0; i < gnum_ft_conn; i++ )
	{
	  std::string ft_name = FF::get_ft_name(i);
	  bool ft_sign = FF::get_ft_sign(i);
	  int str_code = str_tab().get_code( ft_name );
	  int neg_ft = FF::get_ft_negated(i) ;
	  
	  /* If ft is neg and has an equivalent
	   * it has already been created */
	  if ( !ft_sign && ( neg_ft != -1))
	    continue;

	  fluents().push_back( new Fluent( str_code, ft_sign ) );	
	  ff_equiv_table()[i] = (fluents().size()-1);

	  /* creates the negative fluent */
	  if ( ft_sign && ( neg_ft != -1))
	    { 			
	      std::string nft_name = FF::get_ft_name( neg_ft );
	      bool nft_sign = FF::get_ft_sign( neg_ft );

	      fluents().push_back( new Fluent(  str_tab().get_code( nft_name ), nft_sign ) );	
	      ff_equiv_table()[ neg_ft ] = (fluents().size() - 1);

	      not_equiv_table().push_back( fluents().size()-1 );
	      not_equiv_table().push_back(  fluents().size()-2 );
		
#ifdef DEBUG
		print_fluent( not_equiv_table().size()-1);
		std::cout << " <-> ";
		print_fluent( not_equiv_table().size()-2 );
		std::cout << std::endl; 
#endif
	    } 
	  else	    
	    not_equiv_table().push_back( NULL );
	}
	
	
	if(m_negation)
	  for ( int i = 0; i < gnum_ft_conn; i++ )
	    {	
	      /*****************************************/
	      /*ONLY FOR BLOCKS, TEST for other domains*/
	      /*****************************************/
	      
	      std::string ft_not_name = "NOT-"+FF::get_ft_name(i);	 
	      bool ft_sign = FF::get_ft_sign(i);
	      int str_code = str_tab().get_code( ft_not_name );
	      fluents().push_back( new Fluent( str_code, ft_sign ) );
	      
	      
	      /*****************************************/
	    }
	/*****************Generalized Fluents*******************************/
	
	for ( int i = 0; i < gnum_ft_conn; i++ )
	  {
	    std::vector<std::string> abs_ft_name = FF::get_abstract_ft_name(i);
	    for( unsigned j = 0; j < abs_ft_name.size(); j++ )
	      {
		m_predicateName_facts[ abs_ft_name[j] ].push_back(unsigned (i+1));
	      }
	  }
	
	m_fluents_generalized.resize( fluents().size() );
	
	//comment to disble DISJ LAND
	/*
	for(std::map<std::string, std::vector<unsigned> >::iterator it =  m_predicateName_facts.begin();
	    it != m_predicateName_facts.end(); it++)
	  {
	    std::string ft_name = it->first; 
	    int str_code = str_tab().get_code( ft_name );
	    fluents().push_back( new Fluent( str_code ) );
#ifndef ENDEBUG
	    std::cout<<std::endl;
	    std::cout<<it->first<<" <-- ";
#endif
	    for(unsigned j = 0; j < it->second.size(); j++)
	      {
		m_fluents_generalized[ it->second[j] ].push_back( fluents().size() -1 );
#ifndef ENDEBUG
		print_fluent(it->second[j]);
		std::cout<<" "<<std::flush;
#endif
	      }
	  }
	*/
	/*****************************************************************/

	int str_code = str_tab().get_code( "GOAL" );
	fluents().push_back( new Fluent( str_code ) );				
	not_equiv_table().push_back( NULL );

	m_dummy_goal_id = fluents().size() - 1;

	m_required_by.resize( fluents().size() );
	m_added_by.resize( fluents().size() );
	m_deleted_by.resize( fluents().size() );
	m_e_deleted_by.resize( fluents().size() );
}



void Task::create_init_and_goal()
{
  std::vector<unsigned>	tmp_initial_state;
  std::vector<unsigned>     tmp_initial_unknown_atoms;
  std::vector<std::vector<unsigned> > tmp_initial_clauses;
  FF::get_initial_state( tmp_initial_state );
  FF::get_initial_unknown( tmp_initial_unknown_atoms );
  FF::get_initial_ors ( tmp_initial_clauses );
  //  Global_Options& opt = Global_Options::instance();

  m_initial_clauses.resize( tmp_initial_clauses.size() );

  for(int i = 0; i < gnum_ft_conn; i++)
    {
      std::vector<unsigned>::iterator result = std::find(tmp_initial_state.begin(), tmp_initial_state.end(), (unsigned)i);
      std::vector<unsigned>::iterator result2 = std::find(tmp_initial_unknown_atoms.begin(), tmp_initial_unknown_atoms.end(), (unsigned)i);	    

      // Creates the list of initially known atoms
      if(m_negation)
	{
	  if(result == tmp_initial_state.end())
	    m_initial_state.push_back(ff_equivalent(i)+gnum_ft_conn);
	  else
	    m_initial_state.push_back( ff_equivalent(i) );
	}
      else
	if(result != tmp_initial_state.end())
	  {		
	    m_initial_state.push_back( ff_equivalent(i) );
	    for(unsigned j = 0; j < m_fluents_generalized[ff_equivalent(i)].size(); j++)
	      if(std::find( m_initial_state.begin(),
			    m_initial_state.end(), 
			    m_fluents_generalized[ff_equivalent(i)][j])
		 == m_initial_state.end() )
		m_initial_state.push_back( (m_fluents_generalized[ff_equivalent(i)][j]));
	  }

      // Creates the list of initially unknown atoms
      if(m_negation)
	{
	  if(result2 == tmp_initial_unknown_atoms.end())
	    m_initial_unknown.push_back( ff_equivalent(i)+gnum_ft_conn );
	  else
	    m_initial_unknown.push_back( ff_equivalent(i) );
	}
      else
	if(result2 != tmp_initial_unknown_atoms.end())
	  {		
	    m_initial_unknown.push_back( ff_equivalent(i) );
	    for(unsigned j = 0; j < m_fluents_generalized[ff_equivalent(i)].size(); j++)
	      if(std::find(m_initial_unknown.begin(),
			   m_initial_unknown.end(),
			   m_fluents_generalized[ff_equivalent(i)][j])
		 == m_initial_unknown.end() )
		m_initial_unknown.push_back( m_fluents_generalized[ff_equivalent(i)][j]);
	  }
	    
      // Creates the list of initial clauses
      for(unsigned j = 0; j < tmp_initial_clauses.size(); j++)
	{
	  std::vector<unsigned>::iterator result3 = std::find(tmp_initial_clauses[j].begin(),
							      tmp_initial_clauses[j].end(), 
							      (unsigned)i);		

	  //	  std::cout << "     " << FF::get_ft_name( i ) << std::endl;
	  // once a lit has been found in a or in FF, 
	  // it is added in the respective clause in C3
	  if(m_negation)
	    {
	      if(result3 == tmp_initial_clauses[j].end())
		m_initial_clauses[j].push_back( ff_equivalent(i)+gnum_ft_conn );
	      else
		m_initial_clauses[j].push_back( ff_equivalent(i) );
	    }
	  else
	    if(result3 != tmp_initial_clauses[j].end())
	      {		
		m_initial_clauses[j].push_back( ff_equivalent(i) );

		for(unsigned k = 0; k < m_fluents_generalized[ff_equivalent(i)].size(); k++)
		  if(std::find(m_initial_clauses[j].begin(),
			       m_initial_clauses[j].end(),
			       m_fluents_generalized[ff_equivalent(i)][k])
		     == m_initial_clauses[j].end() )
		    m_initial_clauses[j].push_back( m_fluents_generalized[ff_equivalent(i)][k]);
	      }
		
	}
    }

  std::vector<unsigned> oneofs_corr;
  FF::get_initial_oneofs( oneofs_corr );

  // translates the unknown clauses into oneofs
  for (unsigned u = 0; u < initial_unknown().size(); u++)
  {
          NFF::Atom_Vec tmp;
          unsigned not_u = not_equivalent( initial_unknown()[u] );
          if ( !not_u )   continue;
          tmp.push_back( initial_unknown()[u] );
          tmp.push_back( not_u );
          m_initial_oneofs.push_back( tmp );
          m_initial_clauses.push_back( tmp );
  }

  // now that we have the ors, we make the oneofs
  for (unsigned of = 0; of < (unsigned)gnum_full_oneof_initial; of++ ) 
  {
          unsigned or_corr = goneof_or_corr[ of ];
          m_initial_oneofs.push_back( std::vector<unsigned>( initial_clauses()[ or_corr ] ) );
  }
#ifdef DEBUG
  // Printing
  if ( oneofs().size() == 0)
          std::cout << "There are no oneofs in this problem." << std::endl;
  else 
          {
                  std::cout << std::endl << "Initial ONEOFS:" << std::endl;
                  for (unsigned i = 0; i < oneofs().size(); i++)
                          {
                                  for (unsigned j = 0; j < m_initial_oneofs[i].size(); j++)
                                          print_fluent( m_initial_oneofs[i][j], std::cout);

                                  std::cout << std::endl;
                          }
          }
  std::cout << std::endl;
#endif
  
  // Sets the initial belief state
  m_initial_belief = NFF::Belief::make_initial_belief();
  
  std::string start_name( "(START)" );
  m_start_id = 0;
  useful_ops().push_back( new Operator( str_tab().get_code( start_name ) ) );
  useful_ops()[m_start_id]->set_idx(m_start_id);
  for ( unsigned i = m_start_id; i < m_initial_state.size(); i++ )
  {
          useful_ops()[m_start_id]->adds().set( m_initial_state[i] );
          useful_ops()[m_start_id]->add_vec().push_back( m_initial_state[i] );
  }
  
  FF::get_goal_state( m_goal_state );
  unsigned gsize = m_goal_state.size();

  for ( unsigned i = 0; i < gsize ; i++ )
    {
            m_goal_state[i] = ff_equiv_table()[m_goal_state[i]];
    }
	

	
  std::string end_name( "(END)" );
  m_end_id = 1;
  useful_ops().push_back( new Operator( str_tab().get_code( end_name ) ) );
  useful_ops()[ m_end_id ]->set_idx( m_end_id );
  for ( unsigned i = 0; i < m_goal_state.size(); i++ )
    {
            useful_ops()[ m_end_id ]->preconds().set( m_goal_state[i] );
            useful_ops()[ m_end_id ]->prec_vec().push_back( m_goal_state[i] );
            
            required_by(m_goal_state[i]).push_back( m_end_id );
//            if ( opt.precs_and_goals_mode() )
            precs_goals_set().set( m_goal_state[i] );
    }
	
  useful_ops()[ m_end_id ]->adds().set( fluents().size()-1 );
  useful_ops()[ m_end_id ]->add_vec().push_back( fluents().size()-1 );


  useful_ops().push_back( new Operator( str_tab().get_code( "NO-OP" ) ) );
  m_no_op_id = 2;
  useful_ops()[ m_no_op_id ]->set_idx( m_no_op_id );
  Operator* curr_op = useful_ops().back();
  curr_op->set_metric_cost( 0 );

	
  /*
    print_initial_state(std::cout);
    std::cout<<std::endl<<std::endl;
    print_goal_state(std::cout);*/

}


void Task::create_invariants()
{
	pos_invariants_table().resize(  useful_ops().size() );
	neg_invariants_table().resize(  useful_ops().size() );

	for ( unsigned p = 1; p < fluents().size()-1; p++ )
	{
		// Skips negative fluents
		if ( !(fluents()[ p ])->is_pos() )
			continue;

		for ( unsigned i = 0; i < useful_ops().size(); i++ )
		{
			std::vector<PDDL::Operator*> ops =  useful_ops()[i]->same_op_vec();
			bool is_positive_invariant = true;
			for ( std::vector<Operator*>::iterator oj = ops.begin(); oj != ops.end(); oj++ )
			{
				if ( (*oj)->dels().isset(p) || (*oj)->adds().isset(p) )
				{
					is_positive_invariant = false;
					break;
				}
			}
			if ( is_positive_invariant ) 
			{
				pos_invariants_table()[i].push_back( p );
				continue;
			}
			/*
			bool is_negative_invariant = true;
			for ( std::vector<Operator*>::iterator oj = ops.begin(); oj != ops.end(); oj++ )
			{
				if ( (*oj)->adds().isset(p) )
				{
					is_negative_invariant = false;
					break;
				}
			}
			if ( is_negative_invariant )
				neg_invariants_table()[i].push_back( p );
			*/
		}

	}



  // DEBUG
  if (DEBUG_INVARIANT)
    {
      std::cout << "\n\t Printing positive invariants: " << std::endl;
      for ( unsigned i = 0; i < pos_invariants_table().size(); i++ )
	{
	  std::cout << " For operator: " << str_tab().get_token(useful_ops()[i]->code() ) << std::endl;

	  for ( unsigned j = 0; j < pos_invariant_lits(i).size(); j++ )
	    print_fluent( fluents()[pos_invariant_lits(i)[j]], std::cout );
	  std::cout << std::endl;
	}

      std::cout << " Printing negative invariants: " << std::endl;
      for ( unsigned i = 0; i < neg_invariants_table().size(); i++ )
	{
	  std::cout << " For operator: " << str_tab().get_token(useful_ops()[i]->code() ) << std::endl;

	  for ( unsigned j = 0; j < neg_invariant_lits(i).size(); j++ )
	    print_fluent( fluents()[neg_invariant_lits(i)[j]], std::cout );
	  std::cout << std::endl;
	}
    }
}
      

  
void Task::create_operators()
{
        Global_Options & opt = Global_Options::instance();

        // AA: in conformant C3 we treat only conditional effects.
        assert(gconditional_effects);
        //if there are conditional effects, change the model execution
        m_cond_effects = gconditional_effects;

        std::map< int,std::vector<Operator*> >* same_ops=NULL;
        
        if(m_cond_effects)
                same_ops = new std::map< int,std::vector<Operator*> >;
        
        for ( int i = 0; i < gnum_ef_conn; i++ )
        {
                if ( gef_conn[i].removed == TRUE ) continue;

                std::string op_name = FF::get_op_name(i);
                useful_ops().push_back( new Operator( str_tab().get_code( op_name ) ) );
                Operator* curr_op = useful_ops().back();
                
                curr_op->set_idx(useful_ops().size()-1);
		
                if(m_cond_effects)
                {
                        (*same_ops)[curr_op->code()].push_back(curr_op);
                        /**set which are the preconditions. Cond and prec will
                           be all in prec_vec**/


                        int* hard_prec = gop_conn[gef_conn[i].op].action->preconds;
                        int nhard_prec = gop_conn[gef_conn[i].op].action->num_preconds;
                        for( int j = 0; j < nhard_prec; j++)
                        {
//                                if ( opt.precs_and_goals_mode() )
                                precs_goals_set().set( ff_equivalent(hard_prec[j]) );
                                curr_op->hard_preconds().set( ff_equivalent(hard_prec[j]) );
                                curr_op->hard_prec_vec().push_back( ff_equivalent(hard_prec[j]) );
                        }		    
                        
                }
		

                for ( int j = 0; j < gef_conn[i].num_PC; j++ )
	        {
                        unsigned p = ff_equivalent(gef_conn[i].PC[j] );
                        m_required_by[p].push_back( useful_ops().size()-1 );
                        curr_op->preconds().set( p );
                        curr_op->prec_vec().push_back( p );
                        /*
                          for(unsigned j = 0; j < m_fluents_generalized[ p ].size(); j++)
                          if( !curr_op->preconds().isset(m_fluents_generalized[ p ][ j ]) )
                          {
                          unsigned p_abs = m_fluents_generalized[ p ][ j ];
                          m_required_by[p_abs].push_back( useful_ops().size()-1 );
                          curr_op->preconds().set( p_abs );
                          curr_op->prec_vec().push_back( p_abs );
                          }	*/
                }	
		
                for ( int j = 0; j < gef_conn[i].num_A; j++ )
	        {
                        unsigned p = ff_equivalent( gef_conn[i].A[j] );
                        m_added_by[p].push_back( useful_ops().size()-1 );
                        curr_op->adds().set( p );
                        curr_op->add_vec().push_back( p );
                        
                        
                        for(unsigned j = 0; j < m_fluents_generalized[ p ].size(); j++)
                                if( !curr_op->adds().isset(m_fluents_generalized[ p ][ j ]) )
                                {
                                        unsigned p_abs = m_fluents_generalized[ p ][ j ];
                                        m_added_by[p_abs].push_back( useful_ops().size()-1 );
                                        curr_op->adds().set( p_abs );
                                        curr_op->add_vec().push_back( p_abs );
                                }
		  
                        /************************************/
                        if(m_negation)
                        {
                                p += gnum_ft_conn;
                                
                                m_deleted_by[p].push_back( useful_ops().size()-1 );
                                curr_op->dels().set( p );
                                curr_op->del_vec().push_back( p );
                        }
                        /************************************/
		  
                }
		
                for ( int j = 0; j < gef_conn[i].num_D; j++ )
	        {
                        unsigned p = ff_equivalent( gef_conn[i].D[j] );
                        m_deleted_by[p].push_back( useful_ops().size()-1 );
                        curr_op->dels().set( p );
                        curr_op->del_vec().push_back( p );
		    
		    
                        /************************************/
                        if(m_negation)   {
                                p += gnum_ft_conn;
                                
                                m_added_by[p].push_back( useful_ops().size()-1 );
                                curr_op->adds().set( p );
                                curr_op->add_vec().push_back( p );
                        }
                        /************************************/
                }
		
                //Ignore Costs
                curr_op->set_metric_cost( 1 );
		
                /*	//COSTS
                        if ( gef_conn[i].num_IN == 0 )
                        curr_op->set_metric_cost( 1 );
                        else if ( gef_conn[i].num_IN >= 1 )
                        curr_op->set_metric_cost( gef_conn[i].cost );//IN_c[0] ); */
        }
        
        if(m_cond_effects)
        {
                std::vector<Operator*> no_prot_list;
                
                std::vector<Operator*> empty;
                useful_ops()[0]->set_same_op_vec(empty);
                useful_ops()[1]->set_same_op_vec(empty);
                useful_ops()[2]->set_same_op_vec(empty);
                
                for(unsigned i = 3; i != useful_ops().size(); i++)
                {
                        useful_ops()[i]->set_same_op_vec( (*same_ops)[ useful_ops()[i]->code() ] );
                        std::vector<Operator*> sov = useful_ops()[i]->same_op_vec();
                        
                        if ( std::find(no_prot_list.begin(),no_prot_list.end(), useful_ops()[i])
                             == no_prot_list.end())
                        {
                                useful_ops()[i]->set_prototype( m_prototype_ops.size() );
                                for ( unsigned k = 0; k < useful_ops()[i]->same_op_vec().size(); k++ )
                                        useful_ops()[i]->same_op_vec()[k]->set_prototype( m_prototype_ops.size() );
                                m_prototype_ops.push_back( useful_ops()[i] );
                                no_prot_list.insert(no_prot_list.end(),sov.begin()+1,sov.end() );
                        }	     
                }

                /* // DEBUG purpose
                   std::cout << "We have " << m_prototype_ops.size() << " operators" << std::endl;
                   for(unsigned i = 0; i != m_prototype_ops.size(); i++)
                   print_operator ( m_prototype_ops.at(i), std::cout); */
                
      
                delete same_ops;
        }
       
        useful_ops().push_back( new Operator( str_tab().get_code( "dummy" ) ) );
        Operator* curr_op = useful_ops().back();
        curr_op->set_metric_cost( 1 );
        curr_op->set_idx(useful_ops().size()-1);
        
        m_op_edeletes.resize( useful_ops().size() );
        for ( unsigned p = 0; p < useful_ops().size(); p++ )
                m_fast_op_edeletes.push_back( new Fluent_Set( fluents().size()+1 ) );



        /** Now that we have the operators, we create the invariants table
         * for each one of them. */
        create_invariants();
	
}


void Task::calculate_relevance()
{

        std::vector< Fluent_Set* > relevant_atoms;
        for ( unsigned i = 0; i < (unsigned)fluent_count(); i++ )
                relevant_atoms.push_back( new Fluent_Set( fluent_count()+1 ) );

        std::vector< Fluent_Set* > del_relevant_atoms;
        for ( unsigned i = 0; i < (unsigned)fluent_count(); i++ )
                del_relevant_atoms.push_back( new Fluent_Set( fluent_count()+1 ) );

        std::vector< std::vector<unsigned> > del_relevant_vec;
        del_relevant_vec.resize( fluent_count() );
        m_relevant_atoms.resize( fluent_count() );


        // Check goal
        NFF::Atom_Vec& G = goal_state();
        for ( unsigned i = 0; i < G.size(); i++ )
        {
                relevant_atoms[ G[i] ]->set(G[i]);
                relevant_to( G[i] ).push_back( G[i] );
        }
        
        // Check action preconditions		
        for ( unsigned i = 0; i < prototype_ops().size(); i++ )
        {
                Operator* o = prototype_ops()[i];
                NFF::Atom_Vec&	precs = o->hard_prec_vec();
                for ( unsigned j = 0; j < precs.size(); j++ )
                {

                        unsigned lit =  precs[j];

                        if ( relevant_atoms[ lit ]->isset(lit) )
                                continue;

                        relevant_atoms[ lit ]->set(lit);

                        relevant_to( lit ).push_back( lit );

                }

                for ( unsigned ce = 0; ce < o->same_op_vec().size(); ce++)
                {

                        NFF::Atom_Vec&	adds = (o->same_op_vec()[ce])->add_vec();
                        for ( unsigned k = 0; k < adds.size(); k++ )
                        {
                                unsigned lit = adds[ k ];

                                NFF::Atom_Vec&	cond = (o->same_op_vec()[ce])->prec_vec();
                                for ( unsigned j = 0; j < cond.size(); j++ )
                                {
                                        unsigned lit2 =  cond[j];

                                        if ( relevant_atoms[ lit ]->isset( lit2 ) )
                                                continue;
                                        if ( o->hard_preconds().isset(lit2) )
                                                continue;

                                        relevant_atoms[ lit ]->set(lit2);

                                        relevant_to( lit ).push_back( lit2 );
                                }
                        } // end (adds)

                        NFF::Atom_Vec&	dels = (o->same_op_vec()[ce])->del_vec();
                        for ( unsigned k = 0; k < dels.size(); k++ )
                        {
                                unsigned lit = dels[ k ];

                                NFF::Atom_Vec&	cond = (o->same_op_vec()[ce])->prec_vec();
                                for ( unsigned j = 0; j < cond.size(); j++ )
                                {
                                        unsigned lit2 =  cond[j];

                                        if ( del_relevant_atoms[ lit ]->isset( lit2 ) )
                                                continue;
                                        if ( o->hard_preconds().isset(lit2) )
                                                continue;

                                        del_relevant_atoms[ lit ]->set(lit2);
                                        del_relevant_vec[ lit ].push_back(lit2);
                                }
                        } // end (dels)
                } // end (same op vec)                
        }

#ifdef DEBUG
        for ( unsigned i = 0; i < (unsigned)fluent_count(); i++ )
        {
                if ( is_prec_or_goal( i ) )
                {
                        std::cout << "Atoms relevants to ";
                        print_fluent( i, std::cout);

                        for (unsigned j = 0; j < relevant_to(i).size(); j++)
                        {
                                std::cout << std::endl << "\t";
                                print_fluent(  relevant_to( i )[j], std::cout);
                        }
                        std::cout << std::endl;
                }
        }
        std::cout << std::endl;
#endif

        // Compute "del relevance rule":
        // A -> B if (A ->  not C) and (C -> not B)
        bool has_changed = false;
        do
        {
                has_changed = false;
                for ( unsigned i = 0; i < del_relevant_vec.size(); i++ )
                {
                        for ( unsigned j = 0; j < del_relevant_vec[i].size() ; j++ )
                        {
                                unsigned lit = del_relevant_vec[i][j];
                                // los relevantes de los relevantes
                                for ( unsigned k = 0; k < del_relevant_vec[ lit ].size() ; k++ )
                                {
                                        unsigned rel =  del_relevant_vec[lit][k];
                                        if ( relevant_atoms[ i ]->isset( rel ) )
                                                continue;

                                        relevant_atoms[ i ]->set( rel );
                                        relevant_to( i ).push_back( rel );
                                        has_changed = true;
                                }                                
                        }
                }        
        } while (has_changed);

        // Compute transitivity relevance relation
        // lit -> i & rel -> lit     => rel -> i
        do
        {
                has_changed = false;
                for ( unsigned i = 0; i < m_relevant_atoms.size(); i++ )
                {
                        for ( unsigned j = 0; j < m_relevant_atoms[i].size() ; j++ )
                        {
                                unsigned lit = relevant_to(i).at(j);
                                // los relevantes de los relevantes
                                for ( unsigned k = 0; k < m_relevant_atoms[ lit ].size() ; k++ )
                                {
                                        unsigned rel =  relevant_to( lit ).at(k);
                                        if ( relevant_atoms[ i ]->isset( rel ) )
                                                continue;

                                        relevant_atoms[ i ]->set( rel );
                                        relevant_to( i ).push_back( rel );
                                        has_changed = true;
                                }                                
                        }
                }        
                
        } while (has_changed);

#ifdef DEBUG
        std::cout << " Relevant atoms after checking transitivity." << std::endl;
        for ( unsigned i = 0; i < (unsigned)fluent_count(); i++ )
        {
                if ( is_prec_or_goal( i ) )
                {
                        std::cout << "Atoms relevants to ";
                        print_fluent( i, std::cout);

                        for (unsigned j = 0; j < relevant_to(i).size(); j++)
                        {
                                std::cout << std::endl << "\t";
                                print_fluent(  relevant_to( i )[j], std::cout);
                        }
                        std::cout << std::endl;
                }
        }
        std::cout << std::endl;
#endif

        for ( unsigned i = 0; i < (unsigned)fluent_count(); i++ )
        {
                delete relevant_atoms[i];
                delete del_relevant_atoms[i];
        }
}



std::string Task::get_fluent_name( unsigned f)
{
        return str_tab().get_token( fluents()[f]->code() );
}

void Task::print_fluent( Fluent* fi, std::ostream& os )
{
        os << str_tab().get_token( fi->code() );
}


void Task::print_fluents( std::ostream& os )
{
	for ( unsigned i = 1; i < fluents().size(); i++ )
	{
		os << i << " ";
		Fluent* fi = fluents()[i];
		print_fluent( fi, os );
		os << std::endl;
	}
}

void Task::print_initial_state( std::ostream& os )
{
	// Creating initial and goal file
	std::ofstream out1("init_goal.list");
	out1 << "Initial state is:" << std::endl; 
	out1 << "\nKNOWN ATOMS:\n" << std::endl;
	for ( int i = 0; i < ginitial_state.num_F; i++ ) 
                out1 << " "<< FF::get_ft_name( ginitial_state.F[i] ) << std::endl;
 
	out1 << "\nUNKNOWN ATOMS:\n" << std::endl;
        for ( int i = 0; i < ginitial_state.num_U; i++ ) 
                out1 << " "<< FF::get_ft_name( ginitial_state.U[i] ) << std::endl;

	out1 << "\nOR CLAUSES:\n" << std::endl;
        for ( int i = 0; i < gnum_initial_or; i++ ) {
                out1 << " OR:" << std::endl;;
                for ( int j = 0; j < ginitial_or_length[i]; j++ ) 
                        out1 << "     " << FF::get_ft_name( ginitial_or[i][j] ) << std::endl;
	}
	out1 << "\nGoal state is:\n" << std::endl;
        for ( int i = 0; i < ggoal_state.num_F; i++ ) 
                out1 << " "<< FF::get_ft_name( ggoal_state.F[i] ) << std::endl;
        out1.close();

	os << "Initial state:" << std::endl;
	for ( unsigned i = 0;  i < initial_state().size(); i++ )
	{
                print_fluent( fluents()[initial_state()[i]], os );
		os << std::endl;
	}
	os << "Initial clauses:" << std::endl;
	for ( unsigned i = 0;  i < initial_clauses().size(); i++ )
	  {
                  os << "( ";
                  for ( unsigned j = 0;  j < initial_clauses()[i].size(); j++ )	
                          print_fluent( fluents()[initial_clauses()[i][j]], os );
                  os << " )" << std::endl;
	  }
}

void Task::print_goal_state( std::ostream& os )
{
	os << "Goal state:" << std::endl;
	for ( unsigned i = 0; i < goal_state().size(); i++ )
	{
		print_fluent( fluents()[goal_state()[i]], os );
		os << std::endl;
	}
}

void Task::print_initial_unknown_atoms( std::ostream& os )
{
	os << "Initial Unknown atoms:" << std::endl;
	for ( unsigned i = 0;  i < initial_unknown().size(); i++ )
	{
                print_fluent( fluents()[(unsigned int)initial_unknown()[i]], os );
		os << std::endl;
	}
}

void Task::print_same_operators( Operator* oi, std::ostream& os )
{
  for ( unsigned i = 0; i < oi->same_op_vec().size(); i++ )
          print_operator_full( oi->same_op_vec()[i], os );
}

void Task::print_operator_full( Operator* oi, std::ostream& os )
{
	os << str_tab().get_token( oi->code() );
	os << ":" << std::endl;
	os << "\t" << "Preconditions:" << std::endl;
	for ( unsigned i = 0; i < oi->hard_prec_vec().size(); i++ )
	{
		unsigned pi = oi->hard_prec_vec()[i];
		os << "\t\t";
		print_fluent( fluents()[pi], os );
		os << std::endl;
	}
	os << "\t" << "Conditions:" << std::endl;
	for ( unsigned i = 0; i < oi->prec_vec().size(); i++ )
	{
		unsigned pi = oi->prec_vec()[i];
		os << "\t\t";
		print_fluent( fluents()[pi], os );
		os << std::endl;
	}
	os << "\t" << "Adds:" << std::endl;
	for ( unsigned i = 0; i < oi->add_vec().size(); i++ )
	{
		unsigned pi = oi->add_vec()[i];
		os << "\t\t";
		print_fluent( fluents()[pi], os );
		os << std::endl;
	}

	os << "\t" << "Deletes:" << std::endl;
	for ( unsigned i = 0; i < oi->del_vec().size(); i++ )
	{
		unsigned pi = oi->del_vec()[i];
		os << "\t\t";
		print_fluent( fluents()[pi], os );
		os << std::endl;
	}


	os << "\t" << "Other conditional effects of the same action:" << std::endl;
	for ( unsigned i = 0; i < oi->same_op_vec().size(); i++ )
	{
	  Operator* so =  oi->same_op_vec().at(i);
	  print_operator_full_notab( so , os );
	  os << std::endl;
	}
	
}

void Task::print_operator_full_notab( Operator* oi, std::ostream& os )
{
	os << str_tab().get_token( oi->code() );
	os << std::endl;
	os << "Preconditions:" << std::endl;
	for ( unsigned i = 0; i < oi->hard_prec_vec().size(); i++ )
	{
		unsigned pi = oi->hard_prec_vec()[i];		
		print_fluent( fluents()[pi], os );
		os << std::endl;
	}
	os << "\t" << "Conditions:" << std::endl;
	for ( unsigned i = 0; i < oi->prec_vec().size(); i++ )
	{
		unsigned pi = oi->prec_vec()[i];
                print_fluent( fluents()[pi], os );
		os << std::endl;
	}
	os  << "Adds:" << std::endl;
	for ( unsigned i = 0; i < oi->add_vec().size(); i++ )
	{
		unsigned pi = oi->add_vec()[i];
		print_fluent( fluents()[pi], os );
		os << std::endl;
	}

	os << "Deletes:" << std::endl;
	for ( unsigned i = 0; i < oi->del_vec().size(); i++ )
	{
		unsigned pi = oi->del_vec()[i];		
		print_fluent( fluents()[pi], os );
		os << std::endl;
	}

}

void Task::print_operator( Operator* oi, std::ostream& os )
{
	os << str_tab().get_token( oi->code() );
}

void Task::print_operators( std::ostream& os )
{
	for ( unsigned i = 0; i < useful_ops().size(); i++ )
	{
		Operator* oi = useful_ops()[i];
		os << i << ". Name:" << std::endl;
		print_operator_full( oi, os );
		os << "Cost: " << oi->metric_cost() << std::endl;
	}
}


void Task::set_start_time_lb( std::vector<unsigned>& stimes )
{
	assert( stimes.size() == useful_ops().size() );
	m_start_times_lb = stimes;
	m_is_reachable.resize( useful_ops().size() );
	for ( unsigned i = 0; i < useful_ops().size(); i++ )
		m_is_reachable[i] = ( start_time_lb(i) !=  std::numeric_limits<unsigned>::max() ? true : false );
}


void Task::set_metric_start_time_lb( std::vector<float>& stimes )
{
	assert( stimes.size() == useful_ops().size() );
	m_metric_start_times_lb = stimes;
	m_is_reachable.resize( useful_ops().size() );
	for ( unsigned i = 0; i < useful_ops().size(); i++ )
	  m_is_reachable[i] = ( metric_start_time_lb(i) !=  std::numeric_limits<float>::max() ? true : false );
}



}
