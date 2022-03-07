import os
import sys

def inspect_manifest( ) :
	instream = open( 'MANIFEST' )

	domain_pddl = None
	problem_pddl = None

	for line in instream :
		line = line.strip()
		field, value = line.split('=')
		field=field.strip()
		value = value.strip()
		if not os.path.exists( value ) :
			report_and_bail_out( "File %s specified in field %s does not exist!"%(value,field) )
		if field == 'domain' : domain_pddl = value
		elif field == 'problem' : problem_pddl = value
		elif field == 'domain_file' : domain_pddl = value
		elif field == 'instance_file' : problem_pddl = value
	
	instream.close()
	return domain_pddl, problem_pddl
