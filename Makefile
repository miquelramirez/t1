CC = g++
BISON = bison
FLEX = flex

NFF_TARGET = t1

MINISAT_CXX_FLAGS = -D__STDC_LIMIT_MACROS -D__STDC_FORMAT_MACROS -Iminisat/include
CXXFLAGS = -g -Wall -Imod-conformant-ff -Ilibdnnf/include -DDEBUG -DENDEBUG $(MINISAT_CXX_FLAGS)
#CXXFLAGS =   -O3 -Wall -pedantic -Imod-conformant-ff -Ilibdnnf/include $(MINISAT_CXX_FLAGS) -DNDEBUG -DENDEBUG
#LDFLAGS = -Lmod-metric-ff/
LDFLAGS = -Lmod-conformant-ff/ -Lminisat/ -Llibdnnf/
#EFENCE = -lefence
STATIC = -static
LIBS = -lz -lminisat -lff -ldnnf $(EFENCE)
#-lminisat_static 
NFF_SOURCES = main-probe.cxx \
	nff_options.cxx \
	PDDL.cxx \
	pddl_fluent_set.cxx \
	pddl_string_table.cxx \
	bitarray.cxx \
	nff_belief.cxx \
	nff.cxx \
	global_options.cxx \
	nff_algobase.cxx \
	nff_dijkstra.cxx \
	nff_bsearch.cxx \
	nff_minisat.cxx \
	nff_search_node.cxx \
	nff_log.cxx \
	nff_theory_fragment.cxx \
	nff_planner_stats.cxx \
	utils.cxx \
	nff_mutexes.cxx \
#	nff_rp.cxx 

NFF_LIB_SOURCES = nff_options.cxx \
        PDDL.cxx \
        pddl_fluent_set.cxx \
        pddl_string_table.cxx \
        bitarray.cxx \
        nff_belief.cxx \
        nff.cxx \
        global_options.cxx \
        nff_algobase.cxx \
        nff_dijkstra.cxx \
	nff_bsearch.cxx \
	nff_minisat.cxx \
	nff_search_node.cxx \
	nff_log.cxx \
	nff_theory_fragment.cxx \
	nff_planner_stats.cxx \
	utils.cxx \
	nff_mutexes.cxx \
#	nff_rp.cxx

NFF_LIB_OBJECTS = $(NFF_LIB_SOURCES:.cxx=.o)
NFF_OBJECTS = $(NFF_SOURCES:.cxx=.o)

# Implicit rules
#
.SUFFIXES:

.SUFFIXES: .cxx .C .o

.cxx.o:; $(CC) -o $@ -c $(CXXFLAGS) $<
.C.o:; $(CC) -o $@ -c $(CXXFLAGS) $<

# build rules


all : $(NFF_TARGET) libnff.a

libnff.a : $(NFF_LIB_OBJECTS)
	ar cru libnff.a $(NFF_LIB_OBJECTS)

$(NFF_TARGET) : $(PARSER_OBJ) $(NFF_OBJECTS)
	$(CC) -o $(NFF_TARGET) $(STATIC) $(PARSER_OBJ) $(NFF_OBJECTS) $(LDFLAGS) $(LIBS)

# dependencies
depend:
	# Updating dependencies
	@makedepend -- $(CXXFLAGS) -- $(NFF_SOURCES)

# Cleaning
clean:
	rm -rf $(NFF_OBJECTS) $(NFF_TARGET) $(PARSER_SRC) $(PARSER_HDR) *.o *.hh
# DO NOT DELETE

