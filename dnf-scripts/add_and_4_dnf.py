#!/usr/bin/python
import os
import sys, glob
import fileinput
from utils import *
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
./dnf.py <file.tar.bz2>"""
            exit(0)
            
    print >> sys.stdout, "Processing", input_tarball, "(problem %s)"% input_tarball

    cmd = 'tar -jxvf %s'%input_tarball
    rv = os.system( cmd )
    rv = 0
    if rv != 0 :
        report_and_bail_out( "Could not decompress %s"%input_tarball )
        
    print >> sys.stdout, "Inspecting manifest"
    domain_pddl, problem_pddl = inspect_manifest()	
    if domain_pddl is None :
        report_and_bail_out( "No domain file found in manifest" )
    if problem_pddl is None :
        report_and_bail_out( "No problem file found in manifest" )
	
    for i, line in enumerate(fileinput.input(problem_pddl, inplace = 1)):
       sys.stdout.write(line.replace(':init', ':init (and')) 
    for i, line in enumerate(fileinput.input(problem_pddl, inplace = 1)):
        sys.stdout.write(line.replace('(:goal', ') (:goal'))

# be sure to have an unknown for every line
    for i, line in enumerate(fileinput.input(problem_pddl, inplace = 1)): 
        sys.stdout.write(line.replace('(unknown', '\n(unknown'))

    for i, line in enumerate(fileinput.input(problem_pddl, inplace = 1)): 
        sys.stdout.write(line.replace('(oneof', '\n(oneof'))

    for i, line in enumerate(fileinput.input(problem_pddl, inplace = 1)): 
        if 'unknown' in line:
            unk = line.find("unknown")
            first = line.find('(',unk)
            last = line.find(')')
            last = line.find(')',last+1)
            t = line[first:last]
            not_t = '(not %s )' % t
            repl = t+' '+not_t
            sys.stdout.write(line.replace(t, repl))
        else:
            sys.stdout.write(line)

    for i, line in enumerate(fileinput.input(problem_pddl, inplace = 1)): 
        sys.stdout.write(line.replace('unknown', 'or'))
 

    cmd = 'tar -jcvf %s %s %s MANIFEST'%(input_tarball,domain_pddl,problem_pddl)
    os.system( cmd )
    cmd = 'rm *pddl MANIFEST'
    os.system( cmd )

if __name__ == '__main__' :
	main()
