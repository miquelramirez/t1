#!/usr/bin/python
import sys
import os
import copy

def main() :
	input_file = None
	if len(sys.argv) > 1 :
		input_file = sys.argv[1]
	
	init_clauses = []
	with open( input_file ) as instream :
		for line in instream :
			line = line.strip()
			lits = line.split( ' v ' )	
			init_clauses.append(lits)
	unit_clauses = []
	clauses = []
	for c in init_clauses :
		if len(c) == 1 :
			unit_clauses.append(c)
		else :
			clauses.append(c)
	while len(unit_clauses) > 0 :
		l = unit_clauses.pop()
		#after_subsumption = []
		#for c in clauses :
		#	if not l[0] in c :
		#		after_subsumption.append(c)
		if '~' in l :
			not_l = l[0].replace('~','')
		else :
			not_l = '~' + l[0]
		after_up = []
		for c in clauses : #after_subsumption :
			cc = copy.copy(c)
			try :
				while True :
					c.remove( not_l )
			except ValueError :
				pass
			if len(c) == 1 :
				unit_clauses.append(c)
			elif len(c) == 0 :
				print >> sys.stdout, "Empty clause!"
				print >> sys.stdout, "Clause:", " v ".join(cc)
				print >> sys.stdout, "Propagated lit:", l
			else :
				after_up.append(c)
		clauses = after_up

	with open( 'simplified.cnf', 'w' ) as outstream :
		for c in clauses :
			print >> outstream, ' v '.join(c)
				

if __name__ == '__main__' :
	main()
