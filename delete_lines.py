#!/usr/bin/python
import sys, os, re

f=open("/home/nirlipo/Sandboxes/instances/suitcase/p20.pddl", "r")
for line in f:
	line=line.rstrip()
	if re.search('x9\-y[1-18]', line) != None:
		print ''
	else:
		print line
f.close()
