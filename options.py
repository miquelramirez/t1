import getopt
import os
import sys
import glob

class ProcessOptions :
	
	def __init__( self, args ) :
		try :
			opts, args = getopt.getopt(	args,
							"hi:o:",
							[ "help",
							"input=",
							"output="
						 ] )
		except geopt.GetoptError :
			print >> sys.stderr, "Missing or incorrect parameters specified!"
			self.usage()
			sys.exit(1)

		self.experiments = []
		self.output = 'summary.csv'
	
		for opcode, oparg in opts :
			if opcode in ('-h', '--help') :
				print >> sys.stderr, "Help invoked!"
				self.usage()
				sys.exit(0)
			elif opcode in ('-i', '--input' ) :
				if not os.path.exists( oparg ) :
					print >> sys.stderr, "Specified path does not exist", oparg
					sys.exit(1)
				if not os.path.isdir( oparg ) :
					print >> sys.stderr, "Path", oparg, "is not a directory"
					sys.exit(1)
				pattern = os.path.join( oparg, '*.tar.bz2' )
				self.experiments = glob.glob( pattern )
				if len(self.experiments) == 0 :
					print >> sys.stderr, "No experimental results were found in", oparg	
			elif opcode in ('-o', '--output' ) :
				self.output = oparg	

	def usage( self ) :
		print >> sys.stderr, "Parameters:"
		print >> sys.stderr, "-h  --help                           Get Help"
		print >> sys.stderr, "-i  --input <folder>                 Path to experiments to process"

		

class SolveOptions :

	def __init__( self, args ) :
		try:
			opts, args = getopt.getopt(	args,
							"p:ht:m:PACOo:rxW:S:f:Dy:E:H:X:I",
							["problem=",
							"help",
							"max-time=",
							"max-memory=",
							"check-only-precs-goals",
							"action-compilation",
							"ipc",
							"static-ors",
							"output=",
							"use-invariant-diameter"
							"use-invariant-XORs"
							"XOR-heuristic-weight=",
							"search-strategy=",
							"helpful-ratio=",
							"dynamic-samples",
							"conditional-effect-penalty=",
							"ehc-heuristic=",
							"heuristic-function=",
							"hX-evaluation=",
							"only-init"] )
		except getopt.GetoptError :
			print >> sys.stderr, "Missing or incorrect parameters specified!"
			self.usage()
			sys.exit(1)

		self.problem = None	
		self.max_time = 1800
		self.max_memory = 1024
		self.only_precs_and_goals = False
		self.use_action_compilation = False
		self.use_static_ors = False
		self.output_dir = None
		self.extra_args = []

		for opcode, oparg in opts :
			if opcode in ( '-h', '--help' ) :
				print >> sys.stderr, "Help invoked!"
				self.usage()
				sys.exit(0)
			elif opcode in ('-p', '--problem' ) :
				self.problem = oparg
				if not os.path.exists( self.problem ) :
					print >> sys.stderr, "File", self.problem, "does not exist"
					print >> sys.stderr, "Aborting"
					sys.exit(1)
			elif opcode in ('-t', '--max-time' ) :
				try :
					self.max_time = int(oparg)
					if self.max_time <= 0 :
						print >> sys.stderr, "Maximum time must be greater than zero"
						sys.exit(1)
				except ValueError :
					print >> sys.stderr, "Time must be an integer"
					sys.exit(1)
			elif opcode in ('-m', '--max-memory' ) :
				try :
					self.max_memory = int(oparg)
					if self.max_memory <= 0 :
						print >> sys.stderr, "Maximum memory must be greater than zero"
						sys.exit(1)
				except ValueError :
					print >> sys.stderr, "Memory amount must be an integer"
					sys.exit(1)
			elif opcode in ('-P', '--check-only-precs-goals' ) :
				self.only_precs_and_goals = True
				self.extra_args.append( '-p' )
			elif opcode in ('-A', '--action-compilation' ) :
				self.use_action_compilation = True
				self.extra_args.append( '-A' )
			elif opcode in ('-O', '--static-ors' ) :
				self.use_static_ors = True
				self.extra_args.append( '-O' )
			elif opcode in ('-r', '--use-invariant-diameter' ) :
				self.extra_args.append( '-r' )
			elif opcode in ('-x', '--use-invariant-XORs' ) :
				self.extra_args.append( '-x' )
			elif opcode in ('-W', '--XOR-heuristic-weight' ) :
				self.extra_args.append( '-W %d'%int(oparg) )
			elif opcode in ('-S', '--search-strategy' ) :
				self.extra_args.append( '-S %s'%oparg )
			elif opcode in ('-f', '--helpful-ratio' ) :
				self.extra_args.append( '-f %d'%int(oparg) )
			elif opcode in ('-D', '--dynamic-samples' ) :
				self.extra_args.append( '-D' )
			elif opcode in ('-C', '--ipc' ) :
				self.extra_args.append( '-C' )
			elif opcode in ('-y', '--conditional-effect-penalty' ) :
				self.extra_args.append( '-y %d'%int(oparg) )
			elif opcode in ('-E', '--ehc-heuristic' ) :
				self.extra_args.append( '-E %s'%oparg )
			elif opcode in ('-H', '--heuristic-function' ) :
				self.extra_args.append( '-H %s'%oparg )
			elif opcode in ('-X', 'hX-evaluation' ) :
				self.extra_args.append( '-X %s'%oparg )
			elif opcode in ('-I', '--only-init' ) :
				self.extra_args.append( '-I' )
			elif opcode in ('-o', '--output' ) :
				if not os.path.exists( oparg ) :
					print >> sys.stderr, "Output directory", oparg, "does not exist"
					os.system( 'mkdir -p %s'%oparg )
				self.output_dir = oparg
		self.extra_args = " ".join( self.extra_args )

		
		if self.problem is None :
			print >> sys.stderr, "No problem was specified!"
			self.usage()
			sys.exit(1)

	def usage( self ) :
		print >> sys.stderr, "Parameters:"
		print >> sys.stderr, "-a  --args    arguments              Planning problem domain definition in PDDL"
		print >> sys.stderr, "-p  --problem    <file>              Planning problem domain definition in PDDL"
		print >> sys.stderr, "-h  --help                           Get Help"
		print >> sys.stderr, "-t  --max-time   <time>              Maximum allowed execution time (defaults to 1800 secs)"
		print >> sys.stderr, "-m  --max-memory <time>              Maximum allowed memory consumption (defaults to 1Gb)"
		print >> sys.stderr, "-P  --check-only-precs-goals         Check only precondition and goals"
		print >> sys.stderr, "-A  --action-compilation             Add action compilation rules to heuristic"
		print >> sys.stderr, "-O  --static-ors                     Add static OR rules to heuristic"  
		print >> sys.stderr, "-C  --ipc                            Output in IPC format"  
		print >> sys.stderr, "-K  --only-init                      Process initial node and stop"		
		
	def print_options( self ) :
		def print_yes() : print >> sys.stdout, "Yes"
		def print_no() : print >> sys.stdout, "No"
		
		print >> sys.stdout, "Options set"
		print >> sys.stdout, "==========="
                print >> sys.stdout, "Arguments: ", self.args
		print >> sys.stdout, "Max. Time Allowed", self.max_time
		print >> sys.stdout, "Max. Memory Allowed", self.max_memory
		print >> sys.stdout, "Problem: ", self.problem
