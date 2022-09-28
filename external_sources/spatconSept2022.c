/* ************************************************************************
  SPATCONV.C   spatial convolution routines.
Version March 2014 modifications for use with Guidos platform.
Objectives:
1. adapt for 64-bit OS's
2. revise I/O to work with Guidos/IDL environment (done)
3. Get rid of some things:
 * the 'which_slider' code because step size is always 1.(done)
 * conditional compilation for K&R C (done)
 * unused variables etc (done)
4. Anything else needed to add support for windows and mac compilation
 *
compile flags on linux gcc: 
 * gcc -std=c99 -m64 -O2 -Wall -fopenmp spatcon.c -lm  -o spatcon

		environment variable for omp (optional); defaults to maximum available in shell):
 		export OMP_NUM_THREADS=N, or setenv OMP_NUM_THREADS N (N is a positive integer)
static build, from Peter 23 sept 2021
on Debian (using gcc version 10.2):  
  gcc -std=c99 -m64 -O2 -Wall -fopenmp spatconomp.c -o spatcon_lin64_debian11 -lm -static /usr/lib/gcc/x86_64-linux-gnu/10/libgomp.a
oon PCLOS (using gcc version 11.2):  
  gcc -std=c99 -m64 -O2 -Wall -fopenmp spatconomp.c -o spatcon_lin64_pclos -lm -static /usr/lib/gcc/x86_64-mandriva-linux-gnu/11.2.0/libgomp.a
----------
Sept 2022.
added better handling of mixed-type arithmetic in FREQ_FILTERS and FREQ_FILTERS_FLOAT.
	for mapping rules 50-76 only, ensured arithmetic was not mixed type.
	before, rule 76 had random differences in output values between successive runs of same input map,
	depending on which versions of clibs were used (ask Peter).
	changed rules 50-76 only. There was no problem with rules 77, 78, 81 so no changes were made there.

Sept 2021. 
added openmp to parallize the big do loop
	Had to remove the 'p' and 'j' options (the 7 August and 28 July 2018 mods below)
	ecause they are incompatible with omp, because they involve disk writes in the freq-filters function 
	which is called in parallel fashion

7 August 2018.
Added a parameter to allow export of unsigned integer map of the count of focal class pixels instead
	of byte values of percentages when running rule 81
2 July 2018.
Implemented a 103-class version of the LPT classification, supporting bins defined by 10% thesholds on all 
        three axes, plus the three corner points. (a) add a new mapping rule, and (b) new code for that rule
        in Freq_Filters subroutine. It is ok to select '3map output' for this rule.
28 July 2017. 
Got rid of extra 'padding' on right and bottom which accommodated grain step sizes other than 1.
	Grain was fixed at step size 1 since 2005, but the code which supported other choices was not removed.
	(not all of the unnecessary code was removed, only the part that calculated the buffering, because I
	want to be able to calculate the buffering based on window size alone to support the next item.)
Added a parameter to allow export of three additional maps when running lptmaker. The three maps are
	Pa, Pn, and Pd in the middle of the Freq_filters subroutine. Because of where this export is located,
	those three maps will still have a half-window of buffering that has to be removed externally.


19 July 2017. Added ifdef(apple) to include omp.h in that case
June 2017. Minor edit to handle mis-use of rule 78 to get Pxx (rule 77). Doing this resulted in double-
counting the diagonal of the adjacency matrix. The fix is to check for self-join and if so, don't double count.nt.
	
Version Sept 2008
Adds output option of float arrays for Px, Pxx.
The target is arcinfo float file, which has a header like ascii header but with parameter 'byteorder.'
Header is still not used here, so changes to header must be made externally.
	changes-
	outfloat parameter in parfile and parameter array
With float output, "-0.01" indicates missing value.
Tested float output only for mapping rules 77 and 81 (px and pxx),
but it should be ready to test for anything else found in new subroutine Freq_Filters_Float

Version March 2007
LPT adds 3 classes for the corners of the triangle, so num classes is 19 again
(cleaned up description and checked algorithm)
Version Nov 2006
LPT code now does 16 classes instead of 19.
LPT code now returns missing ONLY if all pixels in window are missing
Version Dec 2005
revised from spatcon04 which had lpt bug fixed and tw's new rule included
major overhaul to do only bsq i/o, to get rid of unused junk, clean up memory management
to see if larger images can be processed, and reduce number of options.
notes -
1. grain is always 1 now
2. missing value codes should refer to codes after recoding, if recoding is used.  The missing
   values are not included in the computations of an index value.  However, this program no longer
   sets incoming missing pixels to outgoing missing pixels.  That is, a pixel can be missing when
   calculating an index, but there will be an index value for missing pixels.  When specifying the
   recode table and missing value code, remember that arcinfo, when writing a bsq, sets the background
   to zero.  So the recode table should deal with that if needed.
3. no max window size but min window size is still 3
4. header info is read from .siz file which is kr's version of arc header.  The
   first two rows of the .siz file have nrows and ncols, the other rows are not read.
   No new header is written by this program because it is the same as the input file.
   Version July 2006 for lpt work
   changed missing window test in lpt calculator, now only return missing if ALL in window are missing.

************************************************************************ */
/*  Usage: this version is set up to be run via command line arguments (classic mode)
           and via a call from IDL/GUIDOS (guidos mode).
	The args are checked and if argc = 4 then command line usage is assumed.  If argc = 1
	then guidos mode is assumed.

Classic Mode Usage: spatconv <infile> <outfile> <parameterfile>
           arg 1 =input file (everything except the ".bsq" extension)
           arg 2 =output file
           arg 3 =parameter file
         Output:
                <arg2> = convolved data matrix (a bsq file)
		no new header file is output
		The format of the parameter file...each line has a parameter code, a space, and a parameter value.
	 Use as many lines as needed.
                m or M = missing value code
                w or W = window size
                r or R = mapping rule
                h or H = how to handle missing values
                a or A = one selected code, value depends on mapping rule
                b or B = another selected code
                z or Z = recode the incoming pixels
		        f or F = output as float file, 4 bytes per pixel (selected mapping rules only)

                The default values for the optional parameters are all zero.
	If z or Z is specified, then this program looks for a file called
     '<arg1>.rec' in the same directory as <arg1>.  The file contains the recoding rules.
	Format of recode file:
		old_value new_value
		old_value new_value
		etc, for the old_values to be recoded.
			old_values that are not listed will stay the same.
			All old and new values must be in the range [0,255]
			The values can appear in any order
			If old_values are duplicated then the last one listed will be used.
    The program looks for a 8bit bsq file called '<arg1>.bsq' in the same directory as <arg1>.
	The program looks for a size file called '<arg1>.siz' in the same directory as <arg1>.
	The .siz file has at least two lines (the rest are ignored):
	   nrows xxxxx
       ncols yyyyy
    The original header files are not read or used by this program, and no new header is written.

 *
 * Guidos Mode Usage: spatconv
 *   When argc = 1, the program looks in the current directory for three required files:
		an 8bit bsq file called scinput.
		a parameter text file called scpars.txt. (same format as classic mode, above)
		a size text file called scsize.txt (containing two lines as in classic mode, above)
     If recoding is requested then the program looks for the optional file:
	    a recode text file called screcode.txt (same format as classic mode, above)
 *  The program writes an 8bit bsq file called scoutput in the current directory.
 *
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(__APPLE__)
#include <malloc.h>
#endif
#include <math.h>
#include  <omp.h>

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

/* prototypes */
long int Freq_Conv(long int,long int,long int,long int,long int,long int,long int,long int);
unsigned char Freq_Filters(long int,long int,long int *,long int,long int,long int,long int,long int);
float Freq_Filters_Float(long int,long int,long int *,long int,long int,long int,long int,long int);
long int Read_Parameter_File(FILE *);
/* make the input and output data matrices visible everywhere */
unsigned char *mat_in;
unsigned char *mat_out;
float *mat_outfloat;
/* some useful constants that depend on window size */
struct run_helpers {
	float window_area;
	float window_area_inverse;
	float number_of_edges;
	float number_of_edges_inverse;
};
struct run_helpers constants;

/* the following two must be changed whenever a new mapping rule is added */
#define NUM_MAP_RULES_DEFINED 23
long int ok_mapping_rule[NUM_MAP_RULES_DEFINED]  =
{1,6,7,10,20,21,51,52,53,54,71,72,73,74,75,76,77,78,81,82,83,110,160};

struct run_parameters {
	long int missing_value_code;   /*after recode, if any */
	long int window_size;
	long int map_rule;
	long int handle_missing;
	long int code_1;
	long int code_2;
	long int recode;         /* 0 = no recode, 1 = recode, supply recode values in recode file*/
	long int outfloat;       /* 0 = output bsq chars, 1 = output floats */
};                              /* Default parameter values */
struct run_parameters parameters = {0,0,0,1,0,0,0,0};

