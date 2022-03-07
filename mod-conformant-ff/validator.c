
/*
 * THIS SOURCE CODE IS SUPPLIED  ``AS IS'' WITHOUT WARRANTY OF ANY KIND, 
 * AND ITS AUTHOR AND THE JOURNAL OF ARTIFICIAL INTELLIGENCE RESEARCH 
 * (JAIR) AND JAIR'S PUBLISHERS AND DISTRIBUTORS, DISCLAIM ANY AND ALL 
 * WARRANTIES, INCLUDING BUT NOT LIMITED TO ANY IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND
 * ANY WARRANTIES OR NON INFRINGEMENT.  THE USER ASSUMES ALL LIABILITY AND
 * RESPONSIBILITY FOR USE OF THIS SOURCE CODE, AND NEITHER THE AUTHOR NOR
 * JAIR, NOR JAIR'S PUBLISHERS AND DISTRIBUTORS, WILL BE LIABLE FOR 
 * DAMAGES OF ANY KIND RESULTING FROM ITS USE.  Without limiting the 
 * generality of the foregoing, neither the author, nor JAIR, nor JAIR's
 * publishers and distributors, warrant that the Source Code will be 
 * error-free, will operate without interruption, or will meet the needs 
 * of the user.
 */








/*********************************************************************
 *
 * File: validator.c
 *
 * Description: implementation of routines to validate a plan
 *
 *              ADL version
 *
 *
 * Author: Alexandre Albore 2010
 *
 *********************************************************************/ 

#include "ff.h"
#include "state_transitions.h"
#include "memory.h"
#include "search.h"


int vplan[MAX_PLAN_LENGTH], vplan_length;

/* Takes the vplan and applies it to Init.
 * It can be taken into account to use a simpler version of source_to_dest.
 * Get vplan_length first.
 * Don't have to verify reaching the goal, just apply till the end.
 **/
Bool validate(EhcNode *node0)
{
  State S, S_;
  EhcNode *current_node;
  EhcNode *n;
  int i;


  make_state( &S, gnum_ft_conn ); 
  make_state( &S_, gnum_ft_conn ); 

  current_node = node0;
 
    printf("\nPlan length: %d\n", vplan_length);
    for  ( i = 0; i < vplan_length; i++ )  {
      printf("\n");
      print_op_name( vplan[i] );

    }
      printf("\n");
      
    for  ( i = 0; i < vplan_length; i++ ) {

        print_op_name( vplan[i] );
	  printf(" applied - %d\n", vplan[i]); 

 
  /* Apply next action into current_node and copy the result into S
     and into a new node */
    if ( !result_to_dest( &S, current_node, NULL, vplan[i] ) ) {
      print_op_name( vplan[i] );      
      printf("\naction not applicable!");
	return FALSE;
      }

    n = new_EhcNode();
    n->op = vplan[i];
    copy_source_to_dest( &n->S , &S);  /* new state   source_to_dest( &n->state, &S );   */

    n->father = current_node;

    current_node = n;


    /* Update the node with plan of length i, operator vplan[i], and state S */


 }


  /* NB: Check and maybe rewrite result_to_dest in state_transition.c */


  return TRUE;

}


void read_input_plan(char* vplan_file_name)
{

  char line[128];
  char act[128];
  vplan_length = 0;

  /* open file */
  FILE *fp;
   if( ( fp = fopen( vplan_file_name, "r") ) == NULL ) {
    printf("\nff: can't find plan file: %s\n\n", vplan_file_name );
    /*    perror(tmp);*/
    exit ( 1 );
  }
  /* read input */

   int i,j, len;

   while ( fgets ( line, sizeof line, fp ) != NULL ) /* read a line */
     {
       /* printf("Reading: ");
       fputs ( line, stdout ); 
       printf("\n"); */
       len = strlen( line );
           
       for ( i = 0; i < gnum_op_conn; i++ ) {
	 Action *a = gop_conn[i].action;

	 if ( !a->norm_operator &&
	      !a->pseudo_action ) {
	   strcpy (act, "REACH-GOAL");
	 } else {
	   strcpy(act, a->name ); 
	   for ( j = 0; j < a->num_name_vars; j++ ) {
	     strcat(act, " "); 
	     strcat(act, gconstants[a->name_inst_table[j]]);
	   }}

	 if (strncmp (act, &line[1], len-3) == 0)
	   {
	     vplan[vplan_length++] = i;
	     break;
	     }
       }
       if ( vplan_length == MAX_PLAN_LENGTH ) {
	 printf("\nincrease MAX_PLAN_LENGTH! currently %d\n\n",
		MAX_PLAN_LENGTH);
	 exit( 1 );
       }
     }

   fclose(fp);
   printf("closing input file.........%d\n", vplan_length);

     }
