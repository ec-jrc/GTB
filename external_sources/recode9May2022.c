/* ************************************************************************ 
  RECODE.C   recode the byte values in a file.
Version 1. March 2014 for use with Guidos platform.
revised May 2022 to do row-by-row processing instead of the entire image, to save ram.   
************************************************************************ */
/*  
 * Guidos Mode Usage: recode
 *   The program looks in the current directory for:
		an 8bit "bsq" file called recinput.
		a recode text file called recode.txt
		a size text file called recsize.txt
 *  The program writes an 8bit "bsq" file called recoutput in the current directory.
 * 
 * Format of recode.txt:
 * old_value new_value
 * old_value new_value
 * etc, for the old_values to be recoded.
 * old_values that are not listed will stay the same.
 * All old and new values must be in the range [0,255]
 * The values can appear in any order
 * If old_values are duplicated then the last one listed will be used.
 * 
 * Format of recsize.txt:
 * nrows XXXX  (where XXX is a number) 
 * ncols XXXX  (nrows must be first line, ncols second line)

gcc -std=c99 -m64 -O2 -Wall -fopenmp recode.c  -o recode
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(__APPLE__)
#include <malloc.h>
#endif
#include  <omp.h>

int main(int argc, char **argv)
{
        static long int vnew, vold, index, dum, nrows_in, ncols_in, row, col, temp_int, recode_table[256];
        static FILE *infile, *outfile, *recfile, *sizfile;
        static char filename_in[100], filename_out[100], filename_rec[100], filename_siz[100], dummy[30];
		unsigned char *mat_in;
		
		setbuf(stdout, NULL);
        strcpy(filename_rec, "recode.txt");
		strcpy(filename_siz, "recsize.txt");
		strcpy(filename_out, "recoutput");
		strcpy(filename_in, "recinput");
		
        /* Seed the recode table with identity values */
		for(index = 0; index < 256; index ++){
			recode_table[index] = index;
		}
		if( (recfile = fopen(filename_rec, "r") ) == NULL) {
			printf("\nRecode:Error opening %s\n", filename_rec); exit(5);
		}
		printf("Pixels are being recoded using recode file %s.\n", filename_rec);
		printf("   Old code ---> New code\n");
		/* read the recode file and overwrite the recode table as indicated */
		while(fscanf(recfile, "%ld %ld", &vold, &vnew) == 2){
			recode_table[vold] = vnew;
			printf("   %8ld ---> %3ld\n", vold, vnew);
		}
		fclose(recfile);
        /* Open the input and output files */
        if( (infile = fopen(filename_in, "rb") ) == NULL) {
                printf("\nRecode:Error opening %s\n", filename_in); exit(4);
        }
        if( (outfile = fopen(filename_out, "wb") ) == NULL) {
                printf("\nRecode:Error opening %s\n", filename_out); exit(3);
        }
        /* Read the siz file and save nrows and ncols */
        if( (sizfile = fopen(filename_siz, "r") ) == NULL) {
                printf("\nRecode:Error opening %s\n", filename_siz); fclose(infile); fclose(outfile); exit(7);
        }
		fscanf(sizfile,"%s %ld", dummy, &nrows_in); 
		fscanf(sizfile,"%s %ld", dummy, &ncols_in);
		fclose(sizfile);
        printf("Reading %ld columns and %ld rows from file %s.\n", ncols_in, nrows_in, filename_in);
        /* Allocate the resources for mat_in */
		temp_int = ncols_in;
        if( (mat_in = (unsigned char *)calloc( temp_int, sizeof(unsigned char) ) ) == NULL ) {
                printf("\nRecode:malloc failed, matrix_in.\n");
                fclose(infile); fclose(outfile); exit(2);
        }
		omp_set_num_threads(omp_get_max_threads());
    /* recode the pixels here */
		for (row=0; row < nrows_in; row++) {
			if(fread(mat_in, 1, ncols_in, infile) != ncols_in ) {
				printf("\nRecode: error reading input bsq.\n"); exit(8);
			}
 // OMP
 #pragma omp parallel  for  	 private ( col, dum)
			for (col=0; col < ncols_in; col++) {
				dum = *(mat_in + col);
				*(mat_in + col) = recode_table[dum];
			} 
// end of parallel for cols
			if(fwrite(mat_in, 1, ncols_in, outfile) != ncols_in ) { 
				printf("\nRecode: error writing output file.\n"); exit(8);
			}
		}
		printf("File written OK.\n");
		/* Exit nicely */
		fclose(outfile);
		free(mat_in);
		printf("Recode:Normal Finish.\n");
		exit(0);
}


