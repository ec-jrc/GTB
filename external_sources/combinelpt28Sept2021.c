/* ************************************************************************
Author(s): Kurt Riitters

 combinelpt.c is in the public domain and distributed in the hope that it will be useful.

Although this program has been compiled and executed successfully on a computer system at the US Forest Service, no warranty expressed or implied is made by the US Forest Service regarding the use of the program on any other system, nor does the act of distribution constitute any such warranty. There is no documentation outside the program itself, and no technical support is provided by the author or his employer. This program may be updated without notification. The US Forest Service shall not be liable for any activity involving this program, its fitness for a particular purpose, its use, or analyses results.

  COMBINELPT.C   read N sets of (Pa, Pn, Pd) triplet maps (bsq format), calculate
	average Pa, average Pn, average Pd over all N triplets, apply the 
	LPT classifier from spatcon to the average values, and write LPT output bsq.
   * Version 1. July 2017
   * Version 2. July 2018. Added 103-class lpt classifier from Spatcon.
   	Added second line to parameter file to indicate 1=19class version or 2=103 class version
************************************************************************ */
/** @file
 *  Peter Vogt & Kurt Riitters (2017) GuidosToolbox: universal digital image object analysis, European Journal of Remote Sensing, 50:1, 352-361,
 *  https://doi.org/10.1080/22797254.2017.1330650
 *  @author Kurt Riitters
 */

/* compile flags on linux gcc: 
 * gcc -std=c99 -m64 -O2 -Wall combinelpt.c -o combinelpt
 * 
 * Usage: ./combinelpt parfile hdrfile outfile file1 file2 file3 file4 ... file(N*3)
	Assumes the set of files (file1 through file(N*3)) are sets of triplets in the order (Pa, Pn, Pd) repeating
	the triplet for N triplets, to give file1 through fileN*3.
	File1 through fileN*3 are assumed to be 8-bit files produced by spatcon.
	All of those files must be the same number of rows and columns.
	All of the filenames can be anything, but they must be full filenames including extensions.
	The program writes an 8bit "bsq" file called <outfile>.bsq.
	The output file has an lpt code of 1-19 as usual

	There is no limit on N. N must be at least 1.

	When N is 1, the 'average' is in principle the same map one would get by using spatcon to calculate lpt's.
	However, there will be some differences for N=1 because the Pa, Pd, Pn in this program are less precise than they are in spatcon
		(because this program reads byte values of Pa,Pd,Pn while spatcon uses actual percentages)
 *
 * The parameter file is a text file containing two lines with one number on each line:
 * Line 1. XXX where XXX is the number of triplets
 * Line 2. 1 or 2, where 1 indicates 19-class lpt, and 2 indicates 103-class lpt
 *
 * The header file is a text file containing two lines, with a dummy word followed by a number on each line:
 * nrows XXXX  (where XXX is a number)
 * ncols XXXX  (nrows must be first line, ncols second line)
 * 
 * Other notes:
	The output is not 'masked' to data area, and that must be done by the user. The 'average lpt' values outside the data
	will seem to be correct (in the range 1-19) but they are not correct. Many of them will be '15' because of the way the classifier works.
	The reason this happens is that spatcon does not do any 'masking' of nodata area, and as a result produces values for pa,pd,pn outside of the data area.

	The names "Pa", "Pn" and "Pd" are used here. The cross walk with GTB is that a, n, and d refer to byte values 1,2,3 in a spatcon run of LM.

*/
#include  <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(__APPLE__)
#include <malloc.h>
#endif
/* prototypes*/
unsigned char LPT_Classifier(long int, float, float, float);

