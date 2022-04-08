
/***********************************************************************
Author(s): Pierre Soille and Jacopo Grazzini
Generic functions enabling parsing parameters to programs

Copyright (C) 2000-2022 European Union (Joint Research Centre)

This file is part of GTB/GWB.
This file requires miallib.

miallib is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

miallib is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with miallib.  If not, see <https://www.gnu.org/licenses/>.
***********************************************************************/

/** @file
 *  Morphological Segmentation of Binary Patterns \cite soille-vogt2009
 *  https://doi.org/10.1016/j.patrec.2008.10.015
 *  @author Pierre Soille and Peter Vogt
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h> 
#include <string.h>

#include "mia_parse.h"

/* Some useful internal pseudo-constants */

#ifndef NELEMS
#define NELEMS(t) (sizeof(t) / sizeof *(t))
#endif

#ifndef Free
#define Free(x)   {if((x)!=NULL) {free(x); x=NULL;}}
#endif

/* */

extern int FlagVerbose;
 
/***************************************************************************/
ParseArg* parse_alloc( int Narg ) {
  /***************************************************************************/ 
  ParseArg* atom;
  
  if((atom=(ParseArg*)calloc(Narg,sizeof(ParseArg))) == NULL)
    return NULL;
  return atom;
} // end of parse_alloc


/***************************************************************************/
int parse_init( ParseArg* atom ) {
  /***************************************************************************/
  int in=0;
  
  /* These flags are always displayed */
  
  //   Argument flag VER. Type 0: Flag
  strcpy(atom[in].argname,"-v");
  sprintf(atom[in].explain," %s : %s\n",atom[in].argname,
	  "Flag for verbose mode.");
  atom[in].type=0;
  atom[in].flag = &FlagVerbose; // the only global variable required here
  atom[in].num=1;   
  in++;
 
  /* These others also don't need pointers */
  //   Argument flag USAGE. Type 0: Flag
  strcpy(atom[in].argname,"-usage");
  sprintf(atom[in].explain," %s : %s\n",atom[in].argname,
	  "Flag for displaying progam usage.");
  in++;
  
  //   Argument flag HELP. Type 0: Flag
  strcpy(atom[in].argname,"-help");
  sprintf(atom[in].explain," %s : %s\n",atom[in].argname,
	  "Flag for help.");
  in++;
  
  return in;
} // end of parse_init


/***************************************************************************/
int parse_free( ParseArg* atom ) {
  /***************************************************************************/ 
  Free(atom);
  return OK;
} // end of parse_free


/***************************************************************************/
int parse_load( int Narg, int argc, char *argv[], ParseArg* atom ) {
  /***************************************************************************/
  int flagv=1;
  int lar, arglen;
  int in, i, cur;
  char usage[MAXTEXTLENGTH];
  int l;


  /*                   Parsing loop                         */
  for( lar=1,flagv=1; (lar<argc)&&(flagv==1); lar++ ) {

   flagv = 0;
    for( in=0; (in<Narg)&&(flagv==0); in++ )
      if(!strcmp(argv[lar],atom[in].argname))
	flagv = 1; 
      else if( !strcmp(argv[lar],"-h") || !strcmp(argv[lar],"-help") ) 
	flagv = 2;
      else if( !strcmp(argv[lar],"-usage") ) 
	flagv = 3;
    /* the test (flagv==0) ensures us we leave the loop 'for( in=0;...'
     * when both strings argv[lar] and atom[in].argname match */
    
    /* Error: none of the known flags match with the entered one:
     * leave the loop 'for( lar=1,flagv=1; ...' */
    if(in>=Narg && flagv==0) break;
    
    in--; /* the right index is in-1 */
    
    /* If a flag has been recognized, process... */
    if(flagv == 1) {
      for ( i=0; i<atom[in].num; i++ ) { 
	/* check all parameters (atom[in].num in total) following the flag */
	
	if (atom[in].type == IS_STRING) { /* the parameter is of type CHAR list */
	  lar++;
	  if((argv[lar]==NULL) || (argv[lar][0]=='-')) {
	    fprintf(stderr,"\nMissing argument for option %s",argv[lar-i-1]);
	    exit(-1);
	  }	  
	  strcpy(atom[in+i].var.c,argv[lar]); // atom[in+i].var.c = argv[lar];
	  
	} else if(atom[in].type == IS_INT)  { /* the parameter is of type INT */
	  lar++; 
	  sscanf(argv[lar],"%d",atom[in+i].var.i);
	  if((*(atom[in+i].var.i)<atom[in+i].minval.i) ||
	     (*(atom[in+i].var.i)>atom[in+i].maxval.i)) {
	    fprintf(stderr,"\nValue out of range (%d - %d) for argument %s\n",
		       atom[in+i].minval.i,atom[in+i].maxval.i,atom[in].argname);
	    flagv=-1;
	    break;
	  }
	  
	}  else if(atom[in].type == IS_DOUBLE)  { /* the parameter is of type DOUBLE */	  lar++;
	  sscanf(argv[lar],"%lf",atom[in+i].var.d);
	  if((*(atom[in+i].var.d)<atom[in+i].minval.d) ||
	     (*(atom[in+i].var.d)>atom[in+i].maxval.d))	{
	    fprintf(stderr,"\nValue out of range (%g - %g) for argument %s\n",
		       atom[in+i].minval.d,atom[in+i].maxval.d,atom[in].argname);
	    flagv=-1;
	    break;
	  }
	  
	}
      }
    if(atom[in].type == IS_FLAG) /* : the parameter is a FLAG */
      *(atom[in].flag) = TRUE; 	/* otherwise FALSE */
    }
  }	// end of parsing loop 'for( lar=1,flagv=1; ...'

  if (argc==1) // No arg then usage
    flagv=2;

  if(flagv == 0)  {
    fprintf(stderr,"\nUnknown option %s\n",argv[lar]);
    exit(ERROR);
  } else if(flagv == -1) {
    fprintf(stderr,"\nParameters for option %s out of the range of possible values",
	    argv[lar-i-2]);
    exit(ERROR);
  }
  
  if(flagv != 1) printf(MIA_ADVERTISE);

  if(/*argc==1 ||*/flagv != 1) {
    printf("\nUsage : %s \n---------------- \n",argv[0]);
    l=0;
    for( in=0; in<Narg; in++ ) {
      if(l > MAXLINELENGTH) {printf("\n"); l=0;} // jump a line
      if(strlen(atom[in].argname)){
	if(atom[in].type == 0) {l += printf(" [%s]",atom[in].argname);}
	else l += printf(" [%s <%s>]",atom[in].argname,atom[in].valname);
	for( i=0,cur=in; i<atom[cur].num-1; i++ ) 
	  in++; // skip some indices
      }
    }
  }
  
  if(flagv == 2) {
    printf("\nParameters :\n------------\n");
    for(in=0;in<Narg;in++) printf("%s",atom[in].explain);
  }

  if(flagv != 1) exit(OK);

  return OK;
} // end of parse_init


/***************************************************************************/
int parse_parameter( int argc, char *argv[], int (*parsers[])(), int nparse ) {
/***************************************************************************/

  ParseArg* atom;
  int narg;  //  int nparse=NELEMS(parsers);
  
  atom = parse_alloc( NARGMAX ); // parameters are limited to a "reasonable" number
  
  narg = parse_init( atom );
  
  while(nparse-- >= 0)
    narg += (parsers[nparse])( narg, atom ); // function defining/parsing parameters
  
  parse_load( narg, argc, argv, atom );
  
  parse_free( atom );
  
  return OK;
} // end of parse_parameter
