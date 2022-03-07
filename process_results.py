#!/usr/bin/python
import sys
import os
import csv
from options 	import ProcessOptions
from utils	import *
import 	benchmark

class Result :

	def __init__( self, fname ) :
		self.tarball = os.path.basename(fname)
		self.valid = True
		self.load( fname )

	def load( self, fname ) :
		work_dir = 'work'
		os.system( 'rm -rf work')
		os.mkdir( work_dir )
		current = os.getcwd()
		os.system( 'cp %s work/'%fname )
		os.chdir( work_dir )
		os.system( 'ls *.tar.bz2' )
		os.system( 'tar jxvf *.tar.bz2' )

		actions = []
		if os.path.exists( 'planner.result' ) :
			with open( 'planner.result' ) as instream :		
				for line in instream :
					line = line.strip()
					if not ";;" in line : 
						actions.append( line )
						continue #it's an action
					if not "=" in line : continue # not a field
					line = line.replace(";;","")
					line = line.strip()
					key, value = line.split('=')
					if key in ['domain', 'problem', 'b0_mc'] :
						setattr( self, key, value )
					elif key in ['expanded_nodes', 'evaluated_nodes', 'plan_length',
							'size_of_b0_approx', 'fluents', 'operators',
							'heuristic_computations','expanded_ehc', 
							 'generated_ehc', 'expanded_bfs', 'generated_bfs',
							'entailment_tests','sat_solver_calls','max_num_clauses',
							'primary_expansions', 'secondary_expansions',
							'tertiary_expansions',
							'initial_sample_h', 'initial_card_h', 
							'dead_samples_replaced'] :
						setattr( self, key, int(value) )
					elif key in ['ehc_failed'] :
						if value == 'True' : value = True
						else : value = False
						setattr( self, key, value )
					elif key in ['problem_inconsistent', 'width_1'] :
						if value == 'yes' : value = True
						else : value = False
						setattr( self, key, value )
					else :
						if value == '<1 msec' :
							value = 0.001
						setattr( self, key, float(value) )
		if os.path.exists( 'execution.result' ) :
			with open( 'execution.result' ) as instream :
				for line in instream :
					line = line.strip()
					if not "=" in line : continue
					key, value = line.split('=')
					setattr( self, key, value )
		self.cff_validated = True
		#self.verify_solution(current, actions)
		os.chdir( current )
	
	def verify_solution( self, basedir, actions ) :
		domain_pddl, problem_pddl = inspect_manifest()
		ff_exec_path = os.path.join( os.path.abspath( basedir ), 'ff' )
		# create plan file
		plan_file = 'plan'
		with open( 'plan', 'w' ) as outstream :
			for act in actions :
				print >> outstream, act.replace('(','').replace(')','')

		cmd = '%(ff_exec_path)s -o %(domain_pddl)s -f %(problem_pddl)s -v %(plan_file)s > validation.result'%locals()
		print >> sys.stdout, cmd
		rv, time = benchmark.run( cmd, 100, 1024 )
		if rv != 0 :
			self.cff_validated = str(rv)
			return
		self.cff_validated = True
		with open( 'validation.result' ) as instream :
			for line in instream :
				line = line.strip()
				if 'action not applicable!' in line :
					self.cff_validated = False
					return

	def csv_row( self ) :
		values = []
		try :
			values += [ self.domain ]
		except AttributeError :
			print >> sys.stdout, self.tarball, "had no domain"
			return []
		values += [ self.problem ]
		values += [ self.tarball ]
		values += [ self.fluents ]
		values += [ self.operators ]
		values += [ self.signal ]
		values += [ self.time ]
		#values += [ self.preprocessing ]
		#values += [ self.heuristic_setup_time ]
		values += [ self.b0_mc ]
		try :
			values += [ self.size_of_b0_approx ]
		except AttributeError :
			values += [ 'n/a' ]
		#values += [ self.avg_dead_models ]
		#values += [ self.dead_samples_replaced ]
		#values += [ self.initial_sample_h ]
		#values += [ self.initial_card_h ]
		values += [ self.heuristic_computations ]
		#values += [ self.expanded_ehc ]
		#values += [ self.generated_ehc ]
		values += [ self.primary_expansions ]
		values += [ self.secondary_expansions ]
		values += [ self.tertiary_expansions ]
		values += [ self.expanded_bfs ]
		values += [ self.generated_bfs ]
		values += [ self.entailment_tests ]
		values += [ self.sat_solver_calls ]
		#values += [ self.max_num_clauses ]
		#values += [ self.problem_inconsistent ]
		values += [ self.width_1 ]
		#values += [ self.hash_size ]
		try :
			values += [ self.nodes_mem ] #[ ehc_failed ]
		except AttributeError :
			values += [ 'n/a' ]
		try :
			values += [ self.search_time ]
		except AttributeError :
			values += [ 'n/a' ]
		try :
			values += [ self.plan_length ]
		except AttributeError :
			values += [ 'n/a' ]
		#values += [ self.cff_validated ]
		return values

def main() :

	opt = ProcessOptions( sys.argv[1:] )
	
	results = []
	for fname in opt.experiments :
		res = Result(fname)
		if not res.valid : continue
		results.append( res )

	# compile into csv
	summary = csv.writer( open( opt.output, 'w' ) )
	#header = [ 'Domain', 'Problem', 'Tarball', 'F', 'O', 'S', 'TT', 'PT', 'HT', '|b\'_0|', 'ADM', 'DSR', 'hS0', 'hC0', 'HC', 
	#		'E EHC', 'G EHC', 'E1', 'E2', 'E3', 'E BFS', 'G BFS', '#ENT', '#SAT', 'MC', 'PI', 'w<=1', 'NM', 'EHC Failed?', 'ST', 'PL', 'CFF Valid' ]
	header = [ 'Domain', 'Problem', 'Tarball', 'F', 'O', 'S', 'TT', 'MC', '|b\'_0|', 'HC', 'E1', 'E2', 'E3', 'E BFS', 'G BFS', '#ENT', '#SAT', 'w<=1', 'NM', 'ST', 'PL' ]
	summary.writerow(header)
	for r in results :
		summary.writerow( r.csv_row() )	

if __name__ == '__main__' :
	main()
