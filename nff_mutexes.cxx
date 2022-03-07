/*
  Alexandre Albore, Miguel Ramirez, Hector Geffner
  T1 conformant planner
  Copyright (C) 2010  

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

#include <iterator>
#include <set>
#include "nff_mutexes.hxx"
#include "nff_belief.hxx"
#include "nff_options.hxx"
#include <cassert>

namespace NFF
{

Mutexes::Mutexes()
  	: sm_task( PDDL::Task::instance() ) //, m_mutex_table( String_Table::instance() )
{
        for ( unsigned p = 0; p < (unsigned)sm_task.fluent_count(); p++ )
                m_mutex_table.push_back( new PDDL::Fluent_Set( sm_task.fluent_count()+1 ) );
	m_mutex_table_vec.resize( sm_task.fluent_count() );
}

Mutexes::~Mutexes()
{
	for ( unsigned p = 0; p < (unsigned)sm_task.fluent_count(); p++ )
	{
		assert( m_mutex_table[p] != NULL );
		delete m_mutex_table[p];
	}
}
        
Mutexes& Mutexes::instance()
{
	static Mutexes the_instance;
	return the_instance;
}

void Mutexes::create_mutexes()
{
        std::vector<Mutex> possible_mutexes;
	NFF_Options&	opts = NFF_Options::instance();

        build_possible_mutexes( possible_mutexes );

        reduce_mutex_set(possible_mutexes);

        extend_oneofs();

        if (opts.use_invariant_xors() )
        {
                calculate_goal_invariants();

        if ( 0 && opts.use_invariant_diameter() )
                create_goals_invariants_table();
        }
#ifdef DEBUG
        std::cout << "Total final extended ONEOFs: " <<  sm_task.oneofs().size() << std::endl;

        for (unsigned i = 0; i < sm_task.oneofs().size(); i++)
        {
                for (unsigned j = 0; j < sm_task.oneofs()[i].size(); j++)
                {
                        sm_task.print_fluent(sm_task.oneofs()[i][j], std::cout);
                        if (j < sm_task.oneofs()[i].size()-1)
                                std::cout << " v ";
                }
                std::cout << std::endl;
        }
        std::cout << std::endl;
#endif   
	
	for ( int p = 1; p < sm_task.fluent_count()-1; p++ )
	{
		for ( int q = 1; q < sm_task.fluent_count()-1; q++ )
		{
			if ( mutex(p).isset(q) )
				m_mutex_table_vec[p].push_back(q);
		}
	} 
}

/** True: if all the lits in oneof are mutexes
 *  False: otherwise */
bool Mutexes::check_mutexes(std::vector<unsigned> oneof)
{
        for (unsigned i = 0; i < oneof.size(); i++)
                for ( unsigned j = i+1; j < oneof.size(); j++)
                {
                        
                        if ( i == j ) continue;
                        
                        if ( !are_mutex( oneof[i], oneof[j] ) )
                                return false;  
                }
        return true;
}



