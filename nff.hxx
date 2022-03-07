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
#ifndef __NFF__
#define __NFF__

#include <vector>
#include <set>
#include <list>
#include <map>
#include <limits>

#include "PDDL.hxx"
namespace NFF
{

	inline	unsigned infty() { return  std::numeric_limits<unsigned>::max(); }
	typedef std::vector<unsigned>				Operator_Vec;
	typedef std::set<unsigned>				Operator_Set;
	typedef std::list<unsigned>				Operator_List;
	typedef std::vector<unsigned>				Atom_Vec;
	typedef std::set<unsigned>				Atom_Set;
	typedef std::vector<bool>   				Bool_Vec;
	typedef std::list<unsigned>				Atom_List;
	typedef std::list<Atom_List>				Atom_Double_List;
#define NO_SAT_CALLS 0


}

#endif // nff.hxx
