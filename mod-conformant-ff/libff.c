#include "libff.h"

#include "memory.h"
#include "output.h"

#include "parse.h"

#include "inst_pre.h"
#include "inst_easy.h"
#include "inst_hard.h"
#include "inst_final.h"

/*
 *  ----------------------------- GLOBAL VARIABLES ----------------------------
 */
/*******************
 * GENERAL HELPERS *
 *******************/
/* used to time the different stages of the planner
 */
float gtempl_time = 0, greach_time = 0, grelev_time = 0, gconn_time = 0, gmem_time = 0;
float gsearch_time = 0, geval_time = 0, gcnf_time = 0, genc_time = 0, gsat_time = 0;
float grs_time = 0, grs_sat_time = 0, gsc_time = 0, gss_time = 0;
float gr_sat_time = 0, grp_sat_time = 0, gr_cnf_time = 0, gr_enc_time = 0, gmembership_time = 0;
int gsat_calls = 0, gcnfs = 0, grs_sat_calls = 0, gss_sat_calls = 0;
int gr_sat_calls = 0, grp_sat_calls = 0, gsc_sat_calls = 0, grs_comps = 0;
int grs_hits = 0, gss_hits = 0, grs_conf_comps = 0, gdp_calls = 0, gup_calls = 0;


/* the command line inputs
 */
struct _command_line gcmd_line;

/* number of states that got heuristically evaluated
 */
int gevaluated_states = 0;

/* maximal depth of breadth first search
 */
int gmax_search_depth = 0;

/* CNF statistic
 */
float *gsum_k_clauses = 0, gsum_clauses = 0;




/***********
 * PARSING *
 ***********/
/* used for pddl parsing, flex only allows global variables
 */
int gbracket_count;
char *gproblem_name;

/* The current input line number
 */
int lineno = 1;

/* The current input filename
 */
char *gact_filename;

/* The pddl domain name
 */
char *gdomain_name = NULL;

/* loaded, uninstantiated operators
 */
PlOperator *gloaded_ops = NULL;

/* stores initials as fact_list 
 */
PlNode *gorig_initial_facts = NULL;

/* stores initial ors as an array of OR lists
 */
PlNode_pointer *gorig_initial_ors = NULL;
int gnum_orig_initial_ors = 0;

/* stores initial oneofs as an array of ONEOF lists
 */
PlNode_pointer *gorig_initial_oneofs = NULL;
int gnum_orig_initial_oneofs = 0;

/* not yet preprocessed goal facts
 */
PlNode *gorig_goal_facts = NULL;

/* the types, as defined in the domain file
 */
TypedList *gparse_types = NULL;

/* the constants, as defined in domain file
 */
TypedList *gparse_constants = NULL;

/* the predicates and their arg types, as defined in the domain file
 */
TypedListList *gparse_predicates = NULL;

/* the objects, declared in the problem file
 */
TypedList *gparse_objects = NULL;

/* connection to instantiation ( except ops, goal, initial )
 */

/* all typed objects 
 */
FactList *gorig_constant_list = NULL;

/* the predicates and their types
 */
FactList *gpredicates_and_types = NULL;

/* the functions and their types
 */
FactList *gfunctions_and_types = NULL;

/*****************
 * INSTANTIATING *
 *****************/
/* global arrays of constant names,
 *               type names (with their constants),
 *               predicate names,
 *               predicate aritys,
 *               defined types of predicate args
 */
Token gconstants[MAX_CONSTANTS];
int gnum_constants = 0;
Token gtype_names[MAX_TYPES];
int gtype_consts[MAX_TYPES][MAX_TYPE];
Bool gis_member[MAX_CONSTANTS][MAX_TYPES];
int gtype_size[MAX_TYPES];
int gnum_types = 0;
Token gpredicates[MAX_PREDICATES];
int garity[MAX_PREDICATES];
int gpredicates_args_type[MAX_PREDICATES][MAX_ARITY];
int gnum_predicates = 0;
Bool gnot_fact[MAX_PREDICATES];


/* the domain in integer (Fact) representation
 */
Operator_pointer goperators[MAX_OPERATORS];
int gnum_operators = 0;
Fact *gfull_initial;
int gnum_full_initial = 0;

Fact *gfull_unknown_initial = NULL;
int gnum_full_unknown_initial = 0;
WffNode_pointer *gfull_or_initial = NULL;
int gnum_full_or_initial = 0;
WffNode_pointer *gfull_oneof_initial = NULL;
int gnum_full_oneof_initial = 0;
/* FluentValue *gfull_fluents_initial;
   int gnum_full_fluents_initial = 0; */
WffNode *ggoal = NULL;