main-probe.o: /usr/include/signal.h /usr/include/features.h
main-probe.o: /usr/include/bits/predefs.h /usr/include/sys/cdefs.h
main-probe.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
main-probe.o: /usr/include/gnu/stubs-32.h /usr/include/bits/sigset.h
main-probe.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
main-probe.o: /usr/include/bits/signum.h /usr/include/time.h
main-probe.o: /usr/include/bits/siginfo.h /usr/include/bits/sigaction.h
main-probe.o: /usr/include/bits/sigcontext.h /usr/include/bits/sigstack.h
main-probe.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sigthread.h
main-probe.o: utils.hxx /usr/include/sys/times.h /usr/include/sys/resource.h
main-probe.o: /usr/include/bits/resource.h /usr/include/bits/time.h
main-probe.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
main-probe.o: /usr/include/bits/confname.h /usr/include/getopt.h
main-probe.o: /usr/include/math.h /usr/include/bits/huge_val.h
main-probe.o: /usr/include/bits/huge_valf.h /usr/include/bits/huge_vall.h
main-probe.o: /usr/include/bits/inf.h /usr/include/bits/nan.h
main-probe.o: /usr/include/bits/mathdef.h /usr/include/bits/mathcalls.h
main-probe.o: PDDL.hxx pddl_basic_types.hxx pddl_string_table.hxx
main-probe.o: pddl_fluent_set.hxx bitarray.hxx
main-probe.o: minisat/include/minisat/core/SolverTypes.h
main-probe.o: /usr/include/assert.h minisat/include/minisat/mtl/IntTypes.h
main-probe.o: /usr/include/stdint.h /usr/include/bits/wchar.h
main-probe.o: /usr/include/inttypes.h /usr/include/limits.h
main-probe.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
main-probe.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
main-probe.o: minisat/include/minisat/mtl/Alg.h
main-probe.o: minisat/include/minisat/mtl/Vec.h
main-probe.o: minisat/include/minisat/mtl/XAlloc.h /usr/include/errno.h
main-probe.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
main-probe.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
main-probe.o: /usr/include/asm-generic/errno-base.h /usr/include/stdlib.h
main-probe.o: /usr/include/sys/types.h /usr/include/endian.h
main-probe.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
main-probe.o: /usr/include/sys/select.h /usr/include/bits/select.h
main-probe.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
main-probe.o: minisat/include/minisat/mtl/Map.h
main-probe.o: minisat/include/minisat/mtl/Alloc.h nff_options.hxx
main-probe.o: nff_belief.hxx nff_minisat.hxx
main-probe.o: minisat/include/minisat/core/Solver.h
main-probe.o: minisat/include/minisat/mtl/Heap.h
main-probe.o: minisat/include/minisat/utils/Options.h /usr/include/stdio.h
main-probe.o: /usr/include/libio.h /usr/include/_G_config.h
main-probe.o: /usr/include/wchar.h /usr/include/bits/stdio_lim.h
main-probe.o: /usr/include/bits/sys_errlist.h /usr/include/string.h
main-probe.o: /usr/include/xlocale.h
main-probe.o: minisat/include/minisat/utils/ParseUtils.h /usr/include/zlib.h
main-probe.o: /usr/include/zconf.h /usr/include/zlibdefs.h
main-probe.o: nff_planner_stats.hxx nff_search_node.hxx nff.hxx
main-probe.o: nff_dijkstra.hxx libdnnf/include/libdnnf/dnnf_theory.hxx
main-probe.o: libdnnf/include/libdnnf/cnf_theory.hxx
main-probe.o: libdnnf/include/libdnnf/ext_math.hxx nff_theory_fragment.hxx
main-probe.o: minisat/include/minisat/mtl/Sort.h nff_bsearch.hxx
main-probe.o: hash_table.hxx jenkins_12bit.hxx
main-probe.o: libdnnf/include/libdnnf/cnf2dnnf.hxx nff_log.hxx
main-probe.o: nff_mutexes.hxx
nff_options.o: nff_options.hxx global_options.hxx /usr/include/getopt.h
nff_options.o: /usr/include/stdio.h /usr/include/features.h
nff_options.o: /usr/include/bits/predefs.h /usr/include/sys/cdefs.h
nff_options.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
nff_options.o: /usr/include/gnu/stubs-32.h /usr/include/bits/types.h
nff_options.o: /usr/include/bits/typesizes.h /usr/include/libio.h
nff_options.o: /usr/include/_G_config.h /usr/include/wchar.h
nff_options.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
nff_options.o: /usr/include/stdlib.h /usr/include/sys/types.h
nff_options.o: /usr/include/time.h /usr/include/endian.h
nff_options.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
nff_options.o: /usr/include/sys/select.h /usr/include/bits/select.h
nff_options.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
nff_options.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
nff_options.o: /usr/include/alloca.h
PDDL.o: PDDL.hxx pddl_basic_types.hxx pddl_string_table.hxx
PDDL.o: pddl_fluent_set.hxx bitarray.hxx
PDDL.o: minisat/include/minisat/core/SolverTypes.h /usr/include/assert.h
PDDL.o: /usr/include/features.h /usr/include/bits/predefs.h
PDDL.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
PDDL.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
PDDL.o: minisat/include/minisat/mtl/IntTypes.h /usr/include/stdint.h
PDDL.o: /usr/include/bits/wchar.h /usr/include/inttypes.h
PDDL.o: /usr/include/limits.h /usr/include/bits/posix1_lim.h
PDDL.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
PDDL.o: /usr/include/bits/posix2_lim.h minisat/include/minisat/mtl/Alg.h
PDDL.o: minisat/include/minisat/mtl/Vec.h
PDDL.o: minisat/include/minisat/mtl/XAlloc.h /usr/include/errno.h
PDDL.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
PDDL.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
PDDL.o: /usr/include/asm-generic/errno-base.h /usr/include/stdlib.h
PDDL.o: /usr/include/sys/types.h /usr/include/bits/types.h
PDDL.o: /usr/include/bits/typesizes.h /usr/include/time.h
PDDL.o: /usr/include/endian.h /usr/include/bits/endian.h
PDDL.o: /usr/include/bits/byteswap.h /usr/include/sys/select.h
PDDL.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
PDDL.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
PDDL.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
PDDL.o: minisat/include/minisat/mtl/Map.h minisat/include/minisat/mtl/Alloc.h
PDDL.o: utils.hxx /usr/include/sys/times.h /usr/include/sys/resource.h
PDDL.o: /usr/include/bits/resource.h /usr/include/unistd.h
PDDL.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
PDDL.o: /usr/include/getopt.h /usr/include/math.h
PDDL.o: /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h
PDDL.o: /usr/include/bits/huge_vall.h /usr/include/bits/inf.h
PDDL.o: /usr/include/bits/nan.h /usr/include/bits/mathdef.h
PDDL.o: /usr/include/bits/mathcalls.h global_options.hxx
PDDL.o: mod-conformant-ff/libff.h mod-conformant-ff/ff.h /usr/include/stdio.h
PDDL.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
PDDL.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
PDDL.o: /usr/include/strings.h /usr/include/xlocale.h /usr/include/string.h
PDDL.o: /usr/include/ctype.h /usr/include/sys/timeb.h nff.hxx nff_belief.hxx
PDDL.o: nff_mutexes.hxx nff_options.hxx
pddl_fluent_set.o: pddl_fluent_set.hxx bitarray.hxx
pddl_string_table.o: pddl_string_table.hxx pddl_basic_types.hxx
bitarray.o: bitarray.hxx
nff_belief.o: PDDL.hxx pddl_basic_types.hxx pddl_string_table.hxx
nff_belief.o: pddl_fluent_set.hxx bitarray.hxx
nff_belief.o: minisat/include/minisat/core/SolverTypes.h
nff_belief.o: /usr/include/assert.h /usr/include/features.h
nff_belief.o: /usr/include/bits/predefs.h /usr/include/sys/cdefs.h
nff_belief.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
nff_belief.o: /usr/include/gnu/stubs-32.h
nff_belief.o: minisat/include/minisat/mtl/IntTypes.h /usr/include/stdint.h
nff_belief.o: /usr/include/bits/wchar.h /usr/include/inttypes.h
nff_belief.o: /usr/include/limits.h /usr/include/bits/posix1_lim.h
nff_belief.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
nff_belief.o: /usr/include/bits/posix2_lim.h
nff_belief.o: minisat/include/minisat/mtl/Alg.h
nff_belief.o: minisat/include/minisat/mtl/Vec.h
nff_belief.o: minisat/include/minisat/mtl/XAlloc.h /usr/include/errno.h
nff_belief.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
nff_belief.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
nff_belief.o: /usr/include/asm-generic/errno-base.h /usr/include/stdlib.h
nff_belief.o: /usr/include/sys/types.h /usr/include/bits/types.h
nff_belief.o: /usr/include/bits/typesizes.h /usr/include/time.h
nff_belief.o: /usr/include/endian.h /usr/include/bits/endian.h
nff_belief.o: /usr/include/bits/byteswap.h /usr/include/sys/select.h
nff_belief.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
nff_belief.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
nff_belief.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
nff_belief.o: minisat/include/minisat/mtl/Map.h
nff_belief.o: minisat/include/minisat/mtl/Alloc.h nff_belief.hxx
nff_belief.o: hash_table.hxx jenkins_12bit.hxx
nff.o: nff.hxx PDDL.hxx pddl_basic_types.hxx pddl_string_table.hxx
nff.o: pddl_fluent_set.hxx bitarray.hxx
nff.o: minisat/include/minisat/core/SolverTypes.h /usr/include/assert.h
nff.o: /usr/include/features.h /usr/include/bits/predefs.h
nff.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
nff.o: /usr/include/gnu/stubs.h /usr/include/gnu/stubs-32.h
nff.o: minisat/include/minisat/mtl/IntTypes.h /usr/include/stdint.h
nff.o: /usr/include/bits/wchar.h /usr/include/inttypes.h
nff.o: /usr/include/limits.h /usr/include/bits/posix1_lim.h
nff.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
nff.o: /usr/include/bits/posix2_lim.h minisat/include/minisat/mtl/Alg.h
nff.o: minisat/include/minisat/mtl/Vec.h minisat/include/minisat/mtl/XAlloc.h
nff.o: /usr/include/errno.h /usr/include/bits/errno.h
nff.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
nff.o: /usr/include/asm-generic/errno.h /usr/include/asm-generic/errno-base.h
nff.o: /usr/include/stdlib.h /usr/include/sys/types.h
nff.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
nff.o: /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h
nff.o: /usr/include/bits/byteswap.h /usr/include/sys/select.h
nff.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
nff.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
nff.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
nff.o: minisat/include/minisat/mtl/Map.h minisat/include/minisat/mtl/Alloc.h
global_options.o: global_options.hxx
nff_algobase.o: nff_algobase.hxx nff.hxx PDDL.hxx pddl_basic_types.hxx
nff_algobase.o: pddl_string_table.hxx pddl_fluent_set.hxx bitarray.hxx
nff_algobase.o: minisat/include/minisat/core/SolverTypes.h
nff_algobase.o: /usr/include/assert.h /usr/include/features.h
nff_algobase.o: /usr/include/bits/predefs.h /usr/include/sys/cdefs.h
nff_algobase.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
nff_algobase.o: /usr/include/gnu/stubs-32.h
nff_algobase.o: minisat/include/minisat/mtl/IntTypes.h /usr/include/stdint.h
nff_algobase.o: /usr/include/bits/wchar.h /usr/include/inttypes.h
nff_algobase.o: /usr/include/limits.h /usr/include/bits/posix1_lim.h
nff_algobase.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
nff_algobase.o: /usr/include/bits/posix2_lim.h
nff_algobase.o: minisat/include/minisat/mtl/Alg.h
nff_algobase.o: minisat/include/minisat/mtl/Vec.h
nff_algobase.o: minisat/include/minisat/mtl/XAlloc.h /usr/include/errno.h
nff_algobase.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
nff_algobase.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
nff_algobase.o: /usr/include/asm-generic/errno-base.h /usr/include/stdlib.h
nff_algobase.o: /usr/include/sys/types.h /usr/include/bits/types.h
nff_algobase.o: /usr/include/bits/typesizes.h /usr/include/time.h
nff_algobase.o: /usr/include/endian.h /usr/include/bits/endian.h
nff_algobase.o: /usr/include/bits/byteswap.h /usr/include/sys/select.h
nff_algobase.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
nff_algobase.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
nff_algobase.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
nff_algobase.o: minisat/include/minisat/mtl/Map.h
nff_algobase.o: minisat/include/minisat/mtl/Alloc.h
nff_dijkstra.o: nff_dijkstra.hxx PDDL.hxx pddl_basic_types.hxx
nff_dijkstra.o: pddl_string_table.hxx pddl_fluent_set.hxx bitarray.hxx
nff_dijkstra.o: minisat/include/minisat/core/SolverTypes.h
nff_dijkstra.o: /usr/include/assert.h /usr/include/features.h
nff_dijkstra.o: /usr/include/bits/predefs.h /usr/include/sys/cdefs.h
nff_dijkstra.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
nff_dijkstra.o: /usr/include/gnu/stubs-32.h
nff_dijkstra.o: minisat/include/minisat/mtl/IntTypes.h /usr/include/stdint.h
nff_dijkstra.o: /usr/include/bits/wchar.h /usr/include/inttypes.h
nff_dijkstra.o: /usr/include/limits.h /usr/include/bits/posix1_lim.h
nff_dijkstra.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
nff_dijkstra.o: /usr/include/bits/posix2_lim.h
nff_dijkstra.o: minisat/include/minisat/mtl/Alg.h
nff_dijkstra.o: minisat/include/minisat/mtl/Vec.h
nff_dijkstra.o: minisat/include/minisat/mtl/XAlloc.h /usr/include/errno.h
nff_dijkstra.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
nff_dijkstra.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
nff_dijkstra.o: /usr/include/asm-generic/errno-base.h /usr/include/stdlib.h
nff_dijkstra.o: /usr/include/sys/types.h /usr/include/bits/types.h
nff_dijkstra.o: /usr/include/bits/typesizes.h /usr/include/time.h
nff_dijkstra.o: /usr/include/endian.h /usr/include/bits/endian.h
nff_dijkstra.o: /usr/include/bits/byteswap.h /usr/include/sys/select.h
nff_dijkstra.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
nff_dijkstra.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
nff_dijkstra.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
nff_dijkstra.o: minisat/include/minisat/mtl/Map.h
nff_dijkstra.o: minisat/include/minisat/mtl/Alloc.h
nff_dijkstra.o: libdnnf/include/libdnnf/dnnf_theory.hxx
nff_dijkstra.o: libdnnf/include/libdnnf/cnf_theory.hxx
nff_dijkstra.o: libdnnf/include/libdnnf/ext_math.hxx nff.hxx nff_belief.hxx
nff_dijkstra.o: nff_options.hxx
nff_dijkstra.o: nff_mutexes.hxx
nff_bsearch.o: nff_bsearch.hxx nff_search_node.hxx nff.hxx PDDL.hxx
nff_bsearch.o: pddl_basic_types.hxx pddl_string_table.hxx pddl_fluent_set.hxx
nff_bsearch.o: bitarray.hxx minisat/include/minisat/core/SolverTypes.h
nff_bsearch.o: /usr/include/assert.h /usr/include/features.h
nff_bsearch.o: /usr/include/bits/predefs.h /usr/include/sys/cdefs.h
nff_bsearch.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
nff_bsearch.o: /usr/include/gnu/stubs-32.h
nff_bsearch.o: minisat/include/minisat/mtl/IntTypes.h /usr/include/stdint.h
nff_bsearch.o: /usr/include/bits/wchar.h /usr/include/inttypes.h
nff_bsearch.o: /usr/include/limits.h /usr/include/bits/posix1_lim.h
nff_bsearch.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
nff_bsearch.o: /usr/include/bits/posix2_lim.h
nff_bsearch.o: minisat/include/minisat/mtl/Alg.h
nff_bsearch.o: minisat/include/minisat/mtl/Vec.h
nff_bsearch.o: minisat/include/minisat/mtl/XAlloc.h /usr/include/errno.h
nff_bsearch.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
nff_bsearch.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
nff_bsearch.o: /usr/include/asm-generic/errno-base.h /usr/include/stdlib.h
nff_bsearch.o: /usr/include/sys/types.h /usr/include/bits/types.h
nff_bsearch.o: /usr/include/bits/typesizes.h /usr/include/time.h
nff_bsearch.o: /usr/include/endian.h /usr/include/bits/endian.h
nff_bsearch.o: /usr/include/bits/byteswap.h /usr/include/sys/select.h
nff_bsearch.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
nff_bsearch.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
nff_bsearch.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
nff_bsearch.o: minisat/include/minisat/mtl/Map.h
nff_bsearch.o: minisat/include/minisat/mtl/Alloc.h nff_dijkstra.hxx
nff_bsearch.o: libdnnf/include/libdnnf/dnnf_theory.hxx
nff_bsearch.o: libdnnf/include/libdnnf/cnf_theory.hxx
nff_bsearch.o: libdnnf/include/libdnnf/ext_math.hxx nff_belief.hxx
nff_bsearch.o: nff_minisat.hxx minisat/include/minisat/core/Solver.h
nff_bsearch.o: minisat/include/minisat/mtl/Heap.h
nff_bsearch.o: minisat/include/minisat/utils/Options.h /usr/include/stdio.h
nff_bsearch.o: /usr/include/libio.h /usr/include/_G_config.h
nff_bsearch.o: /usr/include/wchar.h /usr/include/bits/stdio_lim.h
nff_bsearch.o: /usr/include/bits/sys_errlist.h /usr/include/math.h
nff_bsearch.o: /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h
nff_bsearch.o: /usr/include/bits/huge_vall.h /usr/include/bits/inf.h
nff_bsearch.o: /usr/include/bits/nan.h /usr/include/bits/mathdef.h
nff_bsearch.o: /usr/include/bits/mathcalls.h /usr/include/string.h
nff_bsearch.o: /usr/include/xlocale.h
nff_bsearch.o: minisat/include/minisat/utils/ParseUtils.h /usr/include/zlib.h
nff_bsearch.o: /usr/include/zconf.h /usr/include/zlibdefs.h
nff_bsearch.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
nff_bsearch.o: /usr/include/bits/confname.h /usr/include/getopt.h
nff_bsearch.o: nff_planner_stats.hxx nff_theory_fragment.hxx
nff_bsearch.o: minisat/include/minisat/mtl/Sort.h hash_table.hxx
nff_bsearch.o: jenkins_12bit.hxx libdnnf/include/libdnnf/cnf2dnnf.hxx
nff_bsearch.o: nff_options.hxx nff_log.hxx utils.hxx /usr/include/sys/times.h
nff_bsearch.o: /usr/include/sys/resource.h /usr/include/bits/resource.h
nff_minisat.o: nff_search_node.hxx nff.hxx PDDL.hxx pddl_basic_types.hxx
nff_minisat.o: pddl_string_table.hxx pddl_fluent_set.hxx bitarray.hxx
nff_minisat.o: minisat/include/minisat/core/SolverTypes.h
nff_minisat.o: /usr/include/assert.h /usr/include/features.h
nff_minisat.o: /usr/include/bits/predefs.h /usr/include/sys/cdefs.h
nff_minisat.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
nff_minisat.o: /usr/include/gnu/stubs-32.h
nff_minisat.o: minisat/include/minisat/mtl/IntTypes.h /usr/include/stdint.h
nff_minisat.o: /usr/include/bits/wchar.h /usr/include/inttypes.h
nff_minisat.o: /usr/include/limits.h /usr/include/bits/posix1_lim.h
nff_minisat.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
nff_minisat.o: /usr/include/bits/posix2_lim.h
nff_minisat.o: minisat/include/minisat/mtl/Alg.h
nff_minisat.o: minisat/include/minisat/mtl/Vec.h
nff_minisat.o: minisat/include/minisat/mtl/XAlloc.h /usr/include/errno.h
nff_minisat.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
nff_minisat.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
nff_minisat.o: /usr/include/asm-generic/errno-base.h /usr/include/stdlib.h
nff_minisat.o: /usr/include/sys/types.h /usr/include/bits/types.h
nff_minisat.o: /usr/include/bits/typesizes.h /usr/include/time.h
nff_minisat.o: /usr/include/endian.h /usr/include/bits/endian.h
nff_minisat.o: /usr/include/bits/byteswap.h /usr/include/sys/select.h
nff_minisat.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
nff_minisat.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
nff_minisat.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
nff_minisat.o: minisat/include/minisat/mtl/Map.h
nff_minisat.o: minisat/include/minisat/mtl/Alloc.h nff_dijkstra.hxx
nff_minisat.o: libdnnf/include/libdnnf/dnnf_theory.hxx
nff_minisat.o: libdnnf/include/libdnnf/cnf_theory.hxx
nff_minisat.o: libdnnf/include/libdnnf/ext_math.hxx nff_belief.hxx
nff_minisat.o: nff_minisat.hxx minisat/include/minisat/core/Solver.h
nff_minisat.o: minisat/include/minisat/mtl/Heap.h
nff_minisat.o: minisat/include/minisat/utils/Options.h /usr/include/stdio.h
nff_minisat.o: /usr/include/libio.h /usr/include/_G_config.h
nff_minisat.o: /usr/include/wchar.h /usr/include/bits/stdio_lim.h
nff_minisat.o: /usr/include/bits/sys_errlist.h /usr/include/math.h
nff_minisat.o: /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h
nff_minisat.o: /usr/include/bits/huge_vall.h /usr/include/bits/inf.h
nff_minisat.o: /usr/include/bits/nan.h /usr/include/bits/mathdef.h
nff_minisat.o: /usr/include/bits/mathcalls.h /usr/include/string.h
nff_minisat.o: /usr/include/xlocale.h
nff_minisat.o: minisat/include/minisat/utils/ParseUtils.h /usr/include/zlib.h
nff_minisat.o: /usr/include/zconf.h /usr/include/zlibdefs.h
nff_minisat.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
nff_minisat.o: /usr/include/bits/confname.h /usr/include/getopt.h
nff_minisat.o: nff_planner_stats.hxx nff_theory_fragment.hxx
nff_minisat.o: minisat/include/minisat/mtl/Sort.h
nff_search_node.o: nff_search_node.hxx nff.hxx PDDL.hxx pddl_basic_types.hxx
nff_search_node.o: pddl_string_table.hxx pddl_fluent_set.hxx bitarray.hxx
nff_search_node.o: minisat/include/minisat/core/SolverTypes.h
nff_search_node.o: /usr/include/assert.h /usr/include/features.h
nff_search_node.o: /usr/include/bits/predefs.h /usr/include/sys/cdefs.h
nff_search_node.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
nff_search_node.o: /usr/include/gnu/stubs-32.h
nff_search_node.o: minisat/include/minisat/mtl/IntTypes.h
nff_search_node.o: /usr/include/stdint.h /usr/include/bits/wchar.h
nff_search_node.o: /usr/include/inttypes.h /usr/include/limits.h
nff_search_node.o: /usr/include/bits/posix1_lim.h
nff_search_node.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
nff_search_node.o: /usr/include/bits/posix2_lim.h
nff_search_node.o: minisat/include/minisat/mtl/Alg.h
nff_search_node.o: minisat/include/minisat/mtl/Vec.h
nff_search_node.o: minisat/include/minisat/mtl/XAlloc.h /usr/include/errno.h
nff_search_node.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
nff_search_node.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
nff_search_node.o: /usr/include/asm-generic/errno-base.h
nff_search_node.o: /usr/include/stdlib.h /usr/include/sys/types.h
nff_search_node.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
nff_search_node.o: /usr/include/time.h /usr/include/endian.h
nff_search_node.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
nff_search_node.o: /usr/include/sys/select.h /usr/include/bits/select.h
nff_search_node.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
nff_search_node.o: /usr/include/sys/sysmacros.h
nff_search_node.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
nff_search_node.o: minisat/include/minisat/mtl/Map.h
nff_search_node.o: minisat/include/minisat/mtl/Alloc.h nff_dijkstra.hxx
nff_search_node.o: libdnnf/include/libdnnf/dnnf_theory.hxx
nff_search_node.o: libdnnf/include/libdnnf/cnf_theory.hxx
nff_search_node.o: libdnnf/include/libdnnf/ext_math.hxx nff_belief.hxx
nff_search_node.o: nff_minisat.hxx minisat/include/minisat/core/Solver.h
nff_search_node.o: minisat/include/minisat/mtl/Heap.h
nff_search_node.o: minisat/include/minisat/utils/Options.h
nff_search_node.o: /usr/include/stdio.h /usr/include/libio.h
nff_search_node.o: /usr/include/_G_config.h /usr/include/wchar.h
nff_search_node.o: /usr/include/bits/stdio_lim.h
nff_search_node.o: /usr/include/bits/sys_errlist.h /usr/include/math.h
nff_search_node.o: /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h
nff_search_node.o: /usr/include/bits/huge_vall.h /usr/include/bits/inf.h
nff_search_node.o: /usr/include/bits/nan.h /usr/include/bits/mathdef.h
nff_search_node.o: /usr/include/bits/mathcalls.h /usr/include/string.h
nff_search_node.o: /usr/include/xlocale.h
nff_search_node.o: minisat/include/minisat/utils/ParseUtils.h
nff_search_node.o: /usr/include/zlib.h /usr/include/zconf.h
nff_search_node.o: /usr/include/zlibdefs.h /usr/include/unistd.h
nff_search_node.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
nff_search_node.o: /usr/include/getopt.h nff_planner_stats.hxx
nff_search_node.o: nff_theory_fragment.hxx minisat/include/minisat/mtl/Sort.h
nff_search_node.o: hash_table.hxx jenkins_12bit.hxx global_options.hxx
nff_log.o: nff_log.hxx
nff_theory_fragment.o: nff_theory_fragment.hxx
nff_theory_fragment.o: minisat/include/minisat/core/Solver.h
nff_theory_fragment.o: minisat/include/minisat/mtl/Vec.h
nff_theory_fragment.o: /usr/include/assert.h /usr/include/features.h
nff_theory_fragment.o: /usr/include/bits/predefs.h /usr/include/sys/cdefs.h
nff_theory_fragment.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
nff_theory_fragment.o: /usr/include/gnu/stubs-32.h
nff_theory_fragment.o: minisat/include/minisat/mtl/IntTypes.h
nff_theory_fragment.o: /usr/include/stdint.h /usr/include/bits/wchar.h
nff_theory_fragment.o: /usr/include/inttypes.h /usr/include/limits.h
nff_theory_fragment.o: /usr/include/bits/posix1_lim.h
nff_theory_fragment.o: /usr/include/bits/local_lim.h
nff_theory_fragment.o: /usr/include/linux/limits.h
nff_theory_fragment.o: /usr/include/bits/posix2_lim.h
nff_theory_fragment.o: minisat/include/minisat/mtl/XAlloc.h
nff_theory_fragment.o: /usr/include/errno.h /usr/include/bits/errno.h
nff_theory_fragment.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
nff_theory_fragment.o: /usr/include/asm-generic/errno.h
nff_theory_fragment.o: /usr/include/asm-generic/errno-base.h
nff_theory_fragment.o: /usr/include/stdlib.h /usr/include/sys/types.h
nff_theory_fragment.o: /usr/include/bits/types.h
nff_theory_fragment.o: /usr/include/bits/typesizes.h /usr/include/time.h
nff_theory_fragment.o: /usr/include/endian.h /usr/include/bits/endian.h
nff_theory_fragment.o: /usr/include/bits/byteswap.h /usr/include/sys/select.h
nff_theory_fragment.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
nff_theory_fragment.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
nff_theory_fragment.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
nff_theory_fragment.o: minisat/include/minisat/mtl/Heap.h
nff_theory_fragment.o: minisat/include/minisat/mtl/Alg.h
nff_theory_fragment.o: minisat/include/minisat/utils/Options.h
nff_theory_fragment.o: /usr/include/stdio.h /usr/include/libio.h
nff_theory_fragment.o: /usr/include/_G_config.h /usr/include/wchar.h
nff_theory_fragment.o: /usr/include/bits/stdio_lim.h
nff_theory_fragment.o: /usr/include/bits/sys_errlist.h /usr/include/math.h
nff_theory_fragment.o: /usr/include/bits/huge_val.h
nff_theory_fragment.o: /usr/include/bits/huge_valf.h
nff_theory_fragment.o: /usr/include/bits/huge_vall.h /usr/include/bits/inf.h
nff_theory_fragment.o: /usr/include/bits/nan.h /usr/include/bits/mathdef.h
nff_theory_fragment.o: /usr/include/bits/mathcalls.h /usr/include/string.h
nff_theory_fragment.o: /usr/include/xlocale.h
nff_theory_fragment.o: minisat/include/minisat/utils/ParseUtils.h
nff_theory_fragment.o: /usr/include/zlib.h /usr/include/zconf.h
nff_theory_fragment.o: /usr/include/zlibdefs.h /usr/include/unistd.h
nff_theory_fragment.o: /usr/include/bits/posix_opt.h
nff_theory_fragment.o: /usr/include/bits/confname.h /usr/include/getopt.h
nff_theory_fragment.o: minisat/include/minisat/core/SolverTypes.h
nff_theory_fragment.o: minisat/include/minisat/mtl/Map.h
nff_theory_fragment.o: minisat/include/minisat/mtl/Alloc.h
nff_theory_fragment.o: minisat/include/minisat/mtl/Sort.h PDDL.hxx
nff_theory_fragment.o: pddl_basic_types.hxx pddl_string_table.hxx
nff_theory_fragment.o: pddl_fluent_set.hxx bitarray.hxx
nff_planner_stats.o: nff_planner_stats.hxx
utils.o: utils.hxx /usr/include/sys/times.h /usr/include/features.h
utils.o: /usr/include/bits/predefs.h /usr/include/sys/cdefs.h
utils.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
utils.o: /usr/include/gnu/stubs-32.h /usr/include/time.h
utils.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
utils.o: /usr/include/sys/resource.h /usr/include/bits/resource.h
utils.o: /usr/include/bits/time.h /usr/include/unistd.h
utils.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
utils.o: /usr/include/getopt.h /usr/include/math.h
utils.o: /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h
utils.o: /usr/include/bits/huge_vall.h /usr/include/bits/inf.h
utils.o: /usr/include/bits/nan.h /usr/include/bits/mathdef.h
utils.o: /usr/include/bits/mathcalls.h
nff_mutexes.o: nff_mutexes.hxx PDDL.hxx pddl_basic_types.hxx
nff_mutexes.o: pddl_string_table.hxx pddl_fluent_set.hxx bitarray.hxx
nff_mutexes.o: minisat/include/minisat/core/SolverTypes.h
nff_mutexes.o: /usr/include/assert.h /usr/include/features.h
nff_mutexes.o: /usr/include/bits/predefs.h /usr/include/sys/cdefs.h
nff_mutexes.o: /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h
nff_mutexes.o: /usr/include/gnu/stubs-32.h
nff_mutexes.o: minisat/include/minisat/mtl/IntTypes.h /usr/include/stdint.h
nff_mutexes.o: /usr/include/bits/wchar.h /usr/include/inttypes.h
nff_mutexes.o: /usr/include/limits.h /usr/include/bits/posix1_lim.h
nff_mutexes.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
nff_mutexes.o: /usr/include/bits/posix2_lim.h
nff_mutexes.o: minisat/include/minisat/mtl/Alg.h
nff_mutexes.o: minisat/include/minisat/mtl/Vec.h
nff_mutexes.o: minisat/include/minisat/mtl/XAlloc.h /usr/include/errno.h
nff_mutexes.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
nff_mutexes.o: /usr/include/asm/errno.h /usr/include/asm-generic/errno.h
nff_mutexes.o: /usr/include/asm-generic/errno-base.h /usr/include/stdlib.h
nff_mutexes.o: /usr/include/sys/types.h /usr/include/bits/types.h
nff_mutexes.o: /usr/include/bits/typesizes.h /usr/include/time.h
nff_mutexes.o: /usr/include/endian.h /usr/include/bits/endian.h
nff_mutexes.o: /usr/include/bits/byteswap.h /usr/include/sys/select.h
nff_mutexes.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
nff_mutexes.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
nff_mutexes.o: /usr/include/bits/pthreadtypes.h /usr/include/alloca.h
nff_mutexes.o: minisat/include/minisat/mtl/Map.h
nff_mutexes.o: minisat/include/minisat/mtl/Alloc.h nff.hxx nff_belief.hxx
