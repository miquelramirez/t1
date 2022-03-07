#!/usr/bin/python
import os
import sys

def main() :

	to_delete = []
	with open( 'DELETE' ) as instream :
		for line in instream :
			line = line.strip()
			src_file = line
			hdr_file = line.replace('.cxx', '.hxx' )
			to_delete += [ src_file, hdr_file ]
	print >> sys.stdout, "I'm going to delete from repository the following files:"
	for fname in to_delete :
		print >> sys.stdout, "\t", fname
	for fname in to_delete :
		if os.path.exists( fname ) :
			cmd = 'svn rm %s'%fname
			rv = os.system( cmd )
			if rv != 0 :
				print >> sys.stdout, "Removal of", fname, "failed, take a look"

if __name__ == '__main__' :
	main()