/* the following structure handles the variable number of input maps */
struct infileinfo {
	FILE *infileptr;
	char infilename[100];
};
struct infileinfo *infiles; /* pointer to the first infile structure */

 
int main(int argc, char **argv)
{
	static FILE *outfile, *hdrfile, *parfile;
	static char filename_out[100], filename_hdr[100], filename_par[100], dummy[30];
	static long int ntriplets, tripnum, numfiles, row, col, nrows, ncols, filenum, lptmodel;
	static float sum_a, sum_d, sum_n, avg_a, avg_d, avg_n;
	static unsigned char outval, *outrow, *rowdata; /* outrow is one-dim, and rowdata is 2-dim */
	
	setbuf(stdout, NULL);
/* open some files */
	strcpy(filename_par,argv[1]);
	strcpy(filename_hdr,argv[2]);
	strcpy(filename_out,argv[3]);
	if( (parfile = fopen(filename_par, "r") ) == NULL) {
		printf("\nError opening %s\n", filename_par);
		exit(8);
	}
	if( (hdrfile = fopen(filename_hdr, "r") ) == NULL) {
		printf("\n:Error opening %s\n", filename_hdr);
		exit(8);
	}
	if( (outfile = fopen(filename_out, "wb") ) == NULL) {
		printf("\nError opening %s\n", filename_out);
		exit(8);
	}

/* Read the header and parameter files*/
	fscanf(hdrfile,"%s %ld", dummy, &nrows);
	fscanf(hdrfile,"%s %ld", dummy, &ncols);
	fscanf(parfile,"%ld", &ntriplets);
	fscanf(parfile,"%ld", &lptmodel);  /* 1 is 19class and 2 is 103class*/

/* allocate memory to hold infile name and pointer structures*/
	numfiles = ntriplets*3;
	infiles = (struct infileinfo *)calloc(numfiles, sizeof(struct infileinfo)); 
	if(infiles == NULL) {printf("\nMalloc failed for infileinfo\n");exit(8);} 
/* open the input files containing triplet data */
	for(filenum = 0; filenum < numfiles; filenum++) {
		strcpy( (*(infiles+filenum)).infilename, argv[4+filenum]);
		if ( ((*(infiles+filenum)).infileptr = fopen( (*(infiles+filenum)).infilename, "rb") ) == NULL) {
			printf("\nError opening %s\n", (*(infiles+filenum)).infilename);
			exit(8);
		}
	}
/* Allocate memory for input rows (one for each input file) and output row*/
	if( (rowdata = (unsigned char *)calloc( (numfiles*ncols), sizeof(unsigned char) ) ) == NULL ) {
		printf("\nmalloc failed for rowdata.\n");
		exit(8);
	}
	if( (outrow = (unsigned char *)calloc( ncols, sizeof(unsigned char) ) ) == NULL ) {
		printf("\nmalloc failed for outrow.\n");
		exit(8);
	}
/* loop through the rows. populate the rowdata matrix, calculate the averages, call the classifier, populate & write output row */
	for (row=0; row < nrows; row++) {
		/* read a row of data from each file*/
		for(filenum=0; filenum < numfiles; filenum++) {
			if(fread( (rowdata+(filenum*ncols)), 1, ncols, (*(infiles+filenum)).infileptr) != ncols ) {
				printf("\n: Read error on file %ld.\n", filenum);
				exit(8);
			}
		}
		/* initialize the sum across scales */
		/* calculate the average pa,pd,pn for each pixel, call the classifier, and populate the output row */
// OMP
	omp_set_num_threads(omp_get_max_threads());
#pragma omp parallel  for  	 private (col, sum_a, sum_d, sum_n, avg_a, avg_d, avg_n, outval, tripnum)
		for(col=0; col < ncols; col++) {
			sum_a = 0.;
			sum_d = 0.;
			sum_n = 0.;
			/* accumulate sums across scales */
			for(tripnum = 0; tripnum < ntriplets; tripnum++){
				sum_a = sum_a + (*(rowdata + (tripnum*3*ncols) + col) );
				sum_n = sum_n + (*(rowdata + (tripnum*3*ncols) + ncols + col) );
				sum_d = sum_d + (*(rowdata + (tripnum*3*ncols) + ncols + ncols + col) );
			}
			/*calculate averages, convert from byte representation to 0,1 representation */
			avg_a = ((sum_a / (1.*ntriplets)) - 1.)/254.;
			avg_d = ((sum_d / (1.*ntriplets)) - 1.)/254.;
			avg_n = ((sum_n / (1.*ntriplets)) - 1.)/254.;
			/* classify it */
			outval = LPT_Classifier(lptmodel, avg_a, avg_n, avg_d);
			(*(outrow + col)) = outval;

		}/* end loop over cols*/
		/* output the row */
		if(fwrite(outrow, 1, ncols, outfile) != ncols ) {
			printf("\nerror writing output file.\n");
			exit(8);
		}
	} /* end loop over rows*/	
	/* Exit nicely */
	fclose(parfile);
	fclose(hdrfile);
	for(filenum = 0; filenum < numfiles; filenum++) {
		fclose( (*(infiles+filenum)).infileptr);
	}
	free(infiles);
	free(rowdata);
	free(outrow);
	printf("combineLPT:Normal Finish.\n");
	exit(0);
}

