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

#ifndef   	_MIA_PARSE_H_
#define   	_MIA_PARSE_H

/* Some useful pseudo-constants for parametrisation */

#ifndef MIA_ADVERTISE
#define MIA_ADVERTISE \
"-------------------------------------------------------------------\n" \
"         Morphological Spatial Pattern Analysis (MSPA)             \n" \
"            by {Pierre.Soille,Peter.Vogt}@ec.europa.eu             \n" \
"                   Version 2.3, February 2022                      \n" \
"        (c) European Commission, Joint Research Centre             \n" \
"                                                                   \n" \
" References:   \n" \
" P. Soille & P. Vogt, 2009: Morphological Segmentation of Binary   \n" \
" Patterns. Pattern Recognition Letters,                            \n" \
" DOI: https://doi.org/10.1016/j.patrec.2008.10.015                 \n" \
"                                                                   \n" \
" P. Vogt & K. Riitters, 2017: GuidosToolbox: universal digital     \n" \
" image object analysis. European Journal of Remote Sensing (TEJR), \n" \
" DOI: https://doi.org/10.1080/22797254.2017.1330650  \n" \
"\n" \
"Disclaimer: \n" \
"=========== \n" \
"1. The use of the software of this interface is made available\n" \
"   for non-commercial purposes. For commercial purposes please \n" \
"   contact: Peter.Vogt@ec.europa.eu \n" \
"2. The JRC, on behalf of the Commission does not accept any\n" \
"   liability whatsoever for any error, or for any loss or damage\n" \
"   arising from the use of the software of this interface.\n" \
"3. If the results of this interface are passed on to third parties\n" \
"   it has to be assured that the conditions of the disclaimer \n" \
"   are agreed and fulfilled.\n" \
"-------------------------------------------------------------------\n"
#endif

#ifndef NARGMAX
#define NARGMAX 100 /* Maximum number of arguments that can be passed */
#endif

#ifndef NARG
#define NARG NARGMAX /* see option files */
#endif

/* */

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef ERROR
#define ERROR -1
#endif
#ifndef OK
#define OK 0
#endif

#ifndef MAXCHARLENGTH
#define MAXCHARLENGTH  90
#endif
#ifndef MAXTEXTLENGTH
#define MAXTEXTLENGTH  500
#endif
#ifndef MAXNAMELENGTH
#define MAXNAMELENGTH MAXCHARLENGTH
#endif
#ifndef MAXLINELENGTH
#define MAXLINELENGTH  70
#endif
#ifndef MAXCHARBUFFLENGTH
#define MAXCHARBUFFLENGTH  255
#endif

#ifndef FLAG_VERBOSE
#define FLAG_VERBOSE FALSE
#endif

/**/

#ifndef _MIA_PTRPAR_UNION_
#define _MIA_PTRPAR_UNION_
typedef union mia_uptrpar{
  float *f;
  int *i;
  double *d;
  char *c;
  } UPtrPar;
#endif

#ifndef _MIA_PAR_UNION_
#define _MIA_PAR_UNION_
typedef union mia_upar{
  float f;
  int i;
  double d;
  char c;
  } UPar;
#endif


enum {IS_STRING=-1, IS_FLAG, IS_INT, IS_FLOAT, IS_DOUBLE};
/* type of variable:
 * 0: flag, 1: int; 2: float, +3 if flagged, -1: string
 */


/* Definition of the parse atom (for parse_argument routines) */
#ifndef _MIA_PARSEARG_STRUCT_
#define _MIA_PARSEARG_STRUCT_
typedef struct mia_parsearg{
  char  argname[MAXCHARLENGTH];    // name of the argument
  char  valname[MAXNAMELENGTH];    // name of its value
  char  explain[MAXTEXTLENGTH]; // explanation (as used by help)
  int   type;               // type of variable
  int   *flag;              // pointer to variable, if flag
  int   num;              // number of expected parameters
  UPtrPar var;               // pointer to variable
  UPar  minval;            // minimum value
  UPar  maxval;            // maximum value
} ParseArg;
#endif

#ifdef __cplusplus
extern "C"
{
#endif	/* __cplusplus */

  ParseArg* parse_alloc( int Narg );
  int parse_init( ParseArg* atom );
  int parse_free( ParseArg* atom );
  int parse_load( int Narg, int argc, char *argv[], ParseArg* atom );
  int parse_parameter( int argc, char *argv[], int (*parsers[])(), int nparse );

#ifdef __cplusplus
}
#endif		/* __cplusplus */

#endif 	    /* !_MIA_PARSE_H_ */