void Mutexes::extend_oneofs()
{


// 1. Checks invariance of the oneof
	std::vector<std::vector<unsigned> > extended_oneofs;
        PDDL::Fluent_Set conditions_to_check;

	// First checks which oneof is static
      	for ( unsigned i = 0; i < sm_task.oneofs().size(); i++  )
       	{
		bool is_static = true;
                bool oneof_changed = false;
		std::vector<unsigned>& oneof = sm_task.oneofs().at(i);


                // AA: if the lits in oneof are not mutexes, oneof is not static
                if ( !check_mutexes( oneof ) )
                        continue;
                
                do
		for (unsigned j = 0; j < oneof.size(); j++)
		{
                        oneof_changed = false;

			for (unsigned k = 0; k < sm_task.prototype_ops().size(); k++)
			{
 				bool  j_deleted = false;

				// check whether j is deleted or not
				for ( unsigned e = 0; e < (sm_task.prototype_ops()[k])->same_op_vec().size(); e++ ) {
                                        PDDL::Operator* current_op = 
                                                (sm_task.prototype_ops()[k])->same_op_vec()[e];
                                        
                                        if ( current_op->dels().isset( oneof[j] ) )                                          
                                        {
                                                j_deleted = true;
                                                
                                                conditions_to_check = current_op->preconds();
                                                break;
                                        }
                                }    
                                
				if (!j_deleted)
					continue;

				bool was_added = false;

				// checks if another lit into the oneof is added
				for ( unsigned e = 0; e < (sm_task.prototype_ops()[k])->same_op_vec().size(); e++ )
				{
					if (was_added)
						break;

					PDDL::Operator* o = sm_task.prototype_ops()[k]->same_op_vec()[e];

                                        if ( !conditions_to_check.contains( o->preconds() ) )
                                                continue;

					for (unsigned v = 0; v < oneof.size(); v++)
					{
						if ( v == j )
							continue;
                                                
						if ( o->adds().isset( oneof[v]) )
						{
                                                        was_added = true;
                                                        break;
                                                }
                                        }

                                        if (was_added) break; // action adds a xj into oneof
                                                
                                        // checks the possible mutexes
                                        for (unsigned a = 0; a < o->add_vec().size(); a++)
                                        {
                                                bool lit_is_mutex = true;
                                                unsigned add_lit = o->add_vec()[a];

                                                for ( unsigned c = 0; c < oneof.size(); c++)
                                                {
                                                        // the added lit should be mutex with all xi in oneof
                                                        if ( are_mutex( add_lit, oneof[c] ) )
                                                                continue;
                                                        
                                                        lit_is_mutex = false;
                                                        break;
                                                }
                                                
                                                if (lit_is_mutex)
                                                {
#ifdef DEBUG
                                                        std::cout << "Adding literal ";
                                                        sm_task.print_fluent(add_lit, std::cout);
                                                        std::cout << " to the oneof." << std::endl;
//                                                        sm_task.print_operator_full_notab(o,std::cout);
//                                                        std::cout << std::endl;
#endif
                                                        was_added = true;
                                                        oneof_changed = true;
                                                        oneof.push_back( add_lit );
                                                        break;
                                                }       
                                        } // end (check mutexes on o->adds)
                                                                                                       
				} // end (cond-effects)
				is_static &= was_added;
				if ( !is_static )
					break;
			} // for (prototype ops)

			// if at least one literal is not static
			if ( !is_static )
				break;
		} while (oneof_changed);// for (j in oneof)

		if (is_static)
			extended_oneofs.push_back(oneof);
	  
	}  // for (oneofs)

        // AA: copying the new extended oneof in the task
        sm_task.oneofs() = extended_oneofs;

}

/** Builds the list of the negated lits that appear whith the goal
 *  in the extended oneofs. **/
void Mutexes::calculate_goal_invariants()
{
        Oneof_Set already_used_oneofs( sm_task.oneofs().size()+1 );
        PDDL::Fluent_Set already_in_Gx( sm_task.fluent_count()+1 );

	for ( unsigned k = 0; k < sm_task.goal_state().size(); k++ )
		already_in_Gx.set( sm_task.goal_state()[k] );       
 
        for ( unsigned k = 0; k < sm_task.goal_state().size(); k++ )
        {
                unsigned gk = sm_task.goal_state()[k];
                
                for (unsigned oof = 0; oof < sm_task.oneofs().size(); oof++)
                {
                        if ( already_used_oneofs.isset(oof) )
                                continue;
                        
                        std::vector<unsigned>& oneof = sm_task.oneofs().at(oof);
                        // takes the oneofs where a goal appears
                        if ( std::find(oneof.begin(), oneof.end(),gk) == oneof.end() )
                                continue;
                        
                        already_used_oneofs.set(oof);

                        for ( unsigned i = 0; i < oneof.size(); i++)
                        {
                                unsigned mi = oneof[i];
                                //AA: Uncomment to exclude the goal lits from Vg
				if ( sm_task.useful_ops()[sm_task.end()]->preconds().isset(mi) )
					continue;
                                unsigned neg_mi = sm_task.not_equivalent(mi);
                                if ( neg_mi != 0 && !already_in_Gx.isset(neg_mi) )
                                {	
                                        goal_invariants().push_back( neg_mi );
                                        already_in_Gx.set(neg_mi);
                                }
                        }
                }
        }
#ifdef DEBUG
        std::cout << "List of literals (negated) that are in an invariant along with the goal:" << std::endl;
        std::cout << " [";
        for (unsigned k = 0; k < goal_invariants().size(); k++ )
        {
                sm_task.print_fluent( goal_invariants()[k], std::cout );
                if (k < goal_invariants().size()-1)
                        std::cout << ", ";
        }
        std::cout << " ]" << std::endl;
#endif

}



/** Builds the list of the lits that appear along 
    whith the goal atoms in the extended oneofs. **/