unsigned char LPT_Classifier(long int lpt_model, float p_agr, float p_for, float p_dev)
{
	static unsigned char map_val;
if (lpt_model == 1) {
	while (1) {  /* kludge to permit use of break statements */
		if(p_for >= 0.60) {    /* it's an F matrix */
			if(p_for == 1.0) { /* it's all 'forest' */
				map_val = 17; /* F* */
				break;
			}
			if(p_dev > p_agr) {  /* sort the other two */
				if(p_agr >= 0.10) {
					map_val = 12;      /* Fad, note this is the same map val as Fad below. Earlier code distinguished between Fda and Fad, hence mention of sorting the other two*/
					break;
				}
				if(p_dev >= 0.10) {
					map_val = 9;      /* Fd   */
					break;
				}
			} else { /* still F, but agr > dev */
				if(p_dev >= 0.10) {
					map_val = 12;      /* Fad */
					break;
				}
				if(p_agr >= 0.10) {
					map_val = 8;      /* Fa   */
					break;
				}
			}
			map_val = 3;            /* F */
			break;
		} /* if p_for > .6 */
		if(p_agr >= 0.60) {    /* it's an A matrix */
			if(p_agr == 1.0) { /*it's all agriculture */
				map_val = 18;  /* A* */
				break;
			}
			if(p_dev > p_for) {  /* sort the other two */
				if(p_for >= 0.10) {
					map_val = 10;      /* Adf */
					break;
				}
				if(p_dev >= 0.10) {
					map_val = 4;      /* Ad   */
					break;
				}
			} else { /* still A, but for > dev */
				if(p_dev >= 0.10) {
					map_val = 10;      /* Adf */
					break;
				}
				if(p_for >= 0.10) {
					map_val = 5;      /* Af   */
					break;
				}
			}
			map_val = 1;            /* A */
			break;
		} /* if p_agr > .6 */
		if(p_dev >= 0.60) {    /* it's a D matrix */
			if(p_dev == 1.0) { /* it's all developed */
				map_val = 19; /* A* */
				break;
			}
			if(p_for > p_agr) {  /* sort the other two */
				if(p_agr >= 0.10) {
					map_val = 11;      /* Daf */
					break;
				}
				if(p_for >= 0.10) {
					map_val = 6;      /* Df   */
					break;
				}
			} else { /* still D, but agr > for */
				if(p_for >= 0.10) {
					map_val = 11;      /* Daf */
					break;
				}
				if(p_agr >= 0.10) {
					map_val = 7;      /* Da   */
					break;
				}
			}
			map_val = 2;            /* D */
			break;
		} /* if p_dev > .6 */
		/* None of the 3 classes was > 60%, so it's a mosaic */
		/* Just find those >10% and list alphabetically */
		/* Note that if only 1 class is > 10%, then it was handled as a matrix, above */
		if(p_agr >= 0.10) {
			if(p_dev >= 0.10) {
				if(p_for >= 0.10) {
					map_val = 16;   /* adf */
					break;
				} else {
					map_val = 13;   /* ad */
					break;
				}
			} else {
				if(p_for >= 0.10) {
					map_val = 14;   /* af */
					break;
				}
			}
		} else { /* agr is less than 10%, it must be mosaic 'df' */
			map_val = 15;   /* df */
			break;
		}
		map_val = 0;   /* just in case something slipped thru */
		break;
	} /* termination of while(1) loop */
		return(map_val);
}
if (lpt_model == 2) {
	while (1) {  /* kludge to permit use of break statements */
		/* check the corners first */
		if(p_for == 1.0) { /* it's all 'forest' */
			map_val = 170;
			break;
		}
		if(p_dev == 1.0) { /* it's all 'dev' */
			map_val = 190;
			break;
		}
		if(p_agr == 1.0) { /* it's all 'agr' */
			map_val = 180;
			break;
		}
		/* work through the triangle starting at lower left. The use of "<" in if statements for pfor and pagr,
		along with the >= in if statements for pdev, ensure the threshold is like 'X% or more' */
		if(p_for < 0.1) {
			if(p_agr < 0.1) {
				if(p_dev >= 0.9) { map_val = 191; break; } map_val = 192; break;
			}
			if(p_agr < 0.2) {
				if(p_dev >= 0.8) { map_val = 71; break; } map_val = 72; break;
			}
			if(p_agr < 0.3) {
				if(p_dev >= 0.7) { map_val = 73; break; } map_val = 74; break;
			}
			if(p_agr < 0.4) {
				if(p_dev >= 0.6) { map_val = 75; break; } map_val = 131; break;
			}
			if(p_agr < 0.5) {
				if(p_dev >= 0.5) { map_val = 132; break; } map_val = 133; break;
			}
			if(p_agr < 0.6) {
				if(p_dev >= 0.4) { map_val = 134; break; } map_val = 135; break;
			}
			if(p_agr < 0.7) {
				if(p_dev >= 0.3) { map_val = 45; break; } map_val = 44; break;
			}
			if(p_agr < 0.8) {
				if(p_dev >= 0.2) { map_val = 43; break; } map_val = 42; break;
			}
			if(p_agr < 0.9) {
				if(p_dev >= 0.1) { map_val = 41; break; } map_val = 182; break;
			}
			map_val = 181;
			break;
		}
		if(p_for < 0.2) {
			if(p_agr < 0.1) {
				if(p_dev >= 0.8) { map_val = 61; break; } map_val = 62; break;
			}
			if(p_agr < 0.2) {
				if(p_dev >= 0.7) { map_val = 111; break; } map_val = 112; break;
			}
			if(p_agr < 0.3) {
				if(p_dev >= 0.6) { map_val = 114; break; } map_val = 200; break;
			}
			if(p_agr < 0.4) {
				if(p_dev >= 0.5) { map_val = 201; break; } map_val = 202; break;
			}
			if(p_agr < 0.5) {
				if(p_dev >= 0.4) { map_val = 203; break; } map_val = 204; break;
			}
			if(p_agr < 0.6) {
				if(p_dev >= 0.3) { map_val = 205; break; } map_val = 206; break;
			}
			if(p_agr < 0.7) {
				if(p_dev >= 0.2) { map_val = 103; break; } map_val = 102; break;
			}
			if(p_agr < 0.8) {
				if(p_dev >= 0.1) { map_val = 101; break; } map_val = 52; break;
			}
			map_val = 51;
			break;
		}
		if(p_for < 0.3) {
			if(p_agr < 0.1) {
				if(p_dev >= 0.7) { map_val = 63; break; } map_val = 64; break;
			}
			if(p_agr < 0.2) {
				if(p_dev >= 0.6) { map_val = 113; break; } map_val = 222; break;
			}
			if(p_agr < 0.3) {
				if(p_dev >= 0.5) { map_val = 223; break; } map_val = 224; break;
			}
			if(p_agr < 0.4) {
				if(p_dev >= 0.4) { map_val = 225; break; } map_val = 226; break;
			}
			if(p_agr < 0.5) {
				if(p_dev >= 0.3) { map_val = 227; break; } map_val = 228; break;
			}
			if(p_agr < 0.6) {
				if(p_dev >= 0.2) { map_val = 207; break; } map_val = 208; break;
			}
			if(p_agr < 0.7) {
				if(p_dev >= 0.1) { map_val = 104; break; } map_val = 54; break;
			}
			map_val = 53;
			break;
		}
		if(p_for < 0.4) {
			if(p_agr < 0.1) {
				if(p_dev >= 0.6) { map_val = 65; break; } map_val = 155; break;
			}
			if(p_agr < 0.2) {
				if(p_dev >= 0.5) { map_val = 221; break; } map_val = 220; break;
			}
			if(p_agr < 0.3) {
				if(p_dev >= 0.4) { map_val = 235; break; } map_val = 234; break;
			}
			if(p_agr < 0.4) {
				if(p_dev >= 0.3) { map_val = 236; break; } map_val = 230; break;
			}
			if(p_agr < 0.5) {
				if(p_dev >= 0.2) { map_val = 229; break; } map_val = 210; break;
			}
			if(p_agr < 0.6) {
				if(p_dev >= 0.1) { map_val = 209; break; } map_val = 141; break;
			}
			map_val = 55;
			break;
		}
		if(p_for < 0.5) {
			if(p_agr < 0.1) {
				if(p_dev >= 0.5) { map_val = 154; break; } map_val = 153; break;
			}
			if(p_agr < 0.2) {
				if(p_dev >= 0.4) { map_val = 219; break; } map_val = 218; break;
			}
			if(p_agr < 0.3) {
				if(p_dev >= 0.3) { map_val = 233; break; } map_val = 232; break;
			}
			if(p_agr < 0.4) {
				if(p_dev >= 0.2) { map_val = 231; break; } map_val = 212; break;
			}
			if(p_agr < 0.5) {
				if(p_dev >= 0.1) { map_val = 211; break; } map_val = 143; break;
			}
			map_val = 142;
			break;
		}
		if(p_for < 0.6) {
			if(p_agr < 0.1) {
				if(p_dev >= 0.4) { map_val = 152; break; } map_val = 151; break;
			}
			if(p_agr < 0.2) {
				if(p_dev >= 0.3) { map_val = 217; break; } map_val = 216; break;
			}
			if(p_agr < 0.3) {
				if(p_dev >= 0.2) { map_val = 215; break; } map_val = 214; break;
			}
			if(p_agr < 0.4) {
				if(p_dev >= 0.1) { map_val = 213; break; } map_val = 145; break;
			}
			map_val = 144;
			break;
		}
		if(p_for < 0.7) {
			if(p_agr < 0.1) {
				if(p_dev >= 0.3) { map_val = 95; break; } map_val = 94; break;
			}
			if(p_agr < 0.2) {
				if(p_dev >= 0.2) { map_val = 124; break; } map_val = 122; break;
			}
			if(p_agr < 0.3) {
				if(p_dev >= 0.1) { map_val = 123; break; } map_val = 84; break;
			}
			map_val = 85;
			break;
		}
		if(p_for < 0.8) {
			if(p_agr < 0.1) {
				if(p_dev >= 0.2) { map_val = 93; break; } map_val = 92; break;
			}
			if(p_agr < 0.2) {
				if(p_dev >= 0.1) { map_val = 121; break; } map_val = 82; break;
			}
			map_val = 83;
			break;
		}
		if(p_for < 0.9) {
			if(p_agr < 0.1) {
				if(p_dev >= 0.1) { map_val = 91; break; } map_val = 172; break;
			}
			map_val = 81;
			break;
		}
		map_val = 171;
		break;
	}/* termination of while(1) loop */
		return(map_val);
}
/* in case something fell through */
	map_val = 0;
	return(map_val);
}
