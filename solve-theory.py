#!/usr/bin/python
import sys
import os

class Theory :

	def __init__( self, text_clauses ) :
		self.prop_vars = ['']
		self.model = [ False ]
		self.clauses = []
		self.SAT = False
		for c in text_clauses :
			nc = []
			for l in c :
				neg = False
				if '~' in l :
					neg = True
					var = l.replace('~','')
				else :
					var = l	
				try :
					if neg :
						nc.append( -self.prop_vars.index(var) )
					else :
						nc.append( self.prop_vars.index( var ) )
				except ValueError :
					self.prop_vars.append( var )
					self.model.append( False )
					index = len(self.prop_vars)-1
					if neg :
						nc.append( -index )
					else :
						nc.append( index )
			nc.append( 0 )
			self.clauses.append( nc )	
		print >> sys.stdout, "Loaded", len(self.prop_vars), "vars and", len(self.clauses), "clauses"

	def solve( self ) :
		# encode theory in DIMACS
		with open( 'tmp.cnf', 'w' ) as outstream :
			print >> outstream, "p cnf", len(self.prop_vars), len(self.clauses)
			for c in self.clauses :
				print >> outstream, " ".join( [ str(l) for l in c] )
		# invoke SAT solver
		cmd = './minisat-solver tmp.cnf result'
		rv = os.system( cmd )
		# decode result
		with open( 'result' ) as instream :
			for line in instream :
				line = line.strip()
				if 'SAT' in line or 'UNSAT' in line :	
					if 'UNSAT' in line :
						self.SAT = False
					else :
						self.SAT = True
				else :
					if self.SAT :
						model_lits = line.split(' ')
						for l in model_lits :
							l = int(l)
							if l == 0 : continue
							self.model[ abs(l) ] = l > 0
					else :
						print line
		# show result in human-readable mode
		with open( 'result.txt', 'w' ) as outstream :
			if self.SAT == False :
				print >> outstream, "UNSAT"
			else :
				for x in range(1, len(self.prop_vars) ) :
					print >> outstream, self.prop_vars[x], "=", self.model[x]
							
def main() :
	
	input_file = None
	if len(sys.argv) > 1 :
		input_file = sys.argv[1]
	text_clauses = []
	with open( input_file ) as instream :
		for line in instream :
			line = line.strip()
			if len(line) == 0 : continue
			lits = line.split( ' v ' )	
			text_clauses.append(lits)
	tau = Theory( text_clauses )
	tau.solve()

if __name__ == '__main__' :
	main()