void Mutexes::create_goals_invariants_table()
{
        //AA: NB: in case of disjuncive goals, do not use this Oneof_set
        Oneof_Set already_used_oneofs( sm_task.oneofs().size()+1 );
        std::vector<PDDL::Fluent_Set*> already_in_Gx;

        invariants_table().resize( sm_task.goal_state().size() );

	for ( unsigned k = 0; k < sm_task.goal_state().size(); k++ )
        {
                already_in_Gx.push_back( new PDDL::Fluent_Set( sm_task.fluents().size()+1 ) );
 // Considering the goal lits
//		already_in_Gx[k]->set( sm_task.goal_state()[k] );       
        }

        for ( unsigned k = 0; k < sm_task.goal_state().size(); k++ )
        {
                unsigned gk = sm_task.goal_state()[k];
                
                for (unsigned oof = 0; oof < sm_task.oneofs().size(); oof++)
                {
                        if ( already_used_oneofs.isset(oof) )
                                continue;
                        
                        std::vector<unsigned>& oneof = sm_task.oneofs().at(oof);
                        // takes the oneofs where a goal appears
                        if ( std::find(oneof.begin(), oneof.end(),gk) == oneof.end() )
                                continue;
                        
                        already_used_oneofs.set(oof);

                        for ( unsigned i = 0; i < oneof.size(); i++)
                        {
                                unsigned mi = oneof[i];
// Uncomment if you don't want the (-goal) in the invariant_lits()
//                                unsigned neg_mi = sm_task.not_equivalent(mi);
//				if ( neg_mi && sm_task.useful_ops()[sm_task.end()]->preconds().isset(neg_mi) )
//					continue;

                                if ( !already_in_Gx[k]->isset( mi ) )
                                {	
                                        invariants_lits(k).push_back( mi );
                                        already_in_Gx[k]->set(mi);
                                }
                        }
                }
        }
#ifdef DEBUG
        std::cout << "List of literals that are in an invariant along with the goal atoms:" << std::endl;
	for ( unsigned k = 0; k < sm_task.goal_state().size(); k++ )
        {
                unsigned gk = sm_task.goal_state()[k];
                sm_task.print_fluent( gk, std::cout );
                std::cout << "= [";
                for (unsigned ki = 0; ki < invariants_lits(k).size(); ki++ )
                {
                        sm_task.print_fluent( invariants_lits(k)[ki], std::cout );
                        if (ki < invariants_lits(k).size()-1)
                                std::cout << ", ";
                }
                std::cout << " ]" << std::endl;
        }
#endif

	for ( unsigned k = 0; k < sm_task.goal_state().size(); k++ )
                delete  already_in_Gx[k];
        
}


void Mutexes::build_possible_mutexes( std::vector<Mutex>& possible_mutexes )
{
	std::vector< std::vector<bool> > processed;

        Belief& init = sm_task.initial_belief();

	processed.resize( sm_task.fluent_count() );
	for (unsigned i = 1; i < processed.size(); ++i)
	{
		processed[i].resize( sm_task.fluent_count(), false );
	}
    

        for (unsigned c = 0; c < sm_task.oneofs().size(); c++)
        {
                for (unsigned i = 0; i < sm_task.oneofs().at(c).size(); i++)
                {
                        unsigned p = sm_task.oneofs()[c][i];

                        assert( !init.known_set().isset( p ) );

                        if ( !init.unknown_set().isset(p) )
                                continue;

                        for (unsigned j = 0; j <  sm_task.oneofs().at(c).size(); j++)
                        {
                                
                                if (i == j) continue;

                                unsigned q =  sm_task.oneofs()[c][j];
                                
                                if ( processed[p][q] ) continue;
                                
                                processed[p][q] = true;
                                processed[q][p] = true;
                                possible_mutexes.push_back( Mutex(p,q));
                        }
                }
        }
        

        // AA: for all the fluents that are known to be false in init
        for (unsigned p = 1; p < (unsigned)(sm_task.fluent_count()-1); p++)
        {
                
                //                unsigned p = sm_task.fluents().at(i);
                
                if ( init.unknown_set().isset(p) || init.known_set().isset( p ) )
                        continue;
  
                for (unsigned q = 1; q < sm_task.fluents().size()-1; q++) 
                {
                        if ( processed[q][p] || ( p == q ) || processed[p][q] )
                                continue;

                        possible_mutexes.push_back( Mutex(p,q));
                        processed[p][q] = true;
                        processed[q][p] = true;
                }
        } // end (for init.known.to.be.false)

/*
#ifdef DEBUG
        std::cout << std::endl << " List of possible mutexes: " << std::endl;
        for (unsigned k = 0; k < possible_mutexes.size(); k++) 
        {
                std::cout << "[ ";
                sm_task.print_fluent(possible_mutexes[k].first, std::cout);
                std::cout << ", ";
                sm_task.print_fluent(possible_mutexes[k].second, std::cout);
                std::cout << " ]" << std::endl;
        }
#endif
*/
}