/* stores inertia - information: is any occurence of the predicate
 * added / deleted in the uninstantiated ops?
 * is any occurence of the predicate unknown?
 */
Bool gis_added[MAX_PREDICATES];
Bool gis_deleted[MAX_PREDICATES];
Bool gis_unknown[MAX_PREDICATES];

/* splitted initial state:
 * initial non static facts,
 * initial static facts, divided into predicates
 * (will be two dimensional array, allocated directly before need)
 *
 * the same mirrored for unknown facts -- "known negatives" is transferred
 * here to "known positives and unknowns"; seems more adequate for later 
 * purposes, giving access to unknowns directly. note that unknowns are
 * not assumed static.
 */
Facts *ginitial = NULL;
int gnum_initial = 0;
Facts *gunknown_initial = NULL;
int gnum_unknown_initial = 0;
Fact **ginitial_predicate;
int *gnum_initial_predicate;
Fact **gunknown_initial_predicate;
int *gnum_unknown_initial_predicate;

/* this here stores dependencies between initial variables:
 * when translating negations of an unkwown literal we need
 * to remember that the translation, while unkown, will
 * always have the respective inverse value.
 * we remember the fts for whic this holds.
 */
Facts *ginitial_ft_equivalence_A = NULL;
Facts *ginitial_ft_equivalence_notA = NULL;
int gnum_initial_ft_equivalence = 0;


/* the type numbers corresponding to any unary inertia
 */
int gtype_to_predicate[MAX_PREDICATES];
int gpredicate_to_type[MAX_TYPES];

/* (ordered) numbers of types that new type is intersection of
 */
TypeArray gintersected_types[MAX_TYPES];
int gnum_intersected_types[MAX_TYPES];

/* stores which predicate is a translation of which other one.
 */
int gtranslated_predicate_to[MAX_PREDICATES];

/* splitted domain: hard n easy ops
 */
Operator_pointer *ghard_operators;
int gnum_hard_operators;
NormOperator_pointer *geasy_operators;
int gnum_easy_operators;

/* so called Templates for easy ops: possible inertia constrained
 * instantiation constants
 */
EasyTemplate *geasy_templates;
int gnum_easy_templates;

/* first step for hard ops: create mixed operators, with conjunctive
 * precondition and arbitrary effects
 */
MixedOperator *ghard_mixed_operators;
int gnum_hard_mixed_operators;

/* hard ''templates'' : pseudo actions
 */
PseudoAction_pointer *ghard_templates;
int gnum_hard_templates;

/* store the final "relevant facts"
 */
Fact grelevant_facts[MAX_RELEVANT_FACTS];
int gnum_relevant_facts = 0;
int gnum_pp_facts = 0;

/* the final actions and problem representation
 */
Action *gactions = NULL;
int gnum_actions;
State ginitial_state;
State ggoal_state;
/* to access initially valid implications: the translated facts.
 */
int *ginitial_equivalence_A;
int *ginitial_equivalence_notA;
int gnum_initial_equivalence;
/* to know how much space we need for unknown conds in states
 */
int gmax_E;
/* the initial OR constraints in final coding
 */
int **ginitial_or;
int *ginitial_or_length;
int gnum_initial_or;
/* the ors corresponding to the oneofs 
*/
int *goneof_or_corr;


/**********************
 * CONNECTIVITY GRAPH *
 **********************/
/* one ops (actions) array ...
 */
OpConn *gop_conn;
int gnum_op_conn;

/* one effects array ...
 */
EfConn *gef_conn;
int gnum_ef_conn;

/* one facts array.
 */
FtConn *gft_conn;
int gnum_ft_conn;


/* max #conds. for max clauses computation.
 */
int gmax_C;


/* max U: all initial Us plus facts that are 
 * added / deleted by a conditional effect with poss_U conds.
 * (important for various memory allocations)
 */
int gmax_U;
int gmax_CNFU;

/* we get these max #s of clauses and lits.
 */
int gmax_clauses;
int gmax_rs_clauses; 
int gmax_literals;






/*******************
 * SEARCHING NEEDS *
 *******************/
/* applicable actions
 */
int *gA;
int gnum_A;

/* communication from extract 1.P. to search engine:
 * 1P action choice
 */
int *gH;
int gnum_H;

/* always store (current) serial plan
 */
int gplan_ops[MAX_PLAN_LENGTH];
int gnum_plan_ops = 0;

/* stores the states that the current plan goes through
 * ( for knowing where new agenda entry starts from )
 */
State gplan_states[MAX_PLAN_LENGTH + 1];


/* does this contain conditional effects?
 * (if it does then the state hashing has to be made more
 *  cautiously)
 * AA: maintained for compatibility with metric-FF and probe
 */
