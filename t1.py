#!/usr/bin/python
import 	sys, os, glob
import 	benchmark
from	options import SolveOptions	
from	utils	import *

def report_and_bail_out( msg ) :
	print >> sys.stderr, msg
	print >> sys.stderr, "Bailing out!"
	sys.exit(1)


def main() :

	usr_opts = SolveOptions( sys.argv[1:] )

	problems = [usr_opts.problem]
	if os.path.isdir( usr_opts.problem ) :
		print >> sys.stdout, "Collecting problems from dir", usr_opts.problem
		pattern = os.path.join( usr_opts.problem, '*.tar.bz2' )
		problems = glob.glob( pattern )
		print problems 	

	index = 0
	for input_tarball in problems :
		index += 1
		print >> sys.stdout, "Processing", input_tarball, "(problem %s/%s)"%(index,len(problems))
		cmd = 'tar jxvf %s'%input_tarball
		rv = os.system( cmd )
		if rv != 0 :
			report_and_bail_out( "Could not decompress %s"%input_tarball )
		print >> sys.stdout, "Inspecting manifest"
		domain_pddl, problem_pddl = inspect_manifest()	
		if domain_pddl is None :
			report_and_bail_out( "No domain file found in manifest" )
		if problem_pddl is None :
			report_and_bail_out( "No problem file found in manifest" )
	
		# Check whether domain is compressed
		if '.bz2' in domain_pddl :
			cmd = 'bunzip2 -f %s'%domain_pddl
			rv = os.system( cmd )
			if rv != 0 :
				report_and_bail_out( "Error decompressing domain file" )
			domain_pddl = domain_pddl.replace('.bz2', '')
	
		base = os.path.basename(input_tarball).replace('.tar.bz2','')		
		cmd = './t1 -d %s -i %s %s'%(domain_pddl, problem_pddl, usr_opts.extra_args)
		log_filename = base + '.log'
		log = benchmark.Log( log_filename )
		print >> sys.stdout, 'max mem is: %d'%(usr_opts.max_memory)
		rv, time = benchmark.run( cmd, usr_opts.max_time, usr_opts.max_memory, log )
		outstream = open( 'execution.result', 'w' )
		print >> outstream, "signal=%d"%rv
		print >> outstream, "time=%f"%time
		outstream.close()
		# pack result
		name = os.path.basename(input_tarball)
		name = name.replace('.tar.bz2','')
		result_tarball = 'result-%s.tar.bz2'%name
		result_files = [ 'planner.result', 'execution.result', 'plan.ipc', 'MANIFEST', domain_pddl, problem_pddl ]
		cmd = 'tar jcvf %s %s'%(result_tarball, ' '.join(result_files) )
		os.system( cmd )
		if usr_opts.output_dir is not None :
			os.system( 'mv %s %s'%(result_tarball, os.path.join(usr_opts.output_dir,result_tarball) ) )
			os.system( 'rm -rf %s'%' '.join( ['planner.result', 'execution.result', log_filename ] ) )
	
if __name__ == '__main__' :
	main()