int main(int argc, char **argv)
{
	static long int ret_val, vnew, vold, dum;
	static long int recode_table[256];
	static FILE *infile, *outfile, *parfile, *recfile, *sizfile;
	static char filename_par[100], filename_in[100], filename_out[100];
	static char filename_rec[100], filename_siz[100], header_line[30];
	static long int row, col, nrows_in, ncols_in, index, temp_int;
	/*
	index = sizeof(long int);
	printf("\n long int is %ld bytes\n", index);
	index = sizeof(int);
	printf("\n int is %ld bytes\n", index);
	index = sizeof(float);
	printf("\n float is %ld bytes\n", index);
	*/
	/* check command line to see which mode to use, classic or guidos */
	if(argc != 1 && argc != 4) {
		printf("\nSpatconv: Error parsing command line.\n");
		exit(10);
	}
	if(argc == 1) { /* guidos mode */
		strcpy(filename_par,"scpars.txt");
		strcpy(filename_siz, "scsize.txt");
		strcpy(filename_out, "scoutput");
		strcpy(filename_in, "scinput");
		strcpy(filename_rec, "screcode.txt");
	}
	if(argc == 4)  {  /* classic mode */
		strcpy(filename_in,argv[1]);
		strcat(filename_in,".bsq");
		strcpy(filename_out,argv[2]);
		strcat(filename_out, ".bsq");
		strcpy(filename_rec,argv[1]);
		strcat(filename_rec,".rec");
		strcpy(filename_siz,argv[1]);
		strcat(filename_siz,".siz");
		strcpy(filename_par,argv[3]);
	}
	/* Open parameter file and read run parameters */
	if( (parfile = fopen(filename_par, "r") ) == NULL) {
		printf("\nSpatconv:Error opening %s\n", filename_par);
		exit(11);
	}
	ret_val = Read_Parameter_File(parfile);
	if(ret_val != 0) {
		printf("\nSpatconv:Error reading from file %s.\n", filename_par);
		exit(12);
	}
	fclose(parfile);
	/* If re-coding pixels ... */
	if(parameters.recode == 1) {
		for(index = 0; index < 256; index ++) {
			recode_table[index] = index;
		}
		if( (recfile = fopen(filename_rec, "r") ) == NULL) {
			printf("\nSpatconv:Error opening %s\n", filename_rec);
			exit(13);
		}
		printf("Warning: pixels are being recoded using recode file %s.\n", filename_rec);
		printf("   Old code ---> New code\n");
		while(fscanf(recfile, "%ld %ld", &vold, &vnew) == 2) {
			if( (vold < 0) || (vold > 255) ) {
				printf("Oops, recode old value must be 0 to 255.\n");
				fclose(recfile);
				exit(14);
			}
			if( (vnew < 0) || (vnew > 255) ) {
				printf("Oops, recode new value must be 0 to 255.\n");
				fclose(recfile);
				exit(14);
			}
			recode_table[vold] = vnew;
			printf("   %8ld ---> %3ld\n", vold, vnew);
			if(parameters.map_rule == 6) { /* doing LPT's... */
				if( (vnew < 0) || (vnew > 3) ) {
					printf("Oops, recode to 0,1,2,3 only, for Landscape Mosaic.\n");
					fclose(recfile);
					exit(14);
				}
			}
		}
		fclose(recfile);
	}
	setbuf(stdout, NULL);
	printf("Spatial convolution: %s ---> %s\n",filename_in, filename_out);
	printf("Run parameters are: m%ld w%ld r%ld h%ld a%ld b%ld z%ld f%ld\n",
	       parameters.missing_value_code,
	       parameters.window_size, parameters.map_rule,
	       parameters.handle_missing, parameters.code_1, parameters.code_2,
	       parameters.recode, parameters.outfloat);
	/* calculate some run-specific constants */
	constants.window_area = parameters.window_size * parameters.window_size;
	constants.window_area_inverse = 1.0 / constants.window_area;
	constants.number_of_edges =
	    2. * ( parameters.window_size * (parameters.window_size - 1. ) );
	if (constants.number_of_edges > 0) {
		constants.number_of_edges_inverse = 1.0 / constants.number_of_edges;
	} else {
		constants.number_of_edges_inverse = 0.;
	}
	/* Open the input and output files */
	if( (infile = fopen(filename_in, "rb") ) == NULL) {
		printf("\nSpatconv:Error opening %s\n", filename_in);
		exit(15);
	}
	if( (outfile = fopen(filename_out, "wb") ) == NULL) {
		printf("\nSpatconv:Error opening %s\n", filename_out);
		exit(16);
	}
	/* Read the siz file and save nrows and ncols */
	if( (sizfile = fopen(filename_siz, "r") ) == NULL) {
		printf("\nSpatconv:Error opening %s\n", filename_siz);
		fclose(infile);
		fclose(outfile);
		exit(17);
	}
	if(fscanf(sizfile,"%s %ld", header_line, &nrows_in) != 2) { exit(18);}
	if(fscanf(sizfile,"%s %ld", header_line, &ncols_in) != 2) { exit(18);}
	fclose(sizfile);
	printf("Reading %ld columns and %ld rows from file %s.\n", ncols_in, nrows_in, filename_in);
	/* Allocate the resources for mat_in */
	temp_int = nrows_in * ncols_in;
	if( (mat_in = (unsigned char *)calloc( temp_int, sizeof(unsigned char) ) ) == NULL ) {
		printf("\nSpatconv:malloc failed, matrix_in.\n");
		fclose(infile);
		fclose(outfile);
		exit(19);
	}
	/* read the input data*/
	if(fread(mat_in, 1, (nrows_in * ncols_in), infile) != (nrows_in * ncols_in) ) {
		printf("\nSpatconv: error reading input bsq.\n");
		exit(20);
	}
	fclose(infile);
	printf("File read OK.\n");
	/* recode the pixels here if that was requested */
	if(parameters.recode == 1) {
		for (row=0; row < nrows_in; row++) {
			index = row * ncols_in;
			for (col=0; col < ncols_in; col++) {
				dum = *(mat_in + index + col);
				*(mat_in + index + col) = recode_table[dum];
			}
		}
	}
	/* Check, if it's lpt generator, only codes 0 thru 3 allowed*/
	if( (parameters.map_rule == 6) || (parameters.map_rule == 7) ) {
		for (row=0; row < nrows_in; row++) {
			index = row * ncols_in;
			for (col=0; col < ncols_in; col++) {
				dum = *(mat_in + index + col);
				if( (dum < 0 ) || (dum > 3) ) {
					printf("\nSpatconv:pixel is not in range [0,3].\n");
					free(mat_in);
					fclose(outfile);
					exit(21);
				}
			}
		}
	}
	/* Call the convolution subroutine */
	printf("Starting Convolution.\n");
	ret_val = Freq_Conv(nrows_in, ncols_in,
	                    parameters.missing_value_code,
	                    parameters.window_size,
	                    parameters.map_rule,
	                    parameters.handle_missing,
	                    parameters.code_1,
	                    parameters.code_2);
	if(ret_val !=0) {
		printf("\nSpatconv:Error in parameters passed to Freq_Conv.\n");
		free(mat_in);
		fclose(outfile);
		exit(22);
	}
	printf("Convolution completed.\n");
	/* Do the output */
	printf("Writing %s.\n",filename_out);
	if(parameters.outfloat == 1) {
		if(fwrite(mat_outfloat, sizeof(float), (ncols_in * nrows_in), outfile) != (ncols_in * nrows_in) ) {
			printf("\nSpatconv: error writing output file.\n");
			exit(23);
		}
	}
	if(parameters.outfloat == 0) {
		if(fwrite(mat_out, 1, (ncols_in * nrows_in), outfile) != (ncols_in * nrows_in) ) {
			printf("\nSpatconv: error writing output file.\n");
			exit(24);
		}
	}
	/* Exit nicely */
	fclose(outfile);
	printf("File written OK.\n");
	if(parameters.outfloat == 0) {
		free(mat_out);
	}
	if(parameters.outfloat == 1) {
		free(mat_outfloat);
	}
	printf("Spatconv:Normal Finish.\n");
	exit(0);
}

