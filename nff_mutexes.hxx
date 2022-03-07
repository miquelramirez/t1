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
#ifndef __NFF_MUTEXES__
#define __NFF_MUTEXES__

#include "PDDL.hxx"
#include <vector>
#include <iostream>
#include <algorithm>
#include "nff.hxx"
#include <stdlib.h>


namespace NFF
{
        //	class PDDL::Task;

	class Mutexes
	{
                
        private:
                Mutexes();

        public:
                ~Mutexes();
                static Mutexes& instance();

                typedef std::pair<int,int> Mutex;
                typedef PDDL::Fluent_Set Oneof_Set;

                void create_mutexes();
                void calculate_goal_invariants();
                void create_goals_invariants_table();
                bool check_mutexes( std::vector<unsigned> oneof );

                PDDL::Fluent_Set& mutex( unsigned i )
                { 
                        return *(m_mutex_table[i]); 
                }

		Atom_Vec&	mutex_vec( unsigned i )
		{
			return m_mutex_table_vec[i];
		}

		std::vector< Atom_Vec >& invariants_table()
                {   return m_goals_invariant_table; }

                Atom_Vec&   invariants_lits( unsigned i)
                {   return m_goals_invariant_table[i]; }

                PDDL::Task&	sm_task;
                
                bool are_mutex(unsigned p, unsigned q);
                bool is_mutex( Mutex mu)
                {
                        return (are_mutex(mu.first, mu.second)); 
                }
                void set_mutex(unsigned p, unsigned q)
                {
                        m_mutex_table[p]->set(q);
                        m_mutex_table[q]->set(p);
                }
                void set_mutex(Mutex m)
                {
                        m_mutex_table[m.first]->set(m.second);
                        m_mutex_table[m.second]->set(m.first);
                }
               void unset_mutex(unsigned p, unsigned q)
                {
                        m_mutex_table[p]->unset(q);
                        m_mutex_table[q]->unset(p);
                }
                void unset_mutex(Mutex m)
                {
                        m_mutex_table[m.first]->unset(m.second);
                        m_mutex_table[m.second]->unset(m.first);
                } 
                Atom_Vec& goal_invariants() { return m_goal_invariant_list;}
       private:
                std::vector< PDDL::Fluent_Set*>  m_mutex_table;
		std::vector< Atom_Vec > 	 m_mutex_table_vec;
                Atom_Vec                         m_goal_invariant_list;                
		std::vector< Atom_Vec > 	 m_goals_invariant_table;
                void build_possible_mutexes(std::vector<Mutex>& mutex_set);
                void reduce_mutex_set( std::vector<Mutex>& mutex_set);
                void extend_oneofs( );
        };

}

#endif //nff_mutexes.hxx