bool Mutexes::are_mutex( unsigned p, unsigned q)
{
        if ( mutex(p).isset(q) )
                return true;

        if ( mutex(q).isset(p) )
                return true;

        return false;
}

void Mutexes::reduce_mutex_set( std::vector<Mutex>& mutex_set)
{
#ifdef DEBUG
        std::vector<bool> to_be_eliminated( mutex_set.size(), false );
#endif
        std::vector<Mutex> ret; // = new std::vector<Mutex>();
        unsigned p, q;
        bool q_has_no_mutex;
        int counter = 0;
        bool updated = false;


        for ( unsigned mu = 0; mu < mutex_set.size(); mu++ )
                set_mutex( mutex_set[mu] );

        do
        {
                updated =false;                        
                for (unsigned i = 0; i < mutex_set.size(); i++)
                {

                        if (!is_mutex( mutex_set[i]))
                                continue;

                        bool eliminated = false;

                        for ( unsigned k = 0; k < sm_task.prototype_ops().size(); k++)
                        {
                                std::vector<PDDL::Operator*>& ops = (sm_task.prototype_ops()[k])->same_op_vec();
                        
                                // For every action, I check if the mutex is invariant
                                for (std::vector<PDDL::Operator*>::iterator o = ops.begin(); o != ops.end(); o++)
                                { 
                                        if ( (*o)->adds().isset(mutex_set[i].first) )
                                        {
                                                p = mutex_set[i].first;
                                                q = mutex_set[i].second;
                                        } 
                                        else if ( (*o)->adds().isset( mutex_set[i].second ) )
                                        {
                                                p = mutex_set[i].second;
                                                q = mutex_set[i].first;
                                        }
                                        else continue;

                                        // something was added

                                        if ( (*o)->dels().isset(q) )
                                                continue;


                                        bool q_is_added = false;
                                        if ( (*o)->adds().isset(q) )
                                                q_is_added = true;

                                        if ( !q_is_added )
                                        {
                                                q_has_no_mutex = true;
                                                for (unsigned j = 0; j < (*o)->prec_vec().size(); j++ )
                                                {
                                                        // takes only the condition
//                                                if ( (*o)->hard_preconds().isset( (*o)->prec_vec().at(j) ) )
//                                                        continue;

                                                        // if q is mutex with a condition, it is mutex with p
                                                        if ( are_mutex( q, (*o)->prec_vec().at(j) ) )
                                                        {
                                                                q_has_no_mutex = false;
                                                                break;
                                                        }
                                        
                                                }
                                        }
                                        if (q_has_no_mutex || q_is_added)
                                        {

                                                assert( is_mutex( mutex_set[i] ));
                                                // Removes the Mutex
                                                eliminated = true;
                                                updated=true;
                                                unset_mutex( mutex_set[i] );
#ifdef DEBUG
                                                assert( !is_mutex( mutex_set[i] ));
                                                to_be_eliminated[i]= true;
#endif
                                        }
                                        if (eliminated) break;
                                } // end (for conditional effects)
                        
                                if (eliminated) break;
                        } // end (for prototype_ops)

                } // end (for mutex_set)
                std::cout << "Iteraction: " << counter++ << std::endl;
        } while ( updated );


/*
#ifdef DEBUG
        // now makes the final mutex vector
        for (unsigned m = 0; m < mutex_set.size(); m++)
        {
                if ( to_be_eliminated[ m ] )
                        continue;
                
                ret.push_back( mutex_set[m] );
        }
        
        std::cout << "List of simplified mutexes:"<< std::endl;
        for (unsigned k = 0; k < ret.size(); k++)
        {
                std::cout << "[ ";
                sm_task.print_fluent(ret[k].first, std::cout);
                std::cout << ", ";
                sm_task.print_fluent(ret[k].second, std::cout);
                std::cout << " ]" << std::endl;
        }
#endif
*/

}



}