/*   *********************
     Read_Parameter_File
     *********************
*/
long int Read_Parameter_File(FILE *parfile)
{
	static char ch;
	static long int value, flag;
	value = -99;
	flag = 1;
	while(flag == 1) {
		if (fscanf(parfile,"%s %ld", &ch, &value) != 2) {
			flag = 0;
			continue;
		}
		if((ch == 'm') || (ch == 'M')) {
			parameters.missing_value_code = value;
			continue;
		}
		if((ch == 'w') || (ch == 'W')) {
			parameters.window_size = value;
			continue;
		}
		if((ch == 'r') || (ch == 'R')) {
			parameters.map_rule = value;
			continue;
		}
		if((ch == 'h') || (ch == 'H')) {
			parameters.handle_missing = value;
			continue;
		}
		if((ch == 'a') || (ch == 'A')) {
			parameters.code_1 = value;
			continue;
		}
		if((ch == 'b') || (ch == 'B')) {
			parameters.code_2 = value;
			continue;
		}
		if((ch == 'z') || (ch == 'Z')) {
			parameters.recode = value;
			continue;
		}
		if((ch == 'f') || (ch == 'F')) {
			parameters.outfloat = value;
			continue;
		}
		if((ch == 'p') || (ch == 'P')) {
			continue;
		}
		if((ch == 'j') || (ch == 'J')) {
			continue;
		}
		return(1);
	}
	if(value == -99) {
		return(1);
	}
	return(0);
}
/*   ***********
     Freq_Conv.C
     ***********
*/
/* **********************************************************************
  FREQCONV.C  A program to do a spatial convolution on a raster image, based
                  on the frequency distribution of color codes or edge codes
                  within a spatial mask or window.
   Parameters:
        Number of rows and columns in the input matrix.
        Missing value code
        Run options (see below)
                Window size
                Mapping rule
                Handling of 'missing' values
                'Selected' codes 1 & 2
                'Selected' values 1 & 2

                **********************
                |     Run Options    |
                **********************
Window size:
        3 = 3x3
        5 = 5x5
          ...

Mapping rule
         1 = Majority (modal) color code
         6 = LPT's (see special note on re-coding pixels for LPT's, below)
         7 = LPT's 103-class version added July 2018. (see recoding note below)
         10 = #_Colors
         20 = Median color code
         21 = Arithmetic average color code
         5x = Color diversity:
                51 = Simpson diversity [0,1]
                52 = Simpson evenness  [0,1]
                53 = Shannon evenness  [0,1]
                54 = Pmax              [0,1]
         6x = (reserved for) additional diversity
         7x = Edge analysis (with regard to order of pixels in pairs):
                71 = Angular second moment                [0,1]
                72 = Simpson edge-type evenness           [0,1]
                73 = Shannon edge-type evenness           [0,1]
                74 = Sum of diagonals of adjacency matrix [0,1]
                75 = Proportion of all local edges involving one particular color
                        i.e. the marginal over that color / total
                76 = Proportion of all local edges which are between two
                        particular colors, if the two colors are the same,
                        you get the proportion of all edges which are self-joins
                        for that color
                77 = Proportion of all local edges involving a particular
                        color which are self-joins for that color, sort of a local
                        contagion estimator
                78 =  (added Dec 99) Proportion of all local edges involving a particular color
                	    that are joins between that color and some other specified color,
                	    i.e., some non-diagonal element / marginal for that color.
         8x = Color density / ratios
                81 = Local density of one particular color [0,1] ('local' == within window)
                    = # of that color / # in window
                82 = Local ratio of numbers of two particular colors [0,1,0]
                    = # of one color / # of second color
                        if # second color = 0 and # first color > 1, then index is 255
                        if # first color = 0 and # second color > 1, then index is 1
                        if # both colors = 0, then index is set to 0 (missing)
                        if #first color > #second color, the index increases from
                                 129 to 254 with increasing # first color.
                        if #second color > #first color, the index decreases from
                                 127 to 2 with increasing # second color.
                        if #first color = #second color, the index is 128.
                83 = Local combined ratio of two particular colors
                    = # one color / (sum of # of both colors)
                        if # both colors = 0, the index is set to 0 (missing)
                        otherwise scaled to [1,255]
        Handling of 'missing' values:
        1 = Ignore them in the convolution function
        2 = Treat as any other color code
'Selected' codes are used for mapping rules:
        if mapping rule 81, code_1 is the one particular color
        if mapping rule 82, code_1 is the numerator and code_2 is denominator
        if mapping rule 75 or 77, code_1 is the one particular color
        if mapping rule 76, code_1 and code_2 are the two particular colors
         - if code_1 and code_2 are different, it's (i,j) + (j,i) / total
         - if code_1 = code_2, it's (i,i) / total
         if mapping rule is 78, code_1 is the particular color whose marginal is
           the denominator, and code_2 is the specified color whose joins with code_1
            are the numerator

******************************
|   Hardwired restrictions   |
******************************
        1. Window size is odd, minimum 3, no maximum.
        2. The window is square.
        3. Output grain size is 1.
        4. The output grain is square.
        5. The window size must be <= # rows and # columns of the data matrix
        7. The diversity values are calculated so that they end up on a
           scale of [0,1], and then mapped onto character range [1,255].
           this preserves 0 locally for 'missing' values.
        8. Ratio values are also [0,1] --> [1,255], with missing == 0
           and, if numerator > 0 and denominator = 0, set to 255.
           Special coding rules used for ratios in mapping rule 82.

NOTES:
        2. Local color codes are assigned if the convolution does not
           require the actual map value.  if actual map values are
           required, e.g., for median or averaging, then local color
           codes are not assigned.
        3. Recoding pixels for LPT's.... map rules 6 and 7
           The LPT rule assumes that there are four possible codes, namely
           0=missing (M), 1=agric (A), 2=forest (F), 3=developed (D).
           When using this rule it is necessary to recode the incoming
           pixels into one of these four (this is done with parameter 'z'
           and a file of recode values, see main).
           Rule 6. The LPT function returns one of 20 possible codes: (rule 6)

                1 = A        9 = Fd      17 = F* (100% F)
                2 = D       10 = Adf     18 = A* (100% A)
                3 = F       11 = Daf     19 = D* (100% D)
                4 = Ad      12 = Fad
                5 = Af      13 = ad
                6 = Df      14 = af
                7 = Da      15 = df
                8 = Fa      16 = adf
           Code 0 is returned whenever the window is all missing.
           Rule 7. The LPT function returns one of 103 possible codes based on 10% threshold binning (rule 7)
           Code 0 is returned whenever the window is all missing.

*********************************************************************** */
long int Freq_Conv(long int n_rows_in, long int n_cols_in, long int missing, long int window_size,
              long int mapping_rule, long int handle_missing, long int code_1, long int code_2)
{
	 long int okay, counter, n_colors_in_image, edge_freq,
	       color_freq, array_length, preserve_original_colors, t1, t2;
	 long int n_rows, n_cols, buff_b, temp_int,
	       n_places_right, n_places_down, row, col, index, pos_in,
	       grain_row, grain_col, grain_size, r, c, r_min, r_max, c_min, c_max,
	       grain_start_row, grain_start_col, new_c_min, new_c_max;
	 unsigned char map_value;
	 float map_value_float;
	 long int in_to_out[256], out_to_in[256];
	 unsigned char *mat_temp;
	// for omp, declare this inside the loop over rows
//	 long int *freq_ptr;
	grain_size = 1; /* this used to be variable but is fixed now*/

	for(temp_int=0; temp_int<256; temp_int++) {
		in_to_out[temp_int] = -9;
		out_to_in[temp_int] = -9;
	}
	/* Process hardwired restrictions */
	/* Window size is odd, minimum 3 */
	if( (temp_int = window_size % 2) == 0) {
		return(1);
	}
	if( window_size < 3) {
		return (2);
	}
	/* Window size must be <= # rows and # columns of the data matrix */
	if( (window_size > n_rows_in) || (window_size > n_cols_in) ) {
		return(5);
	}
	/* See if original or local color codes will be used */
	/* if it's lpts, don't recode again, but later just use first three
	   positions in array.  the arithmetic operators need to know real value*/
	if( ( (mapping_rule > 19) && (mapping_rule < 30) ) ||
	    (mapping_rule == 6) || (mapping_rule == 7) ) {
		preserve_original_colors = 1;   /* avgs and mfs need real numbers*/
		for(temp_int=0; temp_int<256; temp_int++) {
			in_to_out[temp_int] = temp_int;
			out_to_in[temp_int] = temp_int;
		}
		in_to_out[0] = missing;
		out_to_in[missing] = 0;
	} else {
		preserve_original_colors = 0;
	}
	/* Check code for handling missing values of the matrix */
	if( (handle_missing > 2) || (handle_missing < 0) ) {
		return(7);
	}
	/* Check for acceptable code for mapping rule */
	okay=0;
	for(temp_int = 0; temp_int < NUM_MAP_RULES_DEFINED; temp_int++) {
		if(mapping_rule == ok_mapping_rule[temp_int]) {
			okay=1;
			break;
		}
	}
	if(!okay) {
		return(8);
	}
	/* Initialize some resources and other preparations */
	/* First, make a copy of the data matrix, buffering it all around, */
	/*   sizing it to a full multiple of window_size, and re-coding colors */
	/*   if required */
	buff_b = (window_size - 1) / 2;  /* one side of image */
	n_cols = n_cols_in + (2 * buff_b);
	n_rows = n_rows_in + (2 * buff_b);
	printf("Convolution specs:\n     - Padded matrix has %ld cols and %ld rows.\n",
	       n_cols, n_rows);
	/* Allocate resources for the mat_temp to hold buffered input map */
	if( (mat_temp = (unsigned char *)calloc( (n_rows*n_cols), sizeof(unsigned char) ) ) == NULL ) {
		printf("\nSpatconv:malloc failed for mat_temp in Freq_Conv.\n");
		exit(25);
	}
	/* Copy over the data matrix, buffering all around */
	printf("          - moving input data to padded matrix\n");
	/*  Zero is used locally for the missing value code */
	for(row = 0; row < buff_b; row++) {          /* buffer the top */
		index = row * n_cols;
		for(col = 0; col < n_cols; col++) {
			*(mat_temp + index + col) = 0;
		}
	}
	counter = 0;  /* if recoding,incremented when new colors are found, starting at 1*/
	for(row = buff_b; row < (buff_b + n_rows_in); row++) {
		index = row * n_cols;
		for(col = 0; col < buff_b; col++) {    /* buffer the left */
			*(mat_temp + index + col) = 0;
		}
		if(preserve_original_colors == 0) {    /* permit re-coding */
			for(col = buff_b; col < (buff_b + n_cols_in); col++) {
				/* equivalent position in mat_in */
				pos_in = ((row - buff_b) * n_cols_in) + (col - buff_b);
				/* Assign new type code if needed, checking for missing */
				temp_int = (*(mat_in + pos_in));
				if( out_to_in[temp_int] == -9) {
					/* A new code was found */
					if( temp_int == missing) {
						out_to_in[temp_int] = 0;
						in_to_out[0] = temp_int;
					} else {
						counter++;
						out_to_in[temp_int] = counter;
						in_to_out[counter] = temp_int;
					}
				}
				*(mat_temp + index + col) = out_to_in[temp_int];
			}
		} else {    /* just use original codes, taking care of missing values */
			for(col = buff_b; col < (buff_b + n_cols_in); col++) {
				/* equivalent position in mat_in */
				pos_in = ((row - buff_b) * n_cols_in) + (col - buff_b);
				temp_int = (*(mat_in + pos_in));
				*(mat_temp + index + col) = out_to_in[temp_int];
			}
			counter = 255;  /* this will become n_colors_in_image below*/
		}
		for(col = (buff_b + n_cols_in); col < n_cols; col++) {
			*(mat_temp + index + col) = 0;   /* buffer the right */
		}
	}
	for(row = (buff_b + n_rows_in); row < n_rows; row++) { /*buffer the bottom*/
		index = row * n_cols;
		for(col = 0; col < n_cols; col++) {
			*(mat_temp + index + col) = 0;
		}
	}
	n_colors_in_image = counter;  /* does not include missing */
	/* alter user-specified special codes to local codes */
	parameters.code_1 = out_to_in[parameters.code_1];
	parameters.code_2 = out_to_in[parameters.code_2];
	printf("     - Local codes 1 and 2 are %ld and %ld\n",
	       parameters.code_1,parameters.code_2);
	/* The matrix with local color codes is now ready to be convolved */
	/* mat_temp will be convolved to mat_out */
	/* get rid of mat_in and malloc mat_out*/
	free(mat_in);
	/* Allocate resources for the mat_out or mat_outfloat, pointer declared in common area*/
	/* mat_out -- This will always be an 8-bit map of scores, colors, etc. */
	/* added mat_outfloat Sept08, 32-bit floats */
	if(parameters.outfloat == 0) {
		temp_int = n_rows*n_cols;
		if( (mat_out = (unsigned char *)calloc( temp_int, sizeof(unsigned char) ) ) == NULL ) {
			printf("\nSpatconv:malloc failed for mat_out in Freq_Conv.\n");
			exit(26);
		}
	}
	if(parameters.outfloat == 1) {
		temp_int = sizeof(float) * n_rows * n_cols;
		if( (mat_outfloat = (float *)malloc( temp_int ) ) == NULL ) {
			printf("\nSpatconv:malloc failed for mat_outfloat in Freq_Conv.\n");
			exit(27);
		}
	}
	/* The matrix has been resized and the convolution will proceed in steps of size 1 */
	/* all of the references to grain size are for when the step size could be somththing other than 1 */
	/* The number of placements from left to right is... */
	if( (temp_int = n_cols_in % grain_size) == 0)  {
		n_places_right = n_cols_in / grain_size;
	} else {
		n_places_right = (n_cols_in / grain_size) + 1;
	}
	/* The number of placements from top to bottom is... */
	if( (temp_int = n_rows_in % grain_size) == 0)  {
		n_places_down = n_rows_in / grain_size;
	} else {
		n_places_down = (n_rows_in / grain_size) + 1;
	}
	printf("     - Number of window placements l-->r %ld    t-->b %ld\n", n_places_right,n_places_down);
	/* prepare some space for tabulations of edges or colors, depending on rule*/
	if( (mapping_rule > 70) && (mapping_rule < 80) ) {
		/* will be counting edge types, with regard to order here */
		printf("     - Counting edges (frequency of pairs of pixel colors).\n");
		edge_freq = 1;
		color_freq = 0;
		array_length = (n_colors_in_image + 1) * (n_colors_in_image + 1);
//		/* malloc an adjacency matrix, allowing for missing values */
//		array_length = (n_colors_in_image + 1) * (n_colors_in_image + 1);
//		if( (freq_ptr = (long int *)calloc( array_length, sizeof(long int) ) ) == NULL ) {
//			printf("\nError: malloc failed in Freq_Conv.\n");
//			exit(2);
//		}
	} else { /* will be counting colors */
		color_freq = 1;
		edge_freq = 0;
		array_length = n_colors_in_image + 1;
		printf("     - Counting pixels (frequency of pixel colors).\n");
		/* malloc a freq distn, allowing for missing values */
//		array_length = n_colors_in_image + 1;
//		if( (freq_ptr = (long int *)calloc( array_length, sizeof(long int) ) ) == NULL ) {
//			printf("\nError: malloc failed in Freq_Conv.\n");
//			exit(2);
//		}
	}
	/* Loop thru the image, top to bottom, a row of grains at a time */

//	r_min = 0 - grain_size;
// get (from environment) and set numthreads for omp
	omp_set_num_threads(omp_get_max_threads());
	printf("     - OMP Max number of threads = %d \n", omp_get_max_threads());	
#pragma omp parallel  for  	 private ( grain_row, temp_int, r, c, c_min, r_min, c_max, r_max, t1, t2, map_value, map_value_float, grain_start_row, grain_start_col, index,  grain_col, new_c_max, new_c_min, row, col, pos_in)
	for(grain_row = 0; grain_row < n_places_down; grain_row++) {
//  malloc of freq ptr inside loop, need to free it inside loop also
//		/* malloc an adjacency matrix, allowing for missing values */
		long int *freq_ptr = 0;
		if (edge_freq == 1) {
			if( (freq_ptr = (long int *)calloc( array_length, sizeof(long int) ) ) == NULL ) {
				printf("\nError: malloc failed in Freq_Conv.\n");
				exit(28);
			}
		}
		/* malloc a freq distn, allowing for missing values */
		if(color_freq == 1){
			if( (freq_ptr = (long int *)calloc( array_length, sizeof(long int) ) ) == NULL ) {
				printf("\nError: malloc failed in Freq_Conv.\n");
				exit(29);
			}
		}
		/* Always zero the freq distn at the start of grain row */
		for(temp_int = 0; temp_int < array_length; temp_int++) {
			(*(freq_ptr + temp_int)) = 0;
		}
		/* Seed with the first placement on the left */
		c_min = 0;
//		r_min = r_min + grain_size;
		r_min = grain_row;
		c_max = window_size;
		r_max = r_min + window_size;
		/* Get a frequency distribution of colors or edges within the window*/
		/*  the window goes from r_min,c_min to r_max-1,c_max-1 */
		if(color_freq) {
			for(r = r_min; r < r_max; r++) {
				for(c = c_min; c < c_max; c++) {
					temp_int = (*(mat_temp + (r * n_cols) + c));
					(*(freq_ptr + temp_int))++;
				}
			}
		}
		if(edge_freq) {   /* three loopsets used to avoid lots of if's */
			for(r = r_min; r < r_max-1; r++) {  /* all but last row in window*/
				for(c = c_min; c < c_max-1; c++) {  /* all but last col in window*/
					t1 = (*(mat_temp + (r * n_cols) + c));  /*this cell*/
					t2 = (*(mat_temp + ((r+1) * n_cols) + c)); /*cell below*/
					temp_int = (t1 * (n_colors_in_image + 1)) + t2;
					(*(freq_ptr + temp_int))++;
					t2 = (*(mat_temp + (r * n_cols) + c + 1)); /*cell at right*/
					temp_int = (t1 * (n_colors_in_image + 1)) + t2;
					(*(freq_ptr + temp_int))++;
				}
			}
			for(r = r_min; r < r_max-1; r++) {  /* look at last column */
				t1 = (*(mat_temp + (r * n_cols) + (c_max-1)));  /*this cell*/
				t2 = (*(mat_temp + ((r+1)*n_cols) +(c_max-1))); /*cell below*/
				temp_int = (t1 * (n_colors_in_image + 1)) + t2;
				(*(freq_ptr + temp_int))++;
			}
			for(c = c_min; c < c_max-1; c++) {  /* look at last row */
				t1 = (*(mat_temp + ((r_max-1) * n_cols) + c));  /*this cell*/
				t2 = (*(mat_temp + ((r_max-1) * n_cols) + c + 1)); /*cell at right*/
				temp_int = (t1 * (n_colors_in_image + 1)) + t2;
				(*(freq_ptr + temp_int))++;
			}
		}
		/* Call the convolution function for the seed window, passing the distn*/
		/* call for char or float return depending on output format */
		if(parameters.outfloat == 0) {
			map_value = Freq_Filters(color_freq, edge_freq, freq_ptr, n_colors_in_image,
			                         mapping_rule, handle_missing, code_1, code_2);
			/* Map the results back for this grain */
			grain_start_row = r_min + buff_b;
			grain_start_col = c_min + buff_b;
			index = grain_start_row * n_cols;
			(*(mat_out + index + grain_start_col)) = map_value;
			/* End of the seed window for this grain row */
		}
		if(parameters.outfloat == 1) {
			map_value_float = Freq_Filters_Float(color_freq, edge_freq, freq_ptr, n_colors_in_image,
			                                     mapping_rule, handle_missing, code_1, code_2);
			/* Map the results back for this grain */
			grain_start_row = r_min + buff_b;
			grain_start_col = c_min + buff_b;
			index = grain_start_row * n_cols;
			(*(mat_outfloat + index + grain_start_col)) = map_value_float;
			/* End of the seed window for this grain row */
		}
		/* Proceed to the right, subtracting and adding from the freq distn */
		for(grain_col = 1; grain_col < n_places_right; grain_col++) {
			new_c_min = c_min + grain_size;
			new_c_max = c_max + grain_size;
			if(color_freq) {
				for(r = r_min; r < r_max; r++) {
					/* Subtract from the left */
					for(c = c_min; c < new_c_min; c++) {
						temp_int = (*(mat_temp + (r * n_cols) + c));
						(*(freq_ptr + temp_int))--;
					}
					/* Add from the right */
					for(c = c_max; c < new_c_max; c++) {
						temp_int = (*(mat_temp + (r * n_cols) + c));
						(*(freq_ptr + temp_int))++;
					}
				}
			}
			if(edge_freq) {
				for(r = r_min; r < r_max-1; r++) {  /* all but last row in window*/
					/* subtract from the left, doing every old column */
					for(c = c_min; c < new_c_min; c++) {
						t1 = (*(mat_temp + (r * n_cols) + c));  /*this cell*/
						t2 = (*(mat_temp + ((r+1) * n_cols) + c)); /*cell below*/
						temp_int = (t1 * (n_colors_in_image + 1)) + t2;
						(*(freq_ptr + temp_int))--;
						t2 = (*(mat_temp + (r * n_cols) + c + 1)); /*cell at right*/
						temp_int = (t1 * (n_colors_in_image + 1)) + t2;
						(*(freq_ptr + temp_int))--;
					}
					/* Add from the right, doing every new column, looking left and down */
					/* the new material comes from the joins on the left*/
					for(c = c_max; c < new_c_max; c++) {
						t1 = (*(mat_temp + (r * n_cols) + c));  /*this cell*/
						t2 = (*(mat_temp + ((r+1) * n_cols) + c)); /*cell below*/
						temp_int = (t1 * (n_colors_in_image + 1)) + t2;
						(*(freq_ptr + temp_int))++;
						t2 = (*(mat_temp + (r * n_cols) + c -1)); /*cell at left*/
						/* note order of t1 and t2 switched in the following, the */
						/*  reason...need to store in same order as they will be */
						/*  deleted later... */
						temp_int = (t2 * (n_colors_in_image + 1)) + t1;
						(*(freq_ptr + temp_int))++;
					}
				}
				for(c = c_min; c < new_c_min; c++) {  /* look at last row */
					t1 = (*(mat_temp + ((r_max-1) * n_cols) + c));  /*this cell*/
					t2 = (*(mat_temp + ((r_max-1) * n_cols) + c + 1)); /*cell at right*/
					temp_int = (t1 * (n_colors_in_image + 1)) + t2;
					(*(freq_ptr + temp_int))--;
				}
				/* add from the right, but looking left again */
				for(c = c_max; c < new_c_max; c++) {
					t1 = (*(mat_temp + (r * n_cols) + c));  /*this cell*/
					t2 = (*(mat_temp + (r * n_cols) + c - 1)); /*cell at left*/
					/* note the switch of order of t1,t2...see above */
					temp_int = (t2 * (n_colors_in_image + 1)) + t1;
					(*(freq_ptr + temp_int))++;
				}
			}
			/* Update c_min and c_max */
			c_min = new_c_min;
			c_max = new_c_max;
			/* Call the convolution function for this window, passing the distn*/
			if(parameters.outfloat == 0) {
				map_value = Freq_Filters(color_freq, edge_freq, freq_ptr, n_colors_in_image,
				                         mapping_rule, handle_missing, code_1, code_2);
				/* Map the results back for this grain */
				grain_start_row = r_min + buff_b;
				grain_start_col = c_min + buff_b;
				index = grain_start_row * n_cols;
				(*(mat_out + index + grain_start_col)) = map_value;
			}
			if(parameters.outfloat == 1) {
				map_value_float = Freq_Filters_Float(color_freq, edge_freq, freq_ptr, n_colors_in_image,
				                                     mapping_rule, handle_missing, code_1, code_2);
				/* Map the results back for this grain */
				grain_start_row = r_min + buff_b;
				grain_start_col = c_min + buff_b;
				index = grain_start_row * n_cols;
				(*(mat_outfloat + index + grain_start_col)) = map_value_float;
			}
		}
// omp
	free(freq_ptr);
	} // end of omp parallel for loop
	/* If majority filter, replace with original color codes*/
	if(mapping_rule == 1) {
		for(row = 0; row < n_rows; row++) {
			index = row * n_cols;
			for(col = 0; col < n_cols; col++) {
				temp_int = (*(mat_out + index + col));
				(*(mat_out + index + col)) = in_to_out[temp_int];
			}
		}
	}
	/* Shift mat-out back to mat-in basis, i.e. remove the buffer and pads */
	if(parameters.outfloat == 0) {
		for(row = buff_b; row < (buff_b + n_rows_in); row++) {
			index = row * n_cols;
			for(col = buff_b; col < (buff_b + n_cols_in); col++) {
				/* equivalent position in mat_in */
				pos_in = ((row - buff_b) * n_cols_in) + (col - buff_b);
				(*(mat_out + pos_in)) = (*(mat_out + index + col));
			}
		} // end parallel for
	}
	if(parameters.outfloat == 1) {
		for(row = buff_b; row < (buff_b + n_rows_in); row++) {
			index = row * n_cols;
			for(col = buff_b; col < (buff_b + n_cols_in); col++) {
				/* equivalent position in mat_in */
				pos_in = ((row - buff_b) * n_cols_in) + (col - buff_b);
				(*(mat_outfloat + pos_in)) = (*(mat_outfloat + index + col));
			}
		} // end parallel for
	}
	free(mat_temp);
	return(0);
}
/*   **************
     Freq_Filters.C
     **************
*/
/* **********************************************************************
  FREQ_FILTERS.C
        Contains the functions which convolve from a frequency distribution.
        Examples: Majority color code, Edge-type diversity, Color ratios.
   Parameters:
        color_freq = 1 if frequency distribution is number by color, else 0
        edge_freq  = 1 if frequency distribution is number by edge type, else 0
        freq_ptr   = pointer to frequency distribution
        n_colors_in_image = sets the bounds for the size of freq_ptr matrix,
                                  does not include missing
        mapping_rule = same as in Freq_Conv
        handle_missing = same as in Freq_Conv
        code_1 = user-input altered in Freq_Conv to be the local code
        code_2 =  ditto

   Returns:
        unsigned char value in range [1,255] because 0 is reserved for missing
          values in the calling function.
        What it means depends on the mapping rule, it could be a color
          code or a discretized value of a continuous variable.

   if(color_freq), then freq_ptr is one-dimensional array with the
        count of missing values at (*(freq_ptr)) and the count of different
        colors at (*(freq_ptr + 1)) ... (*(freq_ptr+n_colors_in_image)).
        Some of the positions in that array could have zero counts, which
        means those colors were not present in the sub-window of the image
        which is being passed.

   if(edge_freq), then freq_ptr is a two-dimensional array (adjacency matrix).
        The 0 positions are for missing values.  The dimensions are based on
        n_colors_in_image, but some of the rows/columns can have zero
        marginals, which means that those colors were not present in the
        sub-window which is being passed.  Each edge has been counted just
        once in this array, that is, the counts are with regard to order
        of pixels in pairs.

************************************************************************ */
unsigned char Freq_Filters(long int color_freq, long int edge_freq, long int *freq_ptr, long int n_colors_in_image,
                           long int mapping_rule, long int handle_missing, long int code_1, long int code_2)
{
	 unsigned char map_val;
	 long int index, start, temp_int, temp_int2, ind1, ind2, n_colors_in_window;
	 float temp_float, temp_float_2, temp_float_3;
	 float p_agr, p_for, p_dev;
// changes to avoid warning messages in omp version
	map_val = 0;
	temp_int = 0;
	temp_float = 0.0;
	temp_float_2 = 0.0;
//
	switch(mapping_rule) {
	case 1:   /* Return the most abundant color code in freq_ptr */
		/* Seed max search with first element */
		if(handle_missing == 2) { /*missing included*/
			temp_int = (*(freq_ptr));
			start = 1;
			map_val = 0;
		}
		if(handle_missing == 1) {   /* missing not included */
			temp_int = (*(freq_ptr + 1));
			start = 2;
			map_val = 1;
		}
		for(index = start; index < (n_colors_in_image + 1); index++) {
			if( (*(freq_ptr + index)) > temp_int) {
				map_val = index;
				temp_int = (*(freq_ptr + index));
			}
		}
		break;
	case 6: /* lptmaker here */
		/* Modified July 27-06. Return missing ONLY if window is ALL missing*/
		/* modified Oct 31 - 06. Reduce from 19 to 16 classes */
		/*modified March 23, 2007, to add 3 codes for the cases of the corners, i.e., 100% cases */
		/* the new codes will be 17,18,19 */
		/* also cleaned up the description of the codes elsewhere in this program */
		if( (*(freq_ptr)) == constants.window_area) { /*they're all missing */
			map_val = 0;
			break;
		}
		/* Calculate percentage agriculture, forest, and developed in non-missing window */
		temp_float = constants.window_area - (*(freq_ptr));
		p_agr = (*(freq_ptr + 1)) / temp_float;
		p_for = (*(freq_ptr + 2)) / temp_float;
		p_dev = (*(freq_ptr + 3)) / temp_float;
		/* it is possible that three additional maps will be output for pa, pd, pf, but that has to be handled
			at the very end of the subroutine because a missing window caused a break above this line */
		if(p_for >= 0.60) {    /* it's an F matrix */
			if(p_for == 1.0) { /* it's all 'forest' */
				map_val = 17; /* F* */
				break;
			}
			if(p_dev > p_agr) {  /* sort the other two */
				if(p_agr >= 0.10) {
					map_val = 12;      /* Fad */
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
	case 7: /* 103-class lptmaker here */
		/* Added July 2 2018. Used skeleton from rule 6 above*/
		if( (*(freq_ptr)) == constants.window_area) { /*they're all missing */
			map_val = 0;
			break;
		}
		/* Calculate percentage agriculture, forest, and developed in non-missing window */
		temp_float = constants.window_area - (*(freq_ptr));
		p_agr = (*(freq_ptr + 1)) / temp_float;
		p_for = (*(freq_ptr + 2)) / temp_float;
		p_dev = (*(freq_ptr + 3)) / temp_float;
		/* Note that the output of three float maps is not handled with rule 7, use rule 6 instead */
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
	case 10:  /* Return the number of colors in freq_ptr */
		if(handle_missing == 2) { /*missing included*/
			start = 0;
		}
		if(handle_missing == 1) {   /* missing not included */
			start = 1;
		}
		temp_int = 0;
		for(index = start; index < (n_colors_in_image + 1); index++) {
			if( (*(freq_ptr + index)) > 0) {
				temp_int++;
			}
		}
		map_val = temp_int;
		break;
	case 20:  /* Return the median value of the color frequency distribution
                        No sorting is required because we already have the frequency
                        distribution of color codes in order; just have to sum thru
                        the distn to find what color is at the appropriate position.
                   */
		/* Define the position of the median value */
		if(handle_missing == 2) {       /*missing included*/
			temp_int = (constants.window_area / 2) + 1;  /* actual */
			start = 0;
		}
		if(handle_missing == 1) {   /* missing not included */
			start = 1;
			temp_int = constants.window_area - (*(freq_ptr));
			if( (temp_int % 2) != 0 ) {
				temp_int = (temp_int / 2) + 1;  /* actual */
			} else {
				temp_int = temp_int / 2;    /* one-half less than actual */
			}
		}
		/* find the color at position temp_int */
		temp_float_2 = 0;
		map_val = 0; /* seed it */
		for(index = start; index < (n_colors_in_image + 1); index++) {
			temp_float_2 += (*(freq_ptr + index));
			if(temp_float_2 >= temp_int) {
				map_val = index;
				break;
			}
		}
		if(map_val != 0) {
			break;
		}
		map_val = 0;  /* fall-through indicates error somewhere */
		break;
	case 21:  /* Return the average of the color codes in freq_ptr, get
                         this by weighted average from the freq distn    */
		if( (*(freq_ptr)) == constants.window_area) { /*they're all missing */
			map_val = 0;
			break;
		}
		temp_float_2 = 1.0 / (constants.window_area - (*(freq_ptr)) );
		temp_int = 0;
		for(index = 1; index < (n_colors_in_image + 1); index++) {
			temp_int += (*(freq_ptr + index)) * index;
		}
		temp_float = temp_float_2 * temp_int;
		temp_int = temp_float + 0.5;
		map_val = temp_int;
		break;
	case 51: /* return Simpson diversity of colors, 1 - sum[(Pi)**2] */
	case 52: /* return Simpson evenness, diversity/max diversity */
		if(handle_missing == 2) {  /* include missing values */
			start = 0;
			temp_float_2 = constants.window_area_inverse;
		}
		if(handle_missing == 1) {   /* missing not included */
			start = 1;
			if( (*(freq_ptr)) == constants.window_area) { /*they're all missing */
				map_val = 0;
				break;
			} else {
				temp_float_2 = 1.0 / (constants.window_area - (1.0 * (*(freq_ptr)) ) );
			}
		}
		temp_float = 0.0;
		for(index = start; index < (n_colors_in_image + 1); index++) {
			temp_float_3 = (1.0 * (*(freq_ptr + index)) ) * temp_float_2;
			temp_float += temp_float_3 * temp_float_3;
		}
		temp_float = 1.0 - temp_float; /* this is 'diversity'*/
		if(mapping_rule == 51) {
			temp_int = (temp_float * 254.) + 1;
			map_val = temp_int;
			break;
		}
		if(mapping_rule == 52) { /* change diversity to evenness */
			temp_int2 = 0;  /*counter of number of colors in window*/
			for(index = start; index < (n_colors_in_image + 1); index++) {
				if( (*(freq_ptr + index)) > 0) {
					temp_int2++;
				}
			}
			if(temp_int2 == 1) { /* only one color */
				map_val = 255;
				break;
			} else {
				temp_float = temp_float / (1.0 - (1.0 / (1.0 * temp_int2) ));
				temp_int = (temp_float * 254.) + 1;
				map_val = temp_int;
				break;
			}
		}
		map_val = 0;  /* fall-through indicates error somewhere */
		break;
	case 53: /* return shannon evenness */
		temp_float_2 = 0.0;
		if(handle_missing == 2) {  /* include missing values */
			start = 0;
			temp_float_2 = constants.window_area_inverse;
		}
		if(handle_missing == 1) {   /* missing not included */
			start = 1;
			if( (*(freq_ptr)) == constants.window_area) { /*they're all missing */
				map_val = 0;
				break;
			} else {
				temp_float_2 = 1.0 / (1.0 * (constants.window_area - (*(freq_ptr)) ) );
			}
		}
		temp_float = 0.0;
		for(index = start; index < (n_colors_in_image + 1); index++) {
			temp_float_3 = (1.0 * (*(freq_ptr + index)) ) * temp_float_2;
			if(temp_float_3 > 0) {
				temp_float += temp_float_3 * log(temp_float_3);
			}
		}
		/* temp_float contains shannon diversity,or entropy */
		temp_int2 = 0;  /*counter of number of colors in window*/
		for(index = start; index < (n_colors_in_image + 1); index++) {
			if( (*(freq_ptr + index)) > 0) {
				temp_int2++;
			}
		}
		if(temp_int2 == 1) { /* only one color */
			map_val = 255;
			break;
		} else {
			temp_float = (-1.0 * temp_float) / log( (1.0 * temp_int2) );
			temp_int = (temp_float * 254.) + 1;
			map_val = temp_int;
			break;
		}
		break;
	case 54: /* return Pmax */
		temp_float_2 = 0.0;
		if(handle_missing == 2) {  /* include missing values */
			start = 0;
			temp_float_2 = constants.window_area_inverse;
		}
		if(handle_missing == 1) {   /* missing not included */
			start = 1;
			if( (*(freq_ptr)) == constants.window_area) { /*they're all missing */
				map_val = 0;
				break;
			} else {
				temp_float_2 = 1.0 / (1.0 * (constants.window_area - (*(freq_ptr)) ) );
			}
		}
		temp_float = 0.0;
		for(index = start; index < (n_colors_in_image + 1); index++) {
			temp_float_3 = (1.0 * (*(freq_ptr + index)) ) * temp_float_2;
			temp_float = max(temp_float, temp_float_3);
		}
		temp_int = (temp_float * 254.) + 1;
		map_val = temp_int;
		break;
	case 71: /* return angular second moment, with regard to order*/
		if(handle_missing == 2) {  /*missing included, why would anyone do this*/
			temp_float = 0.;
			for(ind1 = 0; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 0; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_float_2 = (1.0 * (*(freq_ptr + temp_int)) ) *
					               constants.number_of_edges_inverse;
					temp_float += temp_float_2 * temp_float_2;
				}
			}
			temp_int = (temp_float * 254.) + 1;
			map_val = temp_int;
			break;
		}
		if(handle_missing == 1) {  /*missing not included*/
			temp_int2 = 0;  /* will be the divisor */
			/* count the non-missing edges*/
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_int2 += (*(freq_ptr + temp_int));
				}
			}
			/* return missing if there are no non-missing edges */
			if(temp_int2 == 0) {
				map_val = 0;
				break;
			}
			temp_float = 0.;
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_float_2 = (1.0 * (*(freq_ptr + temp_int)) )/ (1.0 * temp_int2);
					temp_float += temp_float_2 * temp_float_2;
				}
			}
			temp_int = (temp_float * 254.) + 1;
			map_val = temp_int;
			break;
		}
		break;
	case 72: /* return Simpson edge-type evenness, or simpson contagion */
		/* Need to know number of colors in window to compute max values */
		if(handle_missing == 2) {  /*missing included, why would anyone do this*/
			n_colors_in_window = 0;
			for(ind1 = 0; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 0; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					if( (*(freq_ptr + temp_int)) > 0) {
						n_colors_in_window++;
						break;
					}
					temp_int = (ind2 * (n_colors_in_image + 1)) + ind1;
					if( (*(freq_ptr + temp_int)) > 0) {
						n_colors_in_window++;
						break;
					}
				}
			}
			if(n_colors_in_window == 1) {
				map_val = 255;
				break;
			}
			temp_float = 0.;
			for(ind1 = 0; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 0; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_float_2 = (1.0 * (*(freq_ptr + temp_int)) ) *
					               constants.number_of_edges_inverse;
					temp_float += temp_float_2 * temp_float_2;
				}
			}
			temp_float = 1.0 - temp_float;
			temp_float_2 = (1.0 - (1.0 / (1.0 * (n_colors_in_window * n_colors_in_window))));
			temp_float = 1.0 - (temp_float / temp_float_2);
			temp_int = (temp_float * 254.) + 1;
			map_val = temp_int;
			break;
		}
		if(handle_missing == 1) {  /*missing not included*/
			n_colors_in_window = 0;
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					if( (*(freq_ptr + temp_int)) > 0) {
						n_colors_in_window++;
						break;
					}
					temp_int = (ind2 * (n_colors_in_image + 1)) + ind1;
					if( (*(freq_ptr + temp_int)) > 0) {
						n_colors_in_window++;
						break;
					}
				}
			}
			if(n_colors_in_window == 0) {
				map_val = 0;
				break;
			}
			if(n_colors_in_window == 1) {
				map_val = 255;
				break;
			}
			temp_int2 = 0;  /* will be the divisor */
			/* count the non-missing edges*/
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_int2 += (*(freq_ptr + temp_int));
				}
			}
			/* return missing if there are no non-missing edges */
			if(temp_int2 == 0) {
				map_val = 0;
				break;
			}
			temp_float = 0.;
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_float_2 = (1.0 * (*(freq_ptr + temp_int)) ) / (1.0 * temp_int2);
					temp_float += temp_float_2 * temp_float_2;
				}
			}
			temp_float = 1.0 - temp_float;
			temp_float_2 = (1.0 - (1.0 / (1.0 * (n_colors_in_window * n_colors_in_window))));
			temp_float = 1.0 - (temp_float / temp_float_2);
			temp_int = (temp_float * 254.) + 1;
			map_val = temp_int;
			break;
		}
		break;
	case 73: /* return shannon edge-type evenness, ie contagion */
		/* Need to know number of colors in window to compute max values */
		if(handle_missing == 2) {  /*missing included, why would anyone do this*/
			n_colors_in_window = 0;
			for(ind1 = 0; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = ind1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_int2 = (ind2 * (n_colors_in_image + 1)) + ind1;
					if(  ( (*(freq_ptr + temp_int)) > 0) ||
					     ( (*(freq_ptr + temp_int2)) > 0)) {
						n_colors_in_window++;
					}
				}
			}
			if(n_colors_in_window == 1) {
				map_val = 255;
				break;
			}
			temp_float = 0.;
			for(ind1 = 0; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 0; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_float_2 = (1.0 * (*(freq_ptr + temp_int)) ) *
					               constants.number_of_edges_inverse;
					if(temp_float_2 > 0) {
						temp_float += temp_float_2 * log(temp_float_2);
					}
				}
			}
			temp_float = -1.0 * temp_float;
			temp_float_2 = 2.0 * log(1.0 * n_colors_in_window);
			temp_float = 1.0 - (temp_float / temp_float_2);
			temp_int = (temp_float * 254.) + 1;
			map_val = temp_int;
			break;
		}
		if(handle_missing == 1) {  /*missing not included*/
			n_colors_in_window = 0;
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = ind1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_int2 = (ind2 * (n_colors_in_image + 1)) + ind1;
					if(  ( (*(freq_ptr + temp_int)) > 0) ||
					     ( (*(freq_ptr + temp_int2)) > 0)) {
						n_colors_in_window++;
					}
				}
			}
			if(n_colors_in_window == 0) {
				map_val = 0;
				break;
			}
			if(n_colors_in_window == 1) {
				map_val = 255;
				break;
			}
			temp_int2 = 0;  /* will be the divisor */
			/* count the non-missing edges*/
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_int2 += (*(freq_ptr + temp_int));
				}
			}
			/* return missing if there are no non-missing edges */
			if(temp_int2 == 0) {
				map_val = 0;
				break;
			}
			temp_float = 0.;
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_float_2 = (1.0 * (*(freq_ptr + temp_int)) ) / (1.0 * temp_int2);
					if(temp_float_2 > 0) {
						temp_float = temp_float + (temp_float_2 * log(temp_float_2));
					}
				}
			}
			temp_float = -1.0 * temp_float;
			temp_float_2 = 2.0 * log(1.0 * n_colors_in_window);
			temp_float = 1.0 - (temp_float / temp_float_2);
			temp_int = (temp_float * 254.) + 1;
			map_val = temp_int;
			break;
		}
		break;
	case 74: /* return sum of diagonal of adjacency matrix */
		if(handle_missing == 2) {  /*missing included, again why? */
			temp_float = 0.;
			/* accumulate frequencies along diagonal in the adjacency matrix */
			for(index = 0; index <= n_colors_in_image; index++) {
				temp_int = (index * (n_colors_in_image + 1)) + index;
				temp_float += (1.0 * (*(freq_ptr + temp_int)) );
			}
			/* and the proportion is... */
			temp_float = temp_float * constants.number_of_edges_inverse;
			temp_int = (temp_float * 254.) + 1;
			map_val = temp_int;
			break;
		}
		if(handle_missing == 1) {  /*missing not included*/
			temp_int2 = 0;  /* will be the divisor */
			/* count all the non-missing edges */
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_int2 += (*(freq_ptr + temp_int));
				}
			}
			/* return missing if there are no non-missing edges */
			if(temp_int2 == 0) {
				map_val = 0;
				break;
			}
			temp_float = 0.;
			/* accumulate frequencies along diagonal in the adjacency matrix */
			for(index = 1; index <= n_colors_in_image; index++) {
				temp_int = (index * (n_colors_in_image + 1)) + index;
				temp_float += (1.0 * (*(freq_ptr + temp_int)) );
			}
			/* and the proportion is... */
			temp_float = temp_float / (1.0 * temp_int2);
			temp_int = (temp_float * 254.) + 1;
			map_val = temp_int;
			break;
		}
		map_val = 0;  /* fall-through indicates error somewhere */
		break;
	case 75: /* return proportion of local edges which involve code_1 */
		if(handle_missing == 2) {  /*missing included*/
			temp_float = 0.;
			/* accumulate across code_1's row in the adjacency matrix */
			for(index = 0; index <= n_colors_in_image; index++) {
				temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + index;
				temp_float += (1.0 * (*(freq_ptr + temp_int)) );
			}
			/* accumulate across code_1's column in the adjacency matrix */
			for(index = 0; index <= n_colors_in_image; index++) {
				temp_int = (index * (n_colors_in_image + 1)) + parameters.code_1;
				temp_float += (1.0 * (*(freq_ptr + temp_int)) );
			}
			/* the diagonal cell has been counted twice, so subtract one of them */
			temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + parameters.code_1;
			temp_float -= (1.0 * (*(freq_ptr + temp_int)) );
			/* and the proportion is... */
			temp_float = temp_float * constants.number_of_edges_inverse;
			temp_int = (temp_float * 254.) + 1;
			map_val = temp_int;
			break;
		}
		if(handle_missing == 1) {  /*missing not included*/
			temp_int2 = 0;  /* will be the divisor */
			/* count all the non-missing edges */
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_int2 += (*(freq_ptr + temp_int));
				}
			}
			/* return missing if there are no non-missing edges */
			if(temp_int2 == 0) {
				map_val = 0;
				break;
			}
			/* count all the edges involving code_1 */
			temp_float = 0.;
			/* accumulate across code_1's row in the adjacency matrix */
			for(index = 1; index <= n_colors_in_image; index++) {
				temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + index;
				temp_float += (1.0 * (*(freq_ptr + temp_int)) );
			}
			/* accumulate across code_1's column in the adjacency matrix */
			for(index = 1; index <= n_colors_in_image; index++) {
				temp_int = (index * (n_colors_in_image + 1)) + parameters.code_1;
				temp_float += (1.0 * (*(freq_ptr + temp_int)) );
			}
			/* the diagonal cell has been counted twice, so subtract one of them */
			temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + parameters.code_1;
			temp_float = temp_float - (1.0 * (*(freq_ptr + temp_int)) );
			/* and the proportion is... (the divide by zero check happened above) */
			temp_float = temp_float / (1.0 * temp_int2);
			temp_int = (temp_float * 254.) + 1;
			map_val = temp_int;
			break;
		}
		break;
	case 76: /* return proportion of edges between code_1 and code_2 */
		if(handle_missing == 2) {  /*missing included*/
			/* sum the i,j and j,i cells */
			temp_int = (parameters.code_1 * (n_colors_in_image + 1)) +
			           parameters.code_2;
			temp_float = (1.0 * (*(freq_ptr + temp_int)) );
			/* don't count a diagonal cell twice if self-joins were requested */
			if(parameters.code_1 != parameters.code_2) {
				temp_int = (parameters.code_2 * (n_colors_in_image + 1)) +
				           parameters.code_1;
				temp_float += (1.0 * (*(freq_ptr + temp_int)) );
			}
			/* and the proportion is... */
			temp_float = temp_float * constants.number_of_edges_inverse;
			temp_int = (temp_float * 254.) + 1;
			map_val = temp_int;
			break;
		}
		if(handle_missing == 1) {  /*missing not included*/
			temp_int2 = 0;  /* will be the divisor */
			/* count all the non-missing edges */
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_int2 += (*(freq_ptr + temp_int));
				}
			}
			/* return missing if there are no non-missing edges */
			if(temp_int2 == 0) {
				map_val = 0;
				break;
			}
			/* sum the i,j and j,i cells */
			temp_int = (parameters.code_1 * (n_colors_in_image + 1)) +
			           parameters.code_2;
			temp_float = (1.0 * (*(freq_ptr + temp_int)) );
			/* don't count a diagonal cell twice if self-joins were requested */
			if(parameters.code_1 != parameters.code_2) {
				temp_int = (parameters.code_2 * (n_colors_in_image + 1)) +
				           parameters.code_1;
				temp_float += (1.0 * (*(freq_ptr + temp_int)) );
			}
			/* and the proportion is... (the divide by zero check happened above) */
			temp_float_3 = 1.0 / (1.0 * temp_int2);
			temp_float = temp_float * temp_float_3;
			temp_int = (temp_float * 254.) + 1;
			map_val = temp_int;
			break;
		}
		break;
	case 77:  /* return proportion of edges with a color which are self-joins*/
		start = 1;  /* don't count edges with missing */
		if(handle_missing == 2) {
			start = 0;    /*missing included*/
		}
		temp_float_2 = 0.;  /* will be divisor */
		/* count all the edges involving code_1*/
		/* accumulate across code_1's row in the adjacency matrix */
		for(index = start; index <= n_colors_in_image; index++) {
			temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + index;
			temp_float_2 += (*(freq_ptr + temp_int));
		}
		/* accumulate across code_1's column in the adjacency matrix */
		for(index = start; index <= n_colors_in_image; index++) {
			temp_int = (index * (n_colors_in_image + 1)) + parameters.code_1;
			temp_float_2 += (*(freq_ptr + temp_int));
		}
		/* the diagonal cell has been counted twice, so subtract one of them */
		temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + parameters.code_1;
		temp_float_2 -= (*(freq_ptr + temp_int));
		if(temp_float_2 == 0.) {   /* set to missing if no edges with that color */
			map_val = 0;
			break;
		}
		/* and the proportion is... */
		temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + parameters.code_1;
		temp_float = (*(freq_ptr + temp_int));
		temp_float = (1.0 * temp_float) / (1.0 * temp_float_2);
		temp_int = (temp_float * 254.) + 1;
		map_val = temp_int;
		break;
	case 78:  /* added 12/99, return proportion of edges for a color that
                  are joins between that color and another specified color */
		start = 1;  /* don't count edges with missing */
		if(handle_missing == 2) {
			start = 0;    /*missing included*/
		}
		temp_float_2 = 0.;  /* will be divisor, the marginal total for code_1 */
		/* count all the edges involving code_1*/
		/* accumulate across code_1's row in the adjacency matrix */
		for(index = start; index <= n_colors_in_image; index++) {
			temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + index;
			temp_float_2 += (*(freq_ptr + temp_int));
		}
		/* accumulate across code_1's column in the adjacency matrix */
		for(index = start; index <= n_colors_in_image; index++) {
			temp_int = (index * (n_colors_in_image + 1)) + parameters.code_1;
			temp_float_2 += (*(freq_ptr + temp_int));
		}
		/* the diagonal cell has been counted twice, so subtract one of them */
		temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + parameters.code_1;
		temp_float_2 -= (*(freq_ptr + temp_int));
		if(temp_float_2 == 0.) {   /* set to missing if no edges with that color */
			map_val = 0;
			break;
		}
		/* Now find the number of joins between code_1 and code_2 */
		temp_float_3 = 0.0;
		/* get the row entry */
		temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + parameters.code_2;
		temp_float_3 +=  (*(freq_ptr + temp_int));
		/* get the column entry and add it */
		/* but not if self joins were requested (don't count the diagonal twice) */
		if(parameters.code_1 != parameters.code_2) {
			temp_int =(parameters.code_2 * (n_colors_in_image + 1)) + parameters.code_1;
			temp_float_3 += (*(freq_ptr + temp_int));
		}
		/* and the proportion is... */
		temp_float = (1.0 * temp_float_3) / (1.0 * temp_float_2);
		temp_int = (temp_float * 254.) + 1;
		map_val = temp_int;
		break;
	case 81: /* return local density of one particular color (code_1) */
		if(handle_missing == 2) { /*missing included*/
			temp_float = (*(freq_ptr + parameters.code_1)) *
			             constants.window_area_inverse;
		}
		if(handle_missing == 1) {   /* missing not included */
			temp_float_2 = 0.;
			for(index = 1; index < (n_colors_in_image + 1); index++) {
				temp_float_2 +=  (*(freq_ptr + index));
			}
			if(temp_float_2 > 0) {
				temp_float = (*(freq_ptr + parameters.code_1)) / temp_float_2;
			} else { /* no non-missing pixels, set to missing */
				map_val = 0;
				break;
			}
		}
		temp_int = (temp_float * 254.) + 1;
		map_val = temp_int;
		break;
	case 82: /* return local ratio of two particular colors (code_1 / code_2) */
		/* set to missing if neither is present */
		if( ( (*(freq_ptr + parameters.code_1)) == 0 ) &&
		    ( (*(freq_ptr + parameters.code_2)) == 0 )    ) {
			map_val = 0;
			break;
		}
		/* set to max if code1> 0 and code2= 0 */
		if( ( (*(freq_ptr + parameters.code_1)) > 0 ) &&
		    ( (*(freq_ptr + parameters.code_2)) == 0 )    ) {
			map_val = 255;
			break;
		}
		/* set to min if code1= 0 and code2> 0 */
		if( ( (*(freq_ptr + parameters.code_1)) == 0 ) &&
		    ( (*(freq_ptr + parameters.code_2)) > 0 )    ) {
			map_val = 1;
			break;
		}
		/* calculate ratio using code_1 in numerator */
		temp_float_2 = (*(freq_ptr + parameters.code_1));
		temp_float_2 = temp_float_2 / (*(freq_ptr + parameters.code_2));
		/* map that onto [129,254] if code_1 is larger */
		if(temp_float_2 > 1.0) {
			temp_float = (1.0 / temp_float_2);
			temp_int = (temp_float * 125.) + 1;
			map_val = 255 - temp_int;
			break;
		}
		/* map that onto [2,127] if code_2 is larger*/
		if(temp_float_2 < 1.0) {
			temp_int = (temp_float_2 * 125.) + 1;
			map_val = 1 + temp_int;
			break;
		}
		if(temp_float_2 == 1.0) {
			map_val = 128;
			break;
		}
		break;
	case 83:  /* return the ratio #code1 / (#code1 + #code2) */
		temp_float_2 = (*(freq_ptr + parameters.code_1)) +
		               (*(freq_ptr + parameters.code_2));
		if(temp_float_2 == 0) {
			map_val = 0;
			break;
		}
		temp_float =  (*(freq_ptr + parameters.code_1)) / temp_float_2;
		temp_int = (temp_float * 254.) + 1;
		map_val = temp_int;
		break;
	default:
		/* do nothing */
		break;
	} /* end of switch statement */
	return(map_val);
}
/*   **************
     Freq_Filters_Float.C
     **************
*/
/* **********************************************************************
  FREQ_FILTERS_FLOAT.C
	The same as Freq_Filters.c (see above) except it returns a float value
	and is only implemented for a few of the mapping rules.
	Only important change is that the variable 'map_val' is declared a float here, instead of unsigned char.
************************************************************************ */
float Freq_Filters_Float(long int color_freq, long int edge_freq, long int *freq_ptr, long int n_colors_in_image,
                         long int mapping_rule, long int handle_missing, long int code_1, long int code_2)
{
	float map_val;
	long int index, start, temp_int, temp_int2, ind1, ind2, n_colors_in_window;
	float temp_float, temp_float_2, temp_float_3;
	map_val = -0.01;
	switch(mapping_rule) {
	case 1:   /* don't do it */
	case 6:
    case 7:
	case 10:
	case 20:
	case 21:
	case 82:
		printf("\nSpatconv: Don't ask for float output for this mapping rule.\n");
		exit(30);
		break;
	case 51:
	case 52: /* return Simpson evenness, diversity/max diversity */
		temp_float_2 = 0.0;
		if(handle_missing == 2) {  /* include missing values */
			start = 0;
			temp_float_2 = constants.window_area_inverse;
		}
		if(handle_missing == 1) {   /* missing not included */
			start = 1;
			if( (*(freq_ptr)) == constants.window_area) { /*they're all missing */
				map_val = -0.01;
				break;
			} else {
				temp_float_2 = 1.0 / (1.0 * (constants.window_area - (*(freq_ptr)) ) );
			}
		}
		temp_float = 0.0;
		for(index = start; index < (n_colors_in_image + 1); index++) {
			temp_float_3 = (1.0 * (*(freq_ptr + index)) ) * temp_float_2;
			temp_float += temp_float_3 * temp_float_3;
		}
		temp_float = 1.0 - temp_float; /* this is 'diversity'*/
		if(mapping_rule == 51) {
			map_val = temp_float;
			break;
		}
		if(mapping_rule == 52) { /* change diversity to evenness */
			temp_int = 0;  /*counter of number of colors in window*/
			for(index = start; index < (n_colors_in_image + 1); index++) {
				if( (*(freq_ptr + index)) > 0) {
					temp_int++;
				}
			}
			if(temp_int == 1) { /* only one color */
				map_val = 1.0;
				break;
			} else {
				temp_float = temp_float / (1.0 - (1.0 / (1.0*temp_int)));
				map_val = temp_float;
				break;
			}
		}
		map_val = -0.01;  /* fall-through indicates error somewhere, set missing */
		break;
	case 53: /* return shannon evenness */
		temp_float_2 = 0.0;
		if(handle_missing == 2) {  /* include missing values */
			start = 0;
			temp_float_2 = constants.window_area_inverse;
		}
		if(handle_missing == 1) {   /* missing not included */
			start = 1;
			if( (*(freq_ptr)) == constants.window_area) { /*they're all missing */
				map_val = -0.01;
				break;
			} else {
				temp_float_2 = 1.0 / (1.0 * (constants.window_area - (*(freq_ptr)) ));
			}
		}
		temp_float = 0.0;
		for(index = start; index < (n_colors_in_image + 1); index++) {
			temp_float_3 = (1.0* (*(freq_ptr + index)) ) * temp_float_2;
			if(temp_float_3 > 0) {
				temp_float += temp_float_3 * log(temp_float_3);
			}
		}
		/* temp_float contains shannon diversity,or entropy */
		temp_int = 0;  /*counter of number of colors in window*/
		for(index = start; index < (n_colors_in_image + 1); index++) {
			if( (*(freq_ptr + index)) > 0) {
				temp_int++;
			}
		}
		if(temp_int == 1) { /* only one color */
			map_val = 1.0;
			break;
		} else {
			temp_float = (-1.0 * temp_float) / log( (1.0* temp_int));
			map_val = temp_float;
			break;
		}
		break;
	case 54: /* return Pmax */
		temp_float_2 = 0.0;
		if(handle_missing == 2) {  /* include missing values */
			start = 0;
			temp_float_2 = constants.window_area_inverse;
		}
		if(handle_missing == 1) {   /* missing not included */
			start = 1;
			if( (*(freq_ptr)) == constants.window_area) { /*they're all missing */
				map_val = -0.01;
				break;
			} else {
				temp_float_2 = 1.0 / (1.0 * (constants.window_area - (*(freq_ptr)) ));
			}
		}
		temp_float = 0.0;
		for(index = start; index < (n_colors_in_image + 1); index++) {
			temp_float_3 = (1.0* (*(freq_ptr + index)) ) * temp_float_2;
			temp_float = max(temp_float, temp_float_3);
		}
		map_val = temp_float;
		break;
	case 71: /* return angular second moment, with regard to order*/
		if(handle_missing == 2) {  /*missing included, why would anyone do this*/
			temp_float = 0.;
			for(ind1 = 0; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 0; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_float_2 = (1.0 * (*(freq_ptr + temp_int)) ) *
					               constants.number_of_edges_inverse;
					temp_float += temp_float_2 * temp_float_2;
				}
			}
			map_val = temp_float;
			break;
		}
		if(handle_missing == 1) {  /*missing not included*/
			temp_int2 = 0;  /* will be the divisor */
			/* count the non-missing edges*/
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_int2 += (*(freq_ptr + temp_int));
				}
			}
			/* return missing if there are no non-missing edges */
			if(temp_int2 == 0) {
				map_val = -0.01;
				break;
			}
			temp_float = 0.;
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_float_2 = (1.0 * (*(freq_ptr + temp_int))) / (1.0*temp_int2);
					temp_float += temp_float_2 * temp_float_2;
				}
			}
			map_val = temp_float;
			break;
		}
		break;
	case 72: /* return Simpson edge-type evenness, or simpson contagion */
		/* Need to know number of colors in window to compute max values */
		if(handle_missing == 2) {  /*missing included, why would anyone do this*/
			n_colors_in_window = 0;
			for(ind1 = 0; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 0; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					if( (*(freq_ptr + temp_int)) > 0) {
						n_colors_in_window++;
						break;
					}
					temp_int = (ind2 * (n_colors_in_image + 1)) + ind1;
					if( (*(freq_ptr + temp_int)) > 0) {
						n_colors_in_window++;
						break;
					}
				}
			}
			if(n_colors_in_window == 1) {
				map_val = 1.0;
				break;
			}
			temp_float = 0.;
			for(ind1 = 0; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 0; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_float_2 = (1.0* (*(freq_ptr + temp_int)) ) *
					               constants.number_of_edges_inverse;
					temp_float += temp_float_2 * temp_float_2;
				}
			}
			temp_float = 1.0 - temp_float;
			temp_float_2 = (1.0 - (1.0 / (1.0 * (n_colors_in_window * n_colors_in_window))));
			temp_float = 1.0 - (temp_float / temp_float_2);
			map_val = temp_float;
			break;
		}
		if(handle_missing == 1) {  /*missing not included*/
			n_colors_in_window = 0;
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					if( (*(freq_ptr + temp_int)) > 0) {
						n_colors_in_window++;
						break;
					}
					temp_int = (ind2 * (n_colors_in_image + 1)) + ind1;
					if( (*(freq_ptr + temp_int)) > 0) {
						n_colors_in_window++;
						break;
					}
				}
			}
			if(n_colors_in_window == 0) {
				map_val = -0.01;
				break;
			}
			if(n_colors_in_window == 1) {
				map_val = 1.0;
				break;
			}
			temp_int2 = 0;  /* will be the divisor */
			/* count the non-missing edges*/
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_int2 += (*(freq_ptr + temp_int));
				}
			}
			/* return missing if there are no non-missing edges */
			if(temp_int2 == 0) {
				map_val = -0.01;
				break;
			}
			temp_float = 0.;
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_float_2 = (1.0* (*(freq_ptr + temp_int)) )/ (1.0*temp_int2);
					temp_float += temp_float_2 * temp_float_2;
				}
			}
			temp_float = 1.0 - temp_float;
			temp_float_2 = (1.0 - (1.0 / (1.0*(n_colors_in_window * n_colors_in_window))));
			temp_float = 1.0 - (temp_float / temp_float_2);
			map_val = temp_float;
			break;
		}
		break;
	case 73: /* return shannon edge-type evenness, ie contagion */
		/* Need to know number of colors in window to compute max values */
		if(handle_missing == 2) {  /*missing included, why would anyone do this*/
			n_colors_in_window = 0;
			for(ind1 = 0; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = ind1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_int2 = (ind2 * (n_colors_in_image + 1)) + ind1;
					if(  ( (*(freq_ptr + temp_int)) > 0) ||
					     ( (*(freq_ptr + temp_int2)) > 0)) {
						n_colors_in_window++;
					}
				}
			}
			if(n_colors_in_window == 1) {
				map_val = 1.0;
				break;
			}
			temp_float = 0.;
			for(ind1 = 0; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 0; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_float_2 = (1.0 * (*(freq_ptr + temp_int)) ) *
					               constants.number_of_edges_inverse;
					if(temp_float_2 > 0) {
						temp_float += temp_float_2 * log(temp_float_2);
					}
				}
			}
			temp_float = -1.0 * temp_float;
			temp_float_2 = 2.0 * log(1.0 * n_colors_in_window);
			temp_float = 1.0 - (temp_float / temp_float_2);
			map_val = temp_float;
			break;
		}
		if(handle_missing == 1) {  /*missing not included*/
			n_colors_in_window = 0;
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = ind1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_int2 = (ind2 * (n_colors_in_image + 1)) + ind1;
					if(  ( (*(freq_ptr + temp_int)) > 0) ||
					     ( (*(freq_ptr + temp_int2)) > 0)) {
						n_colors_in_window++;
					}
				}
			}
			if(n_colors_in_window == 0) {
				map_val = -0.01;
				break;
			}
			if(n_colors_in_window == 1) {
				map_val = 1.0;
				break;
			}
			temp_int2 = 0;  /* will be the divisor */
			/* count the non-missing edges*/
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_int2 +=  (*(freq_ptr + temp_int));
				}
			}
			/* return missing if there are no non-missing edges */
			if(temp_int2 == 0) {
				map_val = -0.01;
				break;
			}
			temp_float = 0.;
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_float_2 = (1.0 * (*(freq_ptr + temp_int)) ) / (1.0 * temp_int2);
					if(temp_float_2 > 0) {
						temp_float = temp_float + (temp_float_2 * log(temp_float_2));
					}
				}
			}
			temp_float = -1.0 * temp_float;
			temp_float_2 = 2.0 * log(1.0 * n_colors_in_window);
			temp_float = 1.0 - (temp_float / temp_float_2);
			map_val = temp_float;
			break;
		}
		break;
	case 74: /* return sum of diagonal of adjacency matrix */
		if(handle_missing == 2) {  /*missing included, again why? */
			temp_int2 = 0;
			/* accumulate frequencies along diagonal in the adjacency matrix */
			for(index = 0; index <= n_colors_in_image; index++) {
				temp_int = (index * (n_colors_in_image + 1)) + index;
				temp_int2 += (*(freq_ptr + temp_int));
			}
			/* and the proportion is... */
			temp_float = (1.0 * temp_int2) * constants.number_of_edges_inverse;
			map_val = temp_float;
			break;
		}
		if(handle_missing == 1) {  /*missing not included*/
			temp_int2 = 0.;  /* will be the divisor */
			/* count all the non-missing edges */
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_int2 += (*(freq_ptr + temp_int));
				}
			}
			/* return missing if there are no non-missing edges */
			if(temp_int2 == 0) {
				map_val = -0.01;
				break;
			}
			temp_float = 0.;
			/* accumulate frequencies along diagonal in the adjacency matrix */
			for(index = 1; index <= n_colors_in_image; index++) {
				temp_int = (index * (n_colors_in_image + 1)) + index;
				temp_float += (1.0 * (*(freq_ptr + temp_int)));
			}
			/* and the proportion is... */
			temp_float = temp_float / (1.0 * temp_int2);
			map_val = temp_float;
			break;
		}
		map_val = -0.01;  /* fall-through indicates error somewhere */
		break;
	case 75: /* return proportion of local edges which involve code_1 */
		if(handle_missing == 2) {  /*missing included*/
			temp_float = 0.;
			/* accumulate across code_1's row in the adjacency matrix */
			for(index = 0; index <= n_colors_in_image; index++) {
				temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + index;
				temp_float += (1.0 * (*(freq_ptr + temp_int)) );
			}
			/* accumulate across code_1's column in the adjacency matrix */
			for(index = 0; index <= n_colors_in_image; index++) {
				temp_int = (index * (n_colors_in_image + 1)) + parameters.code_1;
				temp_float += (1.0 * (*(freq_ptr + temp_int)) );
			}
			/* the diagonal cell has been counted twice, so subtract one of them */
			temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + parameters.code_1;
			temp_float -= (1.0 * (*(freq_ptr + temp_int)) );
			/* and the proportion is... */
			temp_float = temp_float * constants.number_of_edges_inverse;
			map_val = temp_float;
			break;
		}
		if(handle_missing == 1) {  /*missing not included*/
			temp_int2 = 0;  /* will be the divisor */
			/* count all the non-missing edges */
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_int2 += (*(freq_ptr + temp_int));
				}
			}
			/* return missing if there are no non-missing edges */
			if(temp_int2 == 0) {
				map_val = -0.01;
				break;
			}
			/* count all the edges involving code_1 */
			temp_float = 0.;
			/* accumulate across code_1's row in the adjacency matrix */
			for(index = 1; index <= n_colors_in_image; index++) {
				temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + index;
				temp_float += (1.0 * (*(freq_ptr + temp_int)) );
			}
			/* accumulate across code_1's column in the adjacency matrix */
			for(index = 1; index <= n_colors_in_image; index++) {
				temp_int = (index * (n_colors_in_image + 1)) + parameters.code_1;
				temp_float += (1.0 * (*(freq_ptr + temp_int)) );
			}
			/* the diagonal cell has been counted twice, so subtract one of them */
			temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + parameters.code_1;
			temp_float = temp_float - (1.0 * (*(freq_ptr + temp_int)) );
			/* and the proportion is... (the divide by zero check happened above) */
			temp_float =  temp_float / (1.0 * temp_int2);
			map_val = temp_float;
			break;
		}
		break;
	case 76: /* return proportion of edges between code_1 and code_2 */
		temp_float = 0.0;
		temp_float_2 = 0.0;
		if(handle_missing == 2) {  /*missing included*/
			/* sum the i,j and j,i cells */
			temp_int = (parameters.code_1 * (n_colors_in_image + 1)) +
			           parameters.code_2;
			temp_float = (1.0 * (*(freq_ptr + temp_int)) );
			/* don't count a diagonal cell twice if self-joins were requested */
			if(parameters.code_1 != parameters.code_2) {
				temp_int = (parameters.code_2 * (n_colors_in_image + 1)) +
				           parameters.code_1;
				temp_float +=  (1.0 * (*(freq_ptr + temp_int)) );
			}
			/* and the proportion is... */
			temp_float = temp_float * constants.number_of_edges_inverse;
			map_val = temp_float;
			break;
		}
		if(handle_missing == 1) {  /*missing not included*/
			temp_int2 = 0;  /* will be the divisor */
			/* count all the non-missing edges */
			for(ind1 = 1; ind1 <= n_colors_in_image; ind1++) {
				for(ind2 = 1; ind2 <= n_colors_in_image; ind2++) {
					temp_int = (ind1 * (n_colors_in_image + 1)) + ind2;
					temp_int2 += (*(freq_ptr + temp_int));
				}
			}
			/* return missing if there are no non-missing edges */
			if(temp_int2 == 0) {
				map_val = -0.01;
				break;
			}
			/* sum the i,j and j,i cells */
			temp_int = (parameters.code_1 * (n_colors_in_image + 1)) +
			           parameters.code_2;
			temp_float = (1.0 * (*(freq_ptr + temp_int)) );
			/* don't count a diagonal cell twice if self-joins were requested */
			if(parameters.code_1 != parameters.code_2) {
				temp_int = (parameters.code_2 * (n_colors_in_image + 1)) +
				           parameters.code_1;
				temp_float += (1.0 * (*(freq_ptr + temp_int)) );
			}
			/* and the proportion is... (the divide by zero check happened above) */
			temp_float_3 = 1.0 / (1.0 * temp_int2);
			temp_float = temp_float * temp_float_3;
			map_val = temp_float;
			break;
		}
		break;
	case 77:  /* return proportion of edges with a color which are self-joins*/
		start = 1;  /* don't count edges with missing */
		if(handle_missing == 2) {
			start = 0;    /*missing included*/
		}
		temp_float_2 = 0.;  /* will be divisor */
		/* count all the edges involving code_1*/
		/* accumulate across code_1's row in the adjacency matrix */
		for(index = start; index <= n_colors_in_image; index++) {
			temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + index;
			temp_float_2 += (*(freq_ptr + temp_int));
		}
		/* accumulate across code_1's column in the adjacency matrix */
		for(index = start; index <= n_colors_in_image; index++) {
			temp_int = (index * (n_colors_in_image + 1)) + parameters.code_1;
			temp_float_2 += (*(freq_ptr + temp_int));
		}
		/* the diagonal cell has been counted twice, so subtract one of them */
		temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + parameters.code_1;
		temp_float_2 -= (*(freq_ptr + temp_int));
		if(temp_float_2 == 0.) {   /* set to missing if no edges with that color */
			map_val = -0.01;
			break;
		}
		/* and the proportion is... */
		temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + parameters.code_1;
		temp_float = (*(freq_ptr + temp_int));
		temp_float = (1.0 * temp_float) / (1.0 * temp_float_2);
		map_val = temp_float;
		break;
	case 78:  /* added 12/99, return proportion of edges for a color that
                  are joins between that color and another specified color */
		start = 1;  /* don't count edges with missing */
		if(handle_missing == 2) {
			start = 0;    /*missing included*/
		}
		temp_float_2 = 0.;  /* will be divisor, the marginal total for code_1 */
		/* count all the edges involving code_1*/
		/* accumulate across code_1's row in the adjacency matrix */
		for(index = start; index <= n_colors_in_image; index++) {
			temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + index;
			temp_float_2 += (*(freq_ptr + temp_int));
		}
		/* accumulate across code_1's column in the adjacency matrix */
		for(index = start; index <= n_colors_in_image; index++) {
			temp_int = (index * (n_colors_in_image + 1)) + parameters.code_1;
			temp_float_2 += (*(freq_ptr + temp_int));
		}
		/* the diagonal cell has been counted twice, so subtract one of them */
		temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + parameters.code_1;
		temp_float_2 -= (*(freq_ptr + temp_int));
		if(temp_float_2 == 0.) {   /* set to missing if no edges with that color */
			map_val = -0.01;
			break;
		}
		/* Now find the number of joins between code_1 and code_2 */
		temp_float_3 = 0.0;
		/* get the row entry */
		temp_int = (parameters.code_1 * (n_colors_in_image + 1)) + parameters.code_2;
		temp_float_3 +=  (*(freq_ptr + temp_int));
		/* get the column entry and add it */
		temp_int =(parameters.code_2 * (n_colors_in_image + 1)) + parameters.code_1;
		temp_float_3 += (*(freq_ptr + temp_int));
		/* and the proportion is... */
		temp_float = (1.0 * temp_float_3) / (1.0 * temp_float_2);
		map_val = temp_float;
		break;
	case 81: /* return local density of one particular color (code_1) */
		if(handle_missing == 2) { /*missing included*/
			temp_float = (*(freq_ptr + parameters.code_1)) *
			             constants.window_area_inverse;
		}
		if(handle_missing == 1) {   /* missing not included */
			temp_float_2 = 0.;
			for(index = 1; index < (n_colors_in_image + 1); index++) {
				temp_float_2 +=  (*(freq_ptr + index));
			}
			if(temp_float_2 > 0) {
				temp_float = (*(freq_ptr + parameters.code_1)) / temp_float_2;
			} else { /* no non-missing pixels, set to missing */
				map_val = -0.01;
				break;
			}
		}
		map_val = temp_float;
		break;
	case 83:  /* return the ratio #code1 / (#code1 + #code2) */
		temp_float_2 = (*(freq_ptr + parameters.code_1)) +
		               (*(freq_ptr + parameters.code_2));
		if(temp_float_2 == 0) {
			map_val = -0.01;
			break;
		}
		temp_float =  (*(freq_ptr + parameters.code_1)) / temp_float_2;
		map_val = temp_float;
		break;
	default:
		/* do nothing */
		break;
	}
	return(map_val);
}
