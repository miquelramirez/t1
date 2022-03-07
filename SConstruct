import os

common_env = Environment()

include_paths = ['mod-conformant-ff', 'libdnnf/include', 'minisat/include' ]
lib_paths = [ 'mod-conformant-ff', 'minisat', 'libdnnf' ]
libs = ['z', 'minisat', 'ff', 'dnnf']

minisat_defines = [ '-D__STDC_LIMIT_MACROS', '-D__STDC_FORMAT_MACROS' ]
compiler_flags = [ '-Wall', '-m64' ]
debug_flags = ['-g', '-DDEBUG']
release_flags = ['-O3', '-pedantic', '-DNDEBUG' ]

# MRJ: Setup compiler flags
common_env.Append( CPPPATH = [ os.path.abspath(p) for p in include_paths ] )
common_env.Append( CCFLAGS=compiler_flags )
common_env.Append( CCFLAGS=minisat_defines )
# MRJ: Comment following lines to switch on/off debugging
#common_env.Append( CCFLAGS=debug_flags )
common_env.Append( CCFLAGS=release_flags )

# MRJ: Setup linker flags
common_env.Append( LIBS=libs )
common_env.Append( LIBPATH=[ os.path.abspath(p) for p in lib_paths ] )
# MRJ: Uncomment following line to enable efence
# common_env.Append( LIBS=['efence'] )
# MRJ: Comment following line to enable efence
common_env.Append( LINKFLAGS=['-static'] )

sources = """
main-probe.cxx
nff_options.cxx
PDDL.cxx
pddl_fluent_set.cxx
pddl_string_table.cxx
bitarray.cxx
nff_belief.cxx
nff.cxx
global_options.cxx
nff_dijkstra.cxx
nff_bsearch.cxx
nff_minisat.cxx
nff_search_node.cxx
nff_log.cxx
nff_ipc.cxx
nff_theory_fragment.cxx
nff_planner_stats.cxx
utils.cxx
nff_mutexes.cxx
"""

sources = [ src for src in sources.split() if len(src) > 0 ]
objects = [ common_env.Object(src) for src in sources ]

common_env.Program( 't1', objects )
