#!/usr/bin/python

# PDDL to AL
#Alexandre Albore 2010 UPF
import 	sys, os
import  datetime
import  shutil
from	utils	import *

def report_and_bail_out( msg ) :
	print >> sys.stderr, msg
	print >> sys.stderr, "Bailing out!"
	sys.exit(1)


def main() :

	for arg in sys.argv:
		if 'tar.bz2' in arg:
			input_tarball = arg
			break
		else:
			input_tarball = ''

	if input_tarball == '':
		print """Help --
run the script with a valid argument:
./pddl_to_al.py <file.tar.bz2>"""
		exit(0)

	print >> sys.stdout, "Processing", input_tarball, "(problem %s)"% input_tarball
	cmd = 'tar -jxvf %s'%input_tarball
	rv = os.system( cmd )
	print rv
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
	cmdlimit = 'ulimit -S -v $((1024 * 1800))'
	os.system(cmdlimit)
	cmdlimit = 'ulimit -S -t $((120 * 60))'
	os.system(cmdlimit)

	print "Creating dp.pddl file from %s and %s"%(domain_pddl, problem_pddl)
	t0 = datetime.datetime.now()
	cmd = 'cat %s %s > dp.pddl'%(domain_pddl, problem_pddl)
	os.system( cmd )
	os.system ('./cpa.pddl2pl dp.pddl')
	os.system('cat mult5zswi.pl pddl2pl.pl > new.pl')
	cmd = 'prolog -L128m -G128m -f new.pl -g main,halt. > trash'
	os.system(cmd)

	scripttime = (datetime.datetime.now()-t0)

	trans_time = "Total Translation Time %d s %d ms (%s)" % (scripttime.seconds, (scripttime.microseconds)/1000, scripttime)
	print trans_time
	output_name = base+'.al'
	result_file = output_name +'.data'
	res = open( result_file, 'w' )

	res.write(">>>>> writing %s\n" % output_name)
	res.write(trans_time+"\n")
	res.close()
	outstream = open( output_name, 'w' )
	shutil.copyfileobj(open('theory_0.al','rb'), outstream)
	outstream.close()
	os.system('rm theory* trash new.pl')

if __name__ == '__main__' :
	main()