Bool gconditional_effects = TRUE;


/* Alex: probably don't need this */
/* the clauses to be communicated to the SAT solver for
 * determining inferred literals.
 */
TimedLiteral **gclauses;
int *gclause_length;
int gnum_fixed_clauses;
int gnum_clauses;
int gfixed_endtime;

/* array; maps ft / time pair to its number in CNF encoding.
 */
int **gcodes;
int gnum_fixed_c;


/* inverse mapping, to undo changes in table.
 */
int *gcf, *gct, gnum_c;

 

/* statistics: count nr. of times the "disjunction minimisation" actually
 * minimised something
 */
int gremoved_lits;



/* stores the current DP decisions including unit propagations.
 *
 * is for DP given in state_transitions.c!!!!!
 *
 * have to make this global as it's also accessed from repeated states --
 * when checking stagnation. I know it's ugly...
 */
int *gdecision_stack;
int gnum_decision_stack;



/* for each possible ft code, a pointer to connected dynamic list
 * of member elements, ie the clauses in which it participates,
 * positive and negative.
 *
 * used in state_transitions DP solver. global as accessed from search.c
 * in reset between ehc and bfs switch.
 */
MemberList_pointer *gpos_c_in_clause_start;
MemberList_pointer *gpos_c_in_clause_fixed;/* before here, list corresp. to fixed CNF */
MemberList_pointer *gpos_c_in_clause_end;/* before here, members of current list */
MemberList_pointer *gneg_c_in_clause_start;
MemberList_pointer *gneg_c_in_clause_fixed;
MemberList_pointer *gneg_c_in_clause_end;



/* automatically checked Bool saying if sufficient criteria for
 * "more facts are always better" holds.
 */
Bool gdomination_valid;


/*
 *  ----------------------------- HEADERS FOR PARSING ----------------------------
 * ( fns defined in the scan-* files )
 */
void get_fct_file_name( char *filename );
void load_ops_file( char *filename );
void load_fct_file( char *filename );

/*
 *  ----------------------------- MAIN ROUTINE ----------------------------
 */
struct tms lstart, lend;
Bool lfound_plan;

/* Implementation */

int	FF_parse_problem( const char* domain_file, const char* instance_file )
{
	
  /*	gcmd_line.optimize = TRUE;  */

	char ops_file[MAX_LENGTH];
	char fct_file[MAX_LENGTH];
	
	snprintf( ops_file, MAX_LENGTH, "%s", domain_file );
	snprintf( fct_file, MAX_LENGTH, "%s", instance_file );

	load_ops_file( ops_file );
	load_fct_file( fct_file );

	/* This is needed to get all types.
	*/
	build_orig_constant_list();

	/* last step of parsing: see if it's an ADL domain!
	*/
	if ( !make_adl_domain() ) 
	{
		printf("\nlibff: This is not an ADL problem!");
		printf("\n    can't be handled by this version.\n\n");
		return 1 ;
	}
	return 0;
}

int	FF_instantiate_problem()
{

	/**************************
	* first do PREPROCESSING * 
	**************************/
	
	/* start by collecting all strings and thereby encoding 
	* the domain in integers.
	*/
	encode_domain_in_integers();
	
	/* inertia preprocessing, first step:
	*   - collect inertia information
	*   - split initial state into
	*        - arrays for individual predicates
	*        - arrays for all static relations
	*        - array containing non - static relations
	*/
	do_inertia_preprocessing_step_1();
		
	/* normalize all PL1 formulae in domain description:
	* (goal, preconds and effect conditions)
	*   - simplify formula
	*   - expand quantifiers
	*   - NOTs down
	*/
	normalize_all_wffs();
	
	/* translate negative preconds: introduce symmetric new predicate
	* NOT-p(..) (e.g., not-in(?ob) in briefcaseworld)
	*/
	translate_negative_preconds();
	
	/* split domain in easy (disjunction of conjunctive preconds)
	* and hard (non DNF preconds) part, to apply 
	* different instantiation algorithms
	*/
	split_domain();
	
	/***********************************************
	* PREPROCESSING FINISHED                      *
	*                                             *
	* NOW MULTIPLY PARAMETERS IN EFFECTIVE MANNER *
	***********************************************/
	
	build_easy_action_templates();
	build_hard_action_templates();
	
	/* perform reachability analysis in terms of relaxed 
	* fixpoint
	*/
	perform_reachability_analysis();
	
	/* collect the relevant facts and build final domain
	* and problem representations.
	*/
	collect_relevant_facts();
	
	/* now build globally accessable connectivity graph
	*/
	build_connectivity_graph();

	return 0;
}
