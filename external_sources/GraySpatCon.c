/* 
************************************************************************
	GraySpatCon.c 
	Spatial convolution of a gray-scale input map, supporting dichotomous, nominal, and ordinal input data.
	Kurt Riitters
	Version 1.2.1, July 2023
************************************************************************ 
DISCLAIMER:
The author(s), their employer(s), the archive host(s), nor any part of the United States federal government
can assure the reliability or suitability of this software for a particular purpose. The act of distribution 
shall not constitute any such warranty, and no responsibility is assumed for a user's application of this 
software or related materials.
************************************************************************
Compiling and environment
linux/macOS:
	gcc -std=c99 -m64 -O2 -Wall -fopenmp GraySpatCon.c -o GraySpatCon -lm 
mingw64:
	gcc -std=c99 -m64 -O2 -Wall -fopenmp GraySpatCon.c -o GraySpatCon -lm -D__USE_MINGW_ANSI_STDIO
 
Optional: 
	Specify the number of cores to use. Defaults to use the maximum available in the shell.
    Linux: export OMP_NUM_THREADS=N (*csh shells: setenv OMP_NUM_THREADS N)
    Windows: set OMP_NUM_THREADS=N

************************************************************************
Usage: ./GraySpatCon (linux/macOS) or GraySpatCon.exe (windows)
	
	Required input files (must be in current directory; case sensitive):
		gscpars.txt - parameter file 
		gscinput - input image file 
			8-bit unsigned integer normalized to [0,100]
			data values in [0, 100], missing value = 255
			
	Parameter file contents: 
		Required:
			R x - number of rows in input image (x is positive integer)
			C x - number of columns in input image (x is positive integer)
			F x - output image precision (1 = 8-bit, 2 = 32-bit float)
					F must = 2 for metrics 44, 45, 50. 
			M x - metric selection, see list below
			G x - analysis type: 0 = moving window analysis; 1 = global (entire map extent) analysis
		Optional:
			P x - exclude input pixels with value zero (0 = no, 1 = yes)
		Required if G = 0:
			W x - window size; the number of pixels on the side of a x*x window (eg x=5 for 5x5 window)
				Must be odd, positive integer > 1 (eg, 3,5,7,9...)
		Optional if G = 0:
			A x - mask missing: 0 - do not mask input missing on output; 1 - set missing input pixels to missing output pixels
		Required if F = 1:
			B x - byte stretch if converting to bytes.
				For metrics bounded in [0.0, 1.0] only: (metrics 2, 3, 6, 8, 10-15, 20-24, 31-38, 40, 42, 49)
					1.  From metric value in [0.0, 1.0] to byte in [0, 100]
					2.  From metric value in [0.0, 1.0] to byte in [0, 254]
				For all metrics except 1, 9, 16, 17, 18, 19, 25, 27:
					3.  From metric value in [Min, Max] to byte in [0, 254]
					4.  From metric value in [0.0, Max] to byte in [0, 254]
					5.  From metric value in [0.0, Max] to byte in [0, 100]
				For metrics 1, 9, 16, 17, 18, 19, 25, 27 only:
					6.  No stretch allowed; the metric value is converted to byte
		Required for metrics 21, 22, 23, 24:
			X x - target code 1 (t1) (x  in [0,100]). 
		Required for metrics 23, 24:
			Y x - target code 2 (t2) (x  in [0,100]). 
		Required for metric 49:
			K x - target difference level (k*) ( x in [0, 100]).
			
		Parameters can appear in any order. 
		Parameters not used in a given run are ignored.
		Example gscpars.txt:
		R 1289
		C 1990
		F 2
		M 13
		G 0
		P 0
		W 11
		A 1
		B 1 
		X 88
		Y 87
		K 5
		
	Output file (written in the current directory):
		For G = 0 (image output):
			Output file name = gscoutput
			Missing value = -0.01 (32-bit float file) or 255 (8-bit byte file).
				Exception: for metrics 44, 45, 50 the missing value = -9000000.0
		For G =1 (text output):
			Output file name = gscoutput.txt
			Missing value = -0.01 
				Exception: for metrics 44, 45, 50 the missing value = -9000000.0
*/			
/* Change log
************************************************************************
Version and changes:
0.9.0 October 2021
		Revise spatcon.c for continuous input data, testing John's new metric.
0.9.1 March 2022
		Revising for John's paper; realization that more metrics need to be tested and compared and generic code is needed.
0.9.2 December 2022
		Massive revision, starting with spatcon.c again
		implement spatcon metrics and add new gray-level metrics for ordinal data
		improve program structure and use of functions
		add global analysis option
		changed the various 'evenness' indices to yield missing when the number of classes = 1. The rationale is that
		evenness = obs evenness / max evenness, so the question is what to do with n=1 because max evenness is undefined when n=1,
		that is, you get a divide by zero error. Before (spatcon) evenness was assigned highest (1) when diversity or entropy were lowest (0).
		New thinking: when diversity or entropy are zero, then the fraction evaluates to zero because the numerator is zero. It makes no sense	
		to have a metric that could arguably be either zero or one, and the cleanest solution is to make it missing when n=1.
1.0.0 February 2023
		first public release
1.1.0 April 2023 
		Minor revisions to support documentation and distribution in GuidosToolBox (GTB) and GuidosToolbox Workbench (GWB).
1.1.1 April 2023
		Fixed bug in for metric 24. Failure to check for divide by zero error.
		Edited run-time console messages listing the parameter values.
		Fixed metrics 25, 26, 50, 51 which were incorrectly calculating the standard deviation.
		Deleted three un-used subroutines at the end of the code.
		For the metrics which are calculated as floating point values, the output values are rounded to 5 decimal digits for float output,
		and conversions to byte values use the rounded values. This is also done when comparing intermediate values to the value 0.0
		for metrics 26, 44, 50, 51.
1.2.0 June 2023
		Added metric 52 "clustering" from Peter Vogt
1.2.1 July 2023
		Fixed bug in routine Global_Analysis. The missing adjacencies were not tallied, resulting in incorrect calculation of adjacency
		metrics in Metric_Calculator (which calculates non-missing adjacencies by subtracting the missing adjacencies). The equivalent
		code in Moving_Window requires no changes.
************************************************************************
 */
 /* Programming notes
1. Here is the magic to access a 1-D array as a 2-D array, using [row][col] syntax.
	Example. for unsigned char *mat_in_byte of size nrows*ncols, and want to access as Matrix_1[r][c], declare
	unsigned char (*Matrix_1)[ncols] = (unsigned char (*)[ncols])mat_in_byte;
	This casting requires the same typedef on both sides
	Then Matrix_1[r][c] is the same as (*(mat_in_byte + (r*ncols) + c))
	Reference: https://stackoverflow.com/questions/32317605/treating-one-dimensional-array-as-two-dimensional-at-run-time

2. We use the population standard deviation because there are no other pixels in a window to sample,
	and the inference is about the window. Furthermore, many metrics use ONLY the population N, so it's more comparable across metrics.
	
3. Adding a metric. All of the metrics are calculated from the frequencies of different gray levels and/or the frequencies
	of different types of gray level adjacencies. In the simplest scenario, adding a metric involves adding a new case (i.e., a new metric)
	in the function Metric_Calculator, and setting appropriate "control" parameters for that metric by adding a new case within the function
	Check_Parameters_Set_Controls.
	
	The missing code for the input data (255) is reset internally to 101.
	
	Two pointers are passed to Metric_Calculator. For details on their usage refer to the examples of existing metrics. Briefly:
	The first (long int *freqptr1) is a one-dimensional, 102-element array of the frequency of pixels by gray level for levels (0, 1, 2...100) plus an additional level (101) for the frequency of missing pixels.
	The second (long int *freqptr2) is a two-dimensional, 102x102 array of the frequency of gray level adjacencies (101x101) with an additional row and
	additional column for the frequencies of missing adjacencies (i.e., 101,j or i,101).
	
	In the simplest scenario, only three control parameters are needed for a new metric:
	control.freq_type;			// 1 - count pixels, 2 - count adjacencies, 3 - count both pixels and adjacencies
	control.bounded;			// 1 - no, 2 - yes; is the potential range of the metric bounded in [0,1]; used in byte conversion
	control.no_stretch;			// 0 - ok to stretch, 1 - not ok to stretch
	In addition, use this for metrics which can have negative values (and set missing values to -9000000.0 in the code)
	control.negative_flag;		// 1 - metric can have negative values, (default is 0)

	For non-simple metrics it may be necessary to also add elements to the "control" structure, and to set and error-check the new parameters
	for consistency with overall program flow in function Check_Parameters_Set_Controls.

4. GraySpatCon.c uses a moving window algorithm developed for spatcon.c in 1994. It is efficient because it processes images by row,
	with accumulators accounting for the changing contents of a window as it moves along a row, and (after 2010) because the rows are processed 
	in parallel thanks to gomp. For 20 years spatcon.c was not documented or distributed prior to its inclusion in the GuidosToolBox and the
	GuidosToolbox Workbench (https://github.com/ec-jrc/GWB/blob/main/tools/external_sources/SPATCON_Guide.pdf). As a research tool, it was
	the computing engine for most of the author's papers (and for many papers published by collaborators) since 1997. Spatcon.c was designed for 
	processing dichotomous or nominal input images. GraySpatCon.c was motivated to incorporate additional metrics for ordinal image data (e.g., 
	quantized gray-level maps).
	
5. The moving window uses a 2-neighbor rule (one pixel to the right and one pixel below) such that each adjacency is counted once. This is conceptually
	similar to counting each pixel once for frequency-based metrics, which simplifies conceptual extension of frequency-based metrics to adjacency-based
	metrics. While single-counting preserves pixel order in adjacencies, it can be removed by collapsing the ordered adjacency matrix across the main diagonal. Many metrics are invariant with respect to pixel order, but importantly others are not, and furthermore double-counting adjacencies 
	(e.g., a 4-neighbor rule) violates underlying assumptions for some metrics. Thus, GraySpatCon.c offers two versions of some metrics for the 
	ordered and unordered cases.
************************************************************************
*/
 /* Metrics
 
 The first line shows the metric number, short name, and long name corresponding to the external documentation.
 Subsequent lines, if any, show the internal names and other information for metrics.
 
1	Mean. Mean pixel value.
2	EvennessOrderedAdj. Evenness (ordered adjacencies)
		Shannon evenness of adjacency matrix, with regard to pixel order in adjacencies
		complement of Equation 21 (using equation 12) in Li & Reynolds 1993; DOI: 10.1007/BF00125347
3	EvennessUnorderedAdj. Evenness (unordered adjacencies)
		Shannon evenness of adjacency matrix, without regard to pixel order in adjacencies
		complement of Equation 6 in Riitters et al 1996; DOI: 10.1007/BF02071810
4	EntropyOrderedAdj. Entropy (ordered adjacencies)
		Shannon entropy of adjacency matrix, with regard to pixel order in adjacencies
		Haralick 1973 and Lofstedt call this "HXY"
5	EntropyUnorderedAdj. Entropy (unordered adjacencies)
		Shannon entropy of adjacency matrix, without regard to pixel order in adjacencies
6	DiagonalContagion. Diagonal contagion
		Sum of diagonals contagion of adjacency matrix.
		SUMD metric in Riitters et al 1995. DOI: 10.1007/BF00158551
		This is Lofstedt's Px-y(k), for the case of k=0
7	ShannonDiversity. Shannon diversity
		Shannon entropy of pixel values
8	ShannonEvenness. Shannon evenness
		Shannon evenness of pixel values
9	Median. Median
		Median pixel value
10	GSDiversity. Gini-Simpson diversity
		Gini-Simpson diversity of pixel values
11	GSEvenness. Gini-Simpson evenness
		Gini-Simpson evenness of pixel values
		Equation 4 in Wickham & Riitters 1995; DOI: 10.1080/01431169508954647
12	EquitabilityOrderedAdj. Equitability (ordered adjacencies)
		Gini-Simpson evenness of adjacency matrix, with regard to pixel order in adjacencies
		related to Equation 6 in Wickham & Riitters 1995; DOI: 10.1080/01431169508954647
13	EquitabilityUnorderedAdj. Equitability (unordered adjacencies)
		Gini-Simpson evenness of adjacency matrix, without regard to pixel order in adjacencies
		Apparently un-published metric due to novel calculation of maximum possible value
14	DiversityOrderedAdj. Diversity (ordered adjacencies)
		Gini-Simpson diversity of adjacency matrix, with regard to pixel order in adjacencies
		G-S diversity is 1 minus angular second moment ( ASM is equation 1 in appendix 1 of Haralick 1973)
15	DiversityUnorderedAdj. Diversity (unordered adjacencies)
		Gini-Simpson diversity of adjacency matrix, without regard to pixel order in adjacencies
16	Majority. Majority
		Majority pixel value
17	LandscapeMosaic19. Landscape mosaic (19 classes)
18	LandscapeMosaic103. Landscape mosaic (103 classes)
19	NumberGrayLevels. Number of gray levels values
20	MaxAreaDensity. Maximum area density
		Dominance_pixel (Max(Pi))
21	FocalAreaDensity. Focal area density (t1)
		Area density as in spatcon "Px"
22	FocalAdjT1. Focal adjacency (t1)
		Focal adjacency_1: Proportion of total adjacencies, which involve one specific pixel value.
23	FocalAdjT1andT2. Focal adjacency (t1 and t2)
		Focal adjacency_2: Proportion of total adjacencies, which involve two specific pixel values.
24	FocalAdjT1givenT2. Focal adjacency (t2 given t1)
		Focal adjacency_3: Proportion of total adjacencies for one specific pixel value, which involve a second specific pixel value (spatcon "Pxx" and "Pxy"). 
25	StandardDeviation. Standard deviation
		Population standard deviation of pixel values
26	CoefficientVariation. Coefficient of variation
		Population coefficient of variation of pixel values
27	Range. Range
		Range of pixel values (max - min)
28	Dissimilarity. Dissimilarity
		Soh (1999). Sum( Pij * |i-j| ). Indept of pixel order
29	Contrast. Contrast
		Haralick (1973), computing formula in Lofstedt. Sum( Pij * ( (i-j)**2 ) ). Indept of pixel order
30	UniformityOrderedAdj. Uniformity (ordered adjacencies)
		Uniformity of adjacency matrix, with regard to pixel order in adjacencies
		Haralick (1979). AKA angular second moment (Haralick 1973), and energy (Haralick 1979)
		Uniformity equals G-S diversity minus 1
31	UniformityUnorderedAdj. Uniformity (unordered adjacencies)
		Uniformity of adjacency matrix, without regard to pixel order in adjacencies
32	Homogeneity. Homogeneity
		Haralick (1973, there and in Clausi 2002 called "inverse difference moment", but it's not the same as inverse difference
		moment in Haralick 1979. First called homogeneity by Soh 1999 and it seems to be a misnomer)
		Computing formula in Lofstedt. Indept of pixel order. Homogeneity = sum Pij/(1+(i-j)**2). 
33	InverseDifference. Inverse difference
		Clausi (2002). Computing formula in Lofstedt. Indept of pixel order. Inverse difference = sum Pij/(1+|i-j|)
34	SimilarityRMax. Similarity (R = 100)
		Similarity_1.
		Gower (1971). John's original metric. Indept of pixel order. Complement of normalized dissimilarity.
		Similarity_1 = 1 - sum( Pij * { |i-j| / R1 } )	where R1 = 100 is the maximum possible range
35	SimilarityRGlobal. Similarity (R = global range)
		Similarity_2.
		Gower (1971). Indept of pixel order.
		Similarity_2 = 1 - sum( Pij * { |i-j| / R2 } )	where R2 is the observed range in the entire image
36	SimilarityRWindow. Similarity (R = window range)
		Similarity_3
		Gower (1971). Indept of pixel order.
		Similarity_3 = 1 - sum( Pij * { |i-j| / R3 } )	where R3 is the observed range in the window
37	DominanceOrderedAdj. Dominance (ordered adjacencies)
		Dominance_adjacency, with regard to pixel order
		= max(Pij). This is "maximum probability" in Haralick (1979). Lofstedt cites Soh for it.
38	DominanceUnorderedAdj. Dominance (unordered adjacencies)
		Dominance_adjacency, without regard to pixel order
39	DifferenceEntropy. Difference entropy
		Apparently somewhat new. Haralick 1973 and Lofstedt ignore zero pixel value so that formula is only for 'excluding zero pixels'
		Modify Lofstedt equations to include 0. Indept of pixel order
40	DifferenceEvenness. Difference evenness
		Apparently a new metric. Indept of pixel order. Scales entropy to obs/max in [0,1], by analogy to evenness of pixel values.
		This is an alternative to Lofstedt's scale-invariant version of difference entropy
41	SumEntropy. Sum entropy
		apparently somewhat new. Similar to Haralick 73 but includes 0.  modify Lofstedt equations.
42	SumEvenness. Sum evenness
		apparently new. This is an alternative to Lofstedt's scale-invariant version of sum entropy
43	AutoCorrelation. Autocorrelation
		Soh (1999). indept of pixel order. Sum( Pij * (i*j). NOTE: include/exclude 0 is irrelevant because i*0 = 0*j = 0
44	Correlation. Correlation
		Correlation.  Missing value is -9000000.0
		Haralick 1973. Pixel order irrelevant. Use Lofstedt computing formula (Haralick's and Soh's make no sense)
45	ClusterShade. Cluster shade
		Cluster shade. Missing value is -9000000.0
		Formula in Soh 1999 attributes it to Conners 1982.  Don't use Lofstedt formula; it is for symmetric GLCMs
46	ClusterProminence. Cluster prominence
		Formula in Soh 1999 attributes it to Conners 1982.  Don't use Lofstedt formula; it is for symmetric GLCMs
47	RootMeanSquare. Root mean square
		Root mean square of pixel values
		This is RMS or quadratic mean in the statistics world.
		The mean-adjusted version of this is the metric "Population standard deviation of pixel values"
48	AverageAbsDeviation. Average absolute deviation
		Average absolute deviation of pixel values, including 0
49	kContagion. k-contagion
		new metric aka k-attraction
50.	Skewness. Skewness
	skewness of pixel values
51.	Kurtosis. Kurtosis
	kurtosis of pixel values
52. Clustering. Clustering
		Peter Vogt's clustering metric.
************************************************************************
 */
// libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(__APPLE__)
#include <malloc.h>
#endif
#include <math.h>
#include <omp.h>
// ***** prototypes *****
long int Read_Parameter_File(FILE *);
long int Check_Parameters_Set_Controls();
long int Check_Input_Data ();
long int Buffer_Data();
long int Moving_Window();
float Metric_Calculator(long int *, long int *);
long int Unbuffer_Data();
long int Float2Byte();
float Global_Analysis();
long int Global_Range(long int);
// ***** structures *****
// input parameters
struct input_parameters {	// from file parameters.txt
	long int nrows;			// number of rows in input image
	long int ncols;			// number of columns in input image
	long int metric;		// metric number to be calculated
	long int data_type;		// specifies (1) byte or (2) float output (actual floats are converted to ordinal bytes)
	long int window_size;	// side length of window, number of pixels
	long int byte_stretch;	//{ how to strech when converting float to byte:
							// 1: From metric value in [0.0, 1.0] to byte in [0, 100]
							// 2: From metric value in [0.0, 1.0] to byte in [0, 254]
							// 3. From metric value in [Min, Max] to byte in [0, 254] (for Min>=0)
							// 4. From metric value in [0.0, Max] to byte in [0, 254]
							// 5. From metric value in [0.0, Max] to byte in [0, 100]
							// 6. No stretch: For several metrics, the metric value is converted to byte with no stretch allowed
							//}
	long int target_code_1;	// used to specify particular byte value for some metrics
	long int target_code_2;	// used to specify particular byte value for some metrics
	long int global; 		// used to select moving window (0) or global (1) analysis
	long int mask_missing;	// 1=don't mask. 2=mask, that is, any missing input pixel becomes missing output pixel
	long int exclude_zero;	// 0 include pixel value zero, 1 = exclude pixel value zero
	long int k_value; 		// used to specify particular k value for some metrics
};                    
struct input_parameters parameters = {0,0,0,0,0,0,255,255,0,0,0,255};
// control parameters
struct control_parameters { 	// these are set in the code, many depend on the metric
	long int npix_window;		// the number of pixels in a window (e.g. window size = 5 --> 25 pixels
	long int nadj_window;		// the number of adjacencies in a square window, 2*(nrows-1)*ncols
	long int npix_map;			// the number of pixels = nrows*ncols
	long int nadj_map;			// the number of adjacencies = ((nrows-1)*ncols) + ((ncols-1)*nrows)
	long int freq_type;			// 1 - count pixels, 2 - count adjacencies, 3 - count both pixels and adjacencies
	long int bounded;			// 1 - no, 2 - yes; is the potential range of the metric bounded in [0,1]; used in byte conversion
	long int LM_flag;			// 0 (default) - not LM metric, 1 - LM metric
	long int no_stretch;		// 0 (default) - ok to stretch, 1 - not ok to stretch
	long int target1_flag;		// parameter file must specify target code 1.  0 - no (default), 1 - yes
	long int target2_flag;		// parameter file must specify target code 2.  0 - no (default), 1 - yes
	long int zero_is_missing;	// to label the metrics which exclude zero as missing 0 - no (default), 1 - yes
	long int range_flag;		// 0 (default) - don't calculate global range, 1 - calculate global range
	long int global_range;		// for some metrics, stores the observed global range of pixel values on the input image
	long int negative_flag;		// = 1 for metrics which can have negative value. For these, only float output is available, and missing = -9000000.0
	long int k_diff_flag;		// parameter file must specify k value for the difference function. 0 - no (default), 1 - yes
};                    
struct control_parameters control = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
// ***** global variables *****
// input, buffered input, and output data arrays
unsigned char *mat_in_byte;  	// input byte values
unsigned char *mat_out_byte;  	// optional output byte values
unsigned char *mat_temp; 		// buffered copy of input byte values
float *mat_out; 				// default - metrics are stored as float but may be converted to byte on output file
// ***** Main *****
int main(int argc, char **argv)
{
	static long int ret_val, nrows_in, ncols_in, buff_b, n_cols_buf, n_rows_buf, temp_int, row, col, index;
	static FILE *infile, *parfile, *outfile;
	static char filename_par[20], filename_in[20], filename_out[20];
	float metric_value, missing;
	unsigned char *datarow;
	setbuf(stdout, NULL);	
	printf("\nGraySpatCon: Start.");
// process parameter file
	strcpy(filename_par,"gscpars.txt");
	if( (parfile = fopen(filename_par, "r") ) == NULL) { 
		printf("\nGraySpatCon: Error opening parameter file %s.\n", filename_par); exit(11);
	}
	ret_val = Read_Parameter_File(parfile);
	if(ret_val != 0) { 
		printf("\nGraySpatCon: Error -- Parameter file %s is not in correct format.\n", filename_par); exit(12);
	}
	fclose(parfile);
	printf("\nGraySpatCon: Run parameters: (All possible parameters are listed; un-used parameters are ignored.)\n R = %ld (number of rows in input image)\n C = %ld (number of columns in input image)\n M = %ld (metric number)\n P = %ld (exclude zero pixel values in input file: 0 = no, 1 = yes)\n G = %ld (analysis type: 0 = moving window & image output; 1 = global extent & text output)\n W = %ld (if moving window, window size = W x W)\n F = %ld (if moving window, 1 = byte output, 2 = float output)\n B = %ld (if byte output, metric stretch option)\n A = %ld (if moving window, 0 = no output masking, 1 = force missing output for missing input pixels)\n X = %ld (if used, target code 1)\n Y = %ld (if used, target code 2)\n K = %ld (if used, selected k value)", parameters.nrows, parameters.ncols, parameters.metric, parameters.exclude_zero, parameters.global, parameters.window_size, parameters.data_type, parameters.byte_stretch, parameters.mask_missing, parameters.target_code_1, parameters.target_code_2, parameters.k_value);
// check parameters, set control variables, and do some other checking 
	printf("\nGraySpatCon: Checking parameters.");
	ret_val = Check_Parameters_Set_Controls();
	if(ret_val != 0) { 
		printf("\nGraySpatCon: Error -- One or more parameter value is not valid; check parameter file.\n"); exit(12);
	}
// open the I/O files
	strcpy(filename_in, "gscinput");
	strcpy(filename_out, "gscoutput");
	if( (infile = fopen(filename_in, "rb") ) == NULL) { 
		printf("\nGraySpatCon: Error -- Error opening input file %s.\n", filename_in); exit(15);
 	}
	if(parameters.global == 0){
		if( (outfile = fopen(filename_out, "wb") ) == NULL) { 
			printf("\nGraySpatCon: Error -- Error opening output file %s.\n", filename_out); exit(16);
		}
	}
	if(parameters.global == 1){
		strcat(filename_out, ".txt");
		if( (outfile = fopen(filename_out, "w") ) == NULL) { 
			printf("\nGraySpatCon: Error -- Error opening output file %s.\n", filename_out); exit(16);
		}
	}
	printf("\nGraySpatCon: Input file is %s. Output file is %s.", filename_in, filename_out);
// set omp numthreads and report the number of cores available
    omp_set_num_threads(omp_get_max_threads());
    printf("\nGraySpatCon: OpenMP will use %d core(s).", omp_get_max_threads());
// allocate input image memory, read input data 
	printf("\nGraySpatCon: Allocating memory for input data.");
	nrows_in = parameters.nrows;
	ncols_in = parameters.ncols;
	temp_int = nrows_in * ncols_in;
	if( (mat_in_byte = (unsigned char *)calloc( temp_int, sizeof(unsigned char) ) ) == NULL ) {
		printf("\nGraySpatCon: Error -- Cannot allocate memory for input data.\n"); exit(19); 
	}
	if(fread(mat_in_byte, sizeof(unsigned char), temp_int, infile) != temp_int ) {
		printf("\nGraySpatCon: Error -- Cannot read %ld byte values from input file %s.\n", temp_int, filename_in); exit(20);
	}
	printf("\nGraySpatCon: Input file read OK.");
	fclose(infile);
	// Check input data(this function changes missing from 255 to 101)
	printf("\nGraySpatCon: Checking input data.");
	ret_val = Check_Input_Data();
	if(ret_val == 1) { 
		printf("\nGraySpatCon: Error -- input data values must be bytes in range [0,100] or 255 for missing.\n"); exit(12);
	}
	if(ret_val == 2) { 
		printf("\nGraySpatCon: Error -- for landscape mosaic metric, input data values must be bytes in range [1,3] or 255 for missing.\n"); exit(15);
	}
// If needed, get the global range before deleting the input image
	if(control.range_flag == 1) {
		if(control.zero_is_missing == 0) { 
			control.global_range = Global_Range(1);
			if(control.global_range == 0){printf("\nGraySpatCon: Warning - global range is zero; output pixels may be 'missing'.");}
		}
		if(control.zero_is_missing == 1) {
			control.global_range = Global_Range(2);
			if(control.global_range == 0){printf("\nGraySpatCon: Warning - global range is zero; output pixels may be 'missing'.");}
		}
	}
// Code block for moving window analyis
	if(parameters.global == 0){
		printf("\nGraySpatCon: Conducting a moving window analysis.");
		// Create mat_temp as buffered input data
		printf("\nGraySpatCon: Making a temporary buffered copy of the input data.");
		// allocate memory
		buff_b = (parameters.window_size - 1) / 2;  // one side of image 
		n_cols_buf = ncols_in + (2 * buff_b);		// buffered image dimensions
		n_rows_buf = nrows_in + (2 * buff_b);
		temp_int = n_cols_buf * n_rows_buf;
		if( (mat_temp = (unsigned char *)calloc( temp_int, sizeof(unsigned char) ) ) == NULL ) {
			printf("\nGraySpatCon: Error -- Cannot allocate memory for buffered copy of input data.\n"); exit(25);
		}
		ret_val = Buffer_Data();
		if(ret_val != 0) { 
			printf("\nGraySpatCon: Error -- Function Buffer_Data failed to complete.\n"); exit(12);
		}
		// free the memory for input image
		printf("\nGraySpatCon: Releasing memory for original input data.");
		free(mat_in_byte);
		// allocate memory for output image
		printf("\nGraySpatCon: Allocating memory for output data.");
		temp_int = n_cols_buf * n_rows_buf;
		if( (mat_out = (float *)calloc( temp_int, sizeof(float) ) ) == NULL ) {
			printf("\nGraySpatCon: Error -- Cannot allocate memory for output data.\n"); exit(19); 
		}
		// start the convolution,  Moving_Window calls Metric_Calculator and stores metric values in *mat_out
		printf("\nGraySpatCon: Starting image convolution.");
		ret_val = Moving_Window();
		if(ret_val !=0) { 
			printf("\nGraySpatCon: Error -- unspecified failure of convolution.\n"); exit(22);
		}
		printf("\nGraySpatCon: Convolution completed.");
		printf("\nGraySpatCon: Releasing memory for temporary copy of input data.");
		free(mat_temp);
		// Remove the buffer area from the output image
		printf("\nGraySpatCon: Removing buffer from output data.");
		ret_val = Unbuffer_Data();
		if(ret_val != 0) { 
			printf("\nGraySpatCon: Error -- Function Unbuffer_Data failed to complete.\n"); exit(12);
		}
		// optional masking of missing input pixels
		if(parameters.mask_missing == 1){
			printf("\nGraySpatCon: Masking the input image missing pixels.");
			// re-open the input image and malloc a row of pixels
			if( (infile = fopen(filename_in, "rb") ) == NULL) { 
				printf("\nGraySpatCon: Error -- Error re-opening input file %s.\n", filename_in); exit(15);
			}
			if( (datarow = (unsigned char *)calloc( parameters.ncols, sizeof(unsigned char) ) ) == NULL ) {
				printf("\nGraySpatCon: Error -- Cannot allocate memory for masking input data.\n"); exit(25);
			}
			// handle special case of negative metric values, e.g., correlation metric
			missing = -0.01;
			if(control.negative_flag == 1){
				missing = -9000000.0;
			}
			for(row = 0; row < parameters.nrows; row++){
				if(fread(datarow, sizeof(unsigned char), parameters.ncols, infile) != parameters.ncols ) {
					printf("\nGraySpatCon: Error -- Cannot read from input file %s when masking.\n", filename_in); exit(20);
				}
				index = row * parameters.ncols;
				for(col = 0; col < parameters.ncols; col++){
					temp_int = (*(datarow + col));
					if(temp_int == 255){
						(*(mat_out + index + col)) = missing;
					}
				}
			}
			free(datarow);
			fclose(infile);
		}	
		// optional conversion to byte values
		if(parameters.data_type == 1) {
			printf("\nGraySpatCon: Converting float to byte output.");
			nrows_in = parameters.nrows;
			ncols_in = parameters.ncols;
			temp_int = nrows_in * ncols_in;
			printf("\nGraySpatCon: Allocating memory for byte output data.");
			if( (mat_out_byte = (unsigned char *)calloc( temp_int, sizeof(unsigned char) ) ) == NULL ) {
				printf("\nGraySpatCon: Error -- Cannot allocate memory for output byte data.\n"); exit(19); 
			}
			ret_val = Float2Byte();
			if(ret_val != 0) { 
				printf("\nGraySpatCon: Error -- Error converting float output to byte output.\n"); exit(12);
			}
			// write the output bytes
			printf("\nGraySpatCon: Writing byte output to disk.");
			temp_int = nrows_in * ncols_in;
			if(fwrite(mat_out_byte, 1, temp_int, outfile) != temp_int ) {
				printf("\nGraySpatCon: Error writing output file.\n"); exit(24);
			}
		}
		if(parameters.data_type == 2) {
			// write the output floats
			printf("\nGraySpatCon: Writing float output to disk.");
			nrows_in = parameters.nrows;
			ncols_in = parameters.ncols;
			temp_int = nrows_in * ncols_in;
			if(fwrite(mat_out, sizeof(float), temp_int, outfile) != temp_int ) {
				printf("\nError writing output file.\n"); exit(23);
			}
		}
		printf("\nGraySpatCon: Output file written OK.");
		// Exit nicely 
		fclose(outfile);
		free(mat_out);
		printf("\nGraySpatCon: Normal finish.\n");
		exit(0);
	} // end of moving window analysis code
// Code block for global analyis
	if(parameters.global == 1){
		printf("\nGraySpatCon: Conducting a global analysis.");
		// will be working with mat_in_byte only
		metric_value = Global_Analysis();
		fprintf(outfile,"Metric value = %f for metric %ld",  metric_value, parameters.metric);
		fprintf(outfile,"\nFor the parameter set:");
		fprintf(outfile,"\nR = %ld\nC = %ld\nM = %ld\nF = %ld\nG = %ld\nP = %ld\nW = %ld\nA = %ld\nB = %ld\nX = %ld\nY = %ld\nK = %ld", parameters.nrows, parameters.ncols, parameters.metric, parameters.data_type, parameters.global, parameters.exclude_zero, parameters.window_size, parameters.mask_missing, parameters.byte_stretch, parameters.target_code_1, parameters.target_code_2, parameters.k_value);
		// exit nicely
		fclose(outfile);
		free(mat_in_byte);
		printf("\nGraySpatCon: Normal finish.\n");
	}

}
/*   *********************
     Read_Parameter_File
     *********************
*/
long int Read_Parameter_File(FILE *parfile)
{
	//	W x - window size - side length (number of pixels) of square window; must be odd number > 3 (5,7,9...)
	//	R x - number of rows in input image
	//	C x - number of columns in input image
	//	F x - output image precision (1 = 8-bit unsigned integer; 2 = 32-bit float)
	//	M x - metric selection
	//	B x - byte stretch if converting to bytes. 
	//		For metrics bounded in [0.0, 1.0] only: 1 - [0, 100], 2 - [0, 254].
	//		For all metrics, 3 - [min=0, max=254], 4 - [0, max=254], 5 - [0, max=100]
	//		For mean, median, majority, and LM only, no stretch, convert float to byte
	//	X x - target code 1 (x  in [0,100])
	//	Y x - target code 2 (x  in [0,100])
	//	G x - moving window (default, 0) or global (1) analysis
	//  A x - mask missing: 0 - do not mask input missing on output; 1 - set missing input pixels to missing output pixels
	//  P x - exclude zero pixel values, 0 = no, 1 = yes
	//  K x - user-selected k value

	static char ch;
	static long int value, flag;
	value = -99;
	flag = 1;
	while(flag == 1) {
		if (fscanf(parfile,"%s %ld", &ch, &value) != 2) {
			flag = 0;
			continue;
		}
		if((ch == 'w') || (ch == 'W')) {
			parameters.window_size = value;
			continue;
		}
		if((ch == 'r') || (ch == 'R')) {
			parameters.nrows = value;
			continue;
		}
		if((ch == 'c') || (ch == 'C')) {
			parameters.ncols = value;
			continue;
		}
		if((ch == 'm') || (ch == 'M')) {
			parameters.metric = value;
			continue;
		}
		if((ch == 'f') || (ch == 'F')) {
			parameters.data_type = value;
			continue;
		}
		if((ch == 'b') || (ch == 'B')) {
			parameters.byte_stretch = value;
			continue;
		}
		if((ch == 'x') || (ch == 'X')) {
			parameters.target_code_1 = value;
			continue;
		}
		if((ch == 'y') || (ch == 'Y')) {
			parameters.target_code_2 = value;
			continue;
		}
		if((ch == 'g') || (ch == 'G')) {
			parameters.global = value;
			continue;
		}
		if((ch == 'a') || (ch == 'A')) {
			parameters.mask_missing = value;
			continue;
		}
		if((ch == 'p') || (ch == 'P')) {
			parameters.exclude_zero = value;
			continue;
		}
		if((ch == 'k') || (ch == 'K')) {
			parameters.k_value = value;
			continue;
		}
		return(1);
	}
	if(value == -99) {
		return(1);
	}
	return(0);
}
/*  *********************
    Check_Parameters_Set_Controls
	Check parameters, set control variables
    *********************
*/
long int Check_Parameters_Set_Controls()
{
	long int temp_int;
	if( (parameters.nrows == 0) || (parameters.ncols == 0) ){
		printf("\nGraySpatCon: Error -- Parameters _R_ and _C_ must be larger than 0."); return(1);
	}
	if( (parameters.global != 0) && (parameters.global != 1) ){
		printf("\nGraySpatCon: Error -- Parameter _G_ must be either 0 (moving window; default) or 1 (global extent)."); return(1);
	}
	if( parameters.metric < 1){	// will check for illegal metric number later
		printf("\nGraySpatCon: Error -- Parameter _M_ must be at least 1."); return(1);
	}
	// check parameters specific to moving window analysis
	if(parameters.global == 0){
		if( (temp_int = parameters.window_size % 2) == 0) {
			printf("\nGraySpatCon: Error -- Parameter _W_ must be an odd integer."); return(1);
		}
		if( parameters.window_size < 3) {
			printf("\nGraySpatCon: Error -- The minimum value for Parameter _W_ is 3."); return(1);
		}
		if( (parameters.window_size > parameters.nrows) || (parameters.window_size > parameters.ncols) ) {
			printf("\nGraySpatCon: Error -- Parameter _W_ must be smaller than parameters _R_ and _C_."); return(1);
		}
		if( (parameters.data_type < 1) || (parameters.data_type > 2) ){
			printf("\nGraySpatCon: Error -- Parameter _F_ must be 1 or 2."); return(1);
		}
		// ignore parameter B if doing float output- data_type=2
		if(parameters.data_type == 1) {
			if( (parameters.byte_stretch < 1) || (parameters.byte_stretch > 6) ){
				printf("\nGraySpatCon: Error -- Parameter _B_ must be in range [1,6]."); return(1);
			}
		}
	}
// control parameters
	control.npix_window = parameters.window_size * parameters.window_size;
	control.nadj_window = 2 * (parameters.window_size - 1) * parameters.window_size;
	control.npix_map = parameters.nrows * parameters.ncols;
	control.nadj_map = ((parameters.nrows - 1) * parameters.ncols) + ((parameters.ncols - 1) * parameters.nrows);
	if(parameters.exclude_zero == 1) {
		control.zero_is_missing = 1;
	}
	// some control parameters depend on metric selection.
	switch(parameters.metric)		// set metric-specific parameters
									// freq_type: 1 = pixels, 2 = adjacencies, 3 = both
									// bounded: 1 = no, 2 = yes
	{
		case 1:		// mean pixel value; 
			control.freq_type = 1; control.bounded = 1; control.no_stretch = 1;
			break;
		case 2:		// Shannon evenness of adjacency matrix, with regard to pixel order in adjacencies
			control.freq_type = 2; control.bounded = 2;
			break;
		case 3:		// Shannon evenness of adjacency matrix, without regard to pixel order in adjacencies
			control.freq_type = 2; control.bounded = 2;
			break;
		case 4:		// Shannon entropy of adjacency matrix, with regard to pixel order in adjacencies
			control.freq_type = 2; control.bounded = 1;
			break;
		case 5:		// Shannon entropy of adjacency matrix, without regard to pixel order in adjacencies
			control.freq_type = 2; control.bounded = 1;
			break;
		case 6:	// Sum of diagonals contagion of adjacency matrix,
			control.freq_type = 2; control.bounded = 2;
			break;
		case 7:	// Shannon entropy of pixel values; 
			control.freq_type = 1; control.bounded = 1;
			break;
		case 8:	// Shannon evenness of pixel values; 
			control.freq_type = 1; control.bounded = 2;
			break;
		case 9:	// Median pixel value; 
			control.freq_type = 1; control.bounded = 1; control.no_stretch = 1;
			break;
		case 10:	// Gini-Simpson diversity of pixel values, 
			control.freq_type = 1; control.bounded = 2;
			break;
		case 11:	// Gini-Simpson evenness of pixel values; 
			control.freq_type = 1; control.bounded = 2;
			break;
		case 12:	// Gini-Simpson evenness of adjacency matrix,with regard to pixel order in adjacencies
			control.freq_type = 2; control.bounded = 2;
			break;
		case 13:	// Gini-Simpson evenness of adjacency matrix, without regard to pixel order in adjacencies
			control.freq_type = 2; control.bounded = 2;
			break;
		case 14:	// Gini-Simpson diversity of adjacency matrix, with regard to pixel order in adjacencies
			control.freq_type = 2; control.bounded = 2;
			break;
		case 15:	// Gini-Simpson diversity of adjacency matrix, without regard to pixel order in adjacencies
			control.freq_type = 2; control.bounded = 2;
			break;
		case 16:	// Majority pixel value 
			control.freq_type = 1; control.bounded = 1; control.no_stretch = 1;
			break;
		case 17:	// Landscape mosaic; 19 LM classes
			control.freq_type = 1; control.bounded = 1; control.LM_flag = 1; control.no_stretch = 1;
			break;
		case 18:	// Landscape mosaic; 103 LM classes
			control.freq_type = 1; control.bounded = 1; control.LM_flag = 1; control.no_stretch = 1;
			break;
		case 19:	// Number of unique pixel values, 
			control.freq_type = 1; control.bounded = 1; control.no_stretch = 1;
			break;
		case 20:	// Dominance_pixel, 
			control.freq_type = 1; control.bounded = 2; 
			break;
		case 21:	// Area density, 
			control.freq_type = 1; control.bounded = 2; control.target1_flag = 1;
			break;
		case 22:	// Focal adjacency_1: Proportion of total adjacencies, which involve one specific pixel value.
			control.freq_type = 2; control.bounded = 2; control.target1_flag = 1;
			break;
		case 23:	// Focal adjacency_2: Proportion of total adjacencies, which involve two specific pixel values.
			control.freq_type = 2; control.bounded = 2; control.target1_flag = 1; control.target2_flag = 1;
			break;
		case 24:	// Focal adjacency_3: Proportion of total adjacencies for one specific pixel value, which involve a second specific pixel value. 
			control.freq_type = 2; control.bounded = 2; control.target1_flag = 1; control.target2_flag = 1;
			break;
		case 25:	// Population standard deviation of pixel values
					// It is not bounded in [0,1] but must be in [0.0, 100.0]
			control.freq_type = 1; control.bounded = 1; control.no_stretch = 1;
			break;
		case 26:	// Population coefficient of variation of pixel values
			control.freq_type = 1; control.bounded = 1; 
			break;
		case 27:	// Range of pixel values,  (max - min)
			control.freq_type = 1; control.bounded = 1; control.no_stretch = 1;
			break;
		case 28: 	// Dissimilarity
					// Soh (1999). Sum( Pij * |i-j| ). Indept of pixel order
			control.freq_type = 2; control.bounded = 1; 
			break;
		case 29: 	// Contrast
					// Haralick (1973), computing formula in Lofstedt. Sum( Pij * ( (i-j)**2 ) ). Indept of pixel order
			control.freq_type = 2; control.bounded = 1;
			break;
		case 30:	// Uniformity of adjacency matrix,with regard to pixel order in adjacencies
			control.freq_type = 2; control.bounded = 2;
			break;
		case 31:	// Uniformity of adjacency matrix,  without regard to pixel order in adjacencies
			control.freq_type = 2; control.bounded = 2;
			break;
		case 32:	// Homogeneity, 
			control.freq_type = 2; control.bounded = 2;
			break;
		case 33:	// Inverse difference, 
					// Clausi (2002). Computing formula in Lofstedt. Indept of pixel order. Inverse difference = sum Pij/(1+|i-j|)
			control.freq_type = 2; control.bounded = 2; 		
			break;		
		case 34:	// Similarity_1, 
			control.freq_type = 2; control.bounded = 2; 
			break;
		case 35:	// Similarity_2, 
			control.freq_type = 2; control.bounded = 2; control.range_flag = 1;
			break;
		case 36:	// Similarity_3, .  Note freq type is 'both'
			control.freq_type = 3; control.bounded = 2; 
			break;
		case 37:	// Dominance_adjacency,with regard to pixel order
			control.freq_type = 2; control.bounded = 2;
			break;
		case 38:	// Dominance_adjacency,  without regard to pixel order
			control.freq_type = 2; control.bounded = 2;  
			break;
		case 39:	// Difference entropy, 
					// Haralick 1973. indept of pixel order; use Lofstedt equations
			control.freq_type = 2; control.bounded = 1; 		
			break;
		case 40:	// Difference evenness,
					// Apparently a new metric, it scales entropy to obs/max in [0,1], by analogy to entropy of pixel values
			control.freq_type = 2; control.bounded = 2; 
			break;
		case 41:	// Sum entropy,
			control.freq_type = 2; control.bounded = 1; 
			break;
		case 42:	// Sum evenness, 
			control.freq_type = 2; control.bounded = 2; 
			break;
		case 43:	// Autocorrelation, 
			control.freq_type = 2; control.bounded = 1; 
			break;
		case 44:	// Correlation, 
			control.freq_type = 2; control.bounded = 1; control.negative_flag = 1;
			break;
		case 45:	// Cluster shade,
			control.freq_type = 2; control.bounded = 1; control.negative_flag = 1;
			break;
		case 46:	// Cluster prominence,
			control.freq_type = 2; control.bounded = 1; 
			break;
		case 47:	// Root mean square of pixel values,
			control.freq_type = 1; control.bounded = 1; 
			break;
		case 48:	// Average absolute deviation of pixel values, 
			control.freq_type = 1; control.bounded = 1;
			break;
		case 49:	// k-contagion
			control.freq_type = 2; control.bounded = 2; control.k_diff_flag = 1;
			break;
		case 50:	// skewness of pixel values
			control.freq_type = 1; control.bounded = 1; control.negative_flag = 1;
			break;
		case 51:	// kurtosis of pixel values
			control.freq_type = 1; control.bounded = 1;
			break;
		case 52:	// clustering 
			control.freq_type = 2; control.bounded = 2; // bounded in [0,1]
			break;
		default:
			printf("\nGraySpatCon: Error -- Metric number (parameter _M_) %ld is not available.", parameters.metric); return(1);
	}
// check some more things if doing moving window
	if(parameters.global == 0){
		if(parameters.data_type == 1){
			// check consistency between byte stretch and boundedness 
			if( (parameters.byte_stretch == 1) || (parameters.byte_stretch == 2) ) {
				if(control.bounded == 1) {
					printf("\nGraySpatCon: Cannot use parameter B = 1 or 2 with unbounded metrics, use B = 3, 4, or 5."); return(1);
				}
			}
		}
		// special handling of landscape mosaics, mean/median/majority pixel value, number of unique pixel values, range, and std dev
		if( (control.no_stretch == 1) && (parameters.data_type == 1) && (parameters.byte_stretch != 6) ) {
			printf("\nGraySpatCon: Must use parameter B = 6 for selected metric."); return(1);
		}
		if( (control.no_stretch == 0) && (parameters.data_type == 1) && (parameters.byte_stretch == 6) ) {
			printf("\nGraySpatCon: Cannot use parameter B = 6 for the selected metric."); return(1);
		}
	}
// check that target codes are available and valid for some metrics
	if(control.target1_flag == 1){
		if(parameters.target_code_1 == 255){
			printf("\nGraySpatCon: Must specify target code 1 with the _X_ parameter for the selected metric."); return(1);
		}
		if( (parameters.target_code_1 < 0) || (parameters.target_code_1 > 100) ){
			printf("\nGraySpatCon: Error -- Parameter _X_ must be integer in [0,100]."); return(1);
		}
	}
	if(control.target2_flag == 1){
		if(parameters.target_code_2 == 255){
			printf("\nGraySpatCon: Must specify target code 2 with the _Y_ parameter for the selected metric."); return(1);
		}
		if( (parameters.target_code_2 < 0) || (parameters.target_code_2 > 100) ){
			printf("\nGraySpatCon: Error -- Parameter _Y_ must be integer in [0,100]."); return(1);
		}
	}
	if( (control.target1_flag == 1) && (control.zero_is_missing == 1) && (parameters.target_code_1 == 0) ){
		printf("\nGraySpatCon: Target_code_1 (parameter _X_) cannot be zero for a metric that treats zero as missing."); return(1);
	}
	if( (control.target2_flag == 1) && (control.zero_is_missing == 1) && (parameters.target_code_2 == 0) ){
		printf("\nGraySpatCon: Target_code_2 (parameter _Y_) cannot be zero for a metric that treats zero as missing."); return(1);
	}
// metrics that can be negative can only be float output
	if( (control.negative_flag == 1) && (parameters.data_type != 2) ){
		printf("\nGraySpatCon: Error: Parameter _F_ must be 2 for the selected metric."); return(1);
	}
//  some difference metrics eg k-contagion must to supply k value in parameter file
	if(control.k_diff_flag == 1){
		if(parameters.k_value == 255){
			printf("\nGraySpatCon: Must specify k value with the _K_ parameter for the selected metric."); return(1);
		}
		if(parameters.exclude_zero == 0){
			if( (parameters.k_value < 0) || (parameters.k_value > 100) ){
				printf("\nGraySpatCon: Error -- Parameter _K_ must be integer in [0,100] when zero values are included."); return(1);
			}
		}
		if(parameters.exclude_zero == 1){
			if( (parameters.k_value < 0) || (parameters.k_value > 99) ){
				printf("\nGraySpatCon: Error -- Parameter _K_ must be integer in [0,99] when zero values are excluded."); return(1);
			}
		}
	}
	return(0);
}
/*  *********************
    Check_Input_Data
	Ensure input data conform to requirements for 8-bit [0, 100] or 255
	Change value 255 to 101
    *********************
*/
long int Check_Input_Data()
{
	static long int nrows_in, ncols_in, row, col, index, temp_int;
	
	nrows_in = parameters.nrows; 				// input image dimensions
	ncols_in = parameters.ncols;
	if(control.LM_flag == 0) {		// not landscape mosaics
		for(row = 0; row < nrows_in; row++) { // do not parallelize because omp doesn't allow return within loop
			index = row * ncols_in;
			for(col = 0; col < ncols_in; col++) {
				temp_int = (*(mat_in_byte + index + col));
				if( (temp_int < 0) || (temp_int > 100) ) {
					if(temp_int != 255) {
						return(1);
					}
				}
				// change 255 to 101
				if(temp_int == 255) {
					(*(mat_in_byte + index + col)) = 101;
				}
			}
		}
	}
	if(control.LM_flag == 1) {		// landscape mosaics require input in [1,3]
		for(row = 0; row < nrows_in; row++) { // do not parallelize because omp doesn't allow return within loop
			index = row * ncols_in;
			for(col = 0; col < ncols_in; col++) {
				temp_int = (*(mat_in_byte + index + col));
				if( (temp_int < 1) || (temp_int > 3) ) {
					if(temp_int != 255) {
						return(2);
					}
				}
				// change 255 to 101
				if(temp_int == 255) {
					(*(mat_in_byte + index + col)) = 101;
				}
			}
		}
	}
	return(0);
}
/*  *********************
    Buffer_Data
	Make a copy of the data matrix, buffering it all around with missing value 101
	(missing 255 was changed to missing 101 in Check_Input_Data
    *********************
*/
long int Buffer_Data()
{
	static long int buff_b, nrows_in, ncols_in, n_rows, n_cols, row, col, pos_in, index, temp_int;

	nrows_in = parameters.nrows; 				// input image dimensions
	ncols_in = parameters.ncols;
	buff_b = (parameters.window_size - 1) / 2;  // one side of image 
	n_cols = ncols_in + (2 * buff_b);			// buffered image dimensions
	n_rows = nrows_in + (2 * buff_b);
	// buffer the image
#pragma omp parallel  for  	 private (row, col, index)		
	for(row = 0; row < buff_b; row++) {          // buffer the top 
		index = row * n_cols;
		for(col = 0; col < n_cols; col++) {
			*(mat_temp + index + col) = 101;
		}
	} // end OMP
#pragma omp parallel  for  	 private (row, col, index, pos_in, temp_int)		
	for(row = buff_b; row < (buff_b + nrows_in); row++) { // for each row in the data area
		index = row * n_cols;
		for(col = 0; col < buff_b; col++) {    // buffer the left 
			*(mat_temp + index + col) = 101;
		}
		// copy original values from data area of image
		for(col = buff_b; col < (buff_b + ncols_in); col++) {
			// equivalent position in mat_in 
			pos_in = ((row - buff_b) * ncols_in) + (col - buff_b);
			temp_int = (*(mat_in_byte + pos_in));
			*(mat_temp + index + col) = temp_int;
		}
		for(col = (buff_b + ncols_in); col < n_cols; col++) { // buffer the right 
			*(mat_temp + index + col) = 101;   
		}
	} // end omp
#pragma omp parallel  for  	 private (row, col, index)		
	for(row = (buff_b + nrows_in); row < n_rows; row++) { //buffer the bottom
		index = row * n_cols;
		for(col = 0; col < n_cols; col++) {
			*(mat_temp + index + col) = 101;
		}
	}  // end omp
	return(0);
}

/*  *********************
    Un-buffer_Data
	Remove the buffer from the output image.
	This function moves pixel data values towards the start of the output image memory block, thus re-using the memory block.
	Cannot be parallelized.
    *********************
*/
long int Unbuffer_Data()
{
	static long int buff_b, nrows_in, ncols_in, n_cols, row, col, pos_in, index;

	nrows_in = parameters.nrows; 				// input image dimensions
	ncols_in = parameters.ncols;
	buff_b = (parameters.window_size - 1) / 2;  // one side of image 
	n_cols = ncols_in + (2 * buff_b);			// buffered image dimensions
	// Shift the data area to start of buffered memory block 
	for(row = buff_b; row < (buff_b + nrows_in); row++) {  
		index = row * n_cols;
		for(col = buff_b; col < (buff_b + ncols_in); col++) {
			// equivalent position in un-buffered image
			pos_in = ((row - buff_b) * ncols_in) + (col - buff_b);
			(*(mat_out + pos_in)) = (*(mat_out + index + col));
		} 
	}
	return(0);
}
	
/* ***********************************************************************
    Moving_Window
    *************
	
   *********************************************************************** */
long int Moving_Window()
{
	long int freq_type, numval, nrows_in, ncols_in, buff_b, n_cols_buf, temp_int, index;
	long int r, c, row, col, c_min, c_max, r_min, r_max, t1, t2, new_c_min, new_c_max;	
	float metric_value;
	
	freq_type = control.freq_type;
	numval = 102;								// max number of byte values in 0,101; hardwired = 102
	nrows_in = parameters.nrows; 				// input image dimensions
	ncols_in = parameters.ncols;
	buff_b = (parameters.window_size - 1) / 2;  // one side of image 
	n_cols_buf = ncols_in + (2 * buff_b);		// buffered image dimensions, need only cols in this function
// OMP pragma to parallelize the for loop over the rows of the data area
#pragma omp parallel  for  	 private (row, col, temp_int, index, c_min, c_max, r_min, r_max, r, c, t1, t2, metric_value, new_c_max, new_c_min)
    for(row = 0; row < nrows_in; row++) {
		// declare frequency tables, allocate and de-allocate memory for them inside the omp loop
		long int *freqptr1; // pixel frequencies
		long int *freqptr2; // adjacency frequencies
		temp_int = numval;
		// the following callocs cannot be checked for completion because OMP doesn't allow exiting from within an OMP for loop
		freqptr1 = (long int *)calloc(temp_int, sizeof(long int) );
		for(index = 0; index < temp_int; index++) {	// Zero the freq distns at the start of a row
            (*(freqptr1 + index)) = 0;
        }
		temp_int = numval * numval;
		freqptr2 = (long int *)calloc(temp_int, sizeof(long int) );
		for(index = 0; index < temp_int; index++) {	// Zero the freq distns at the start of a row
            (*(freqptr2 + index)) = 0;
        }
		// Seed the accumulator with the first data area column on the left
		// window boundary in the buffered data area for the given row in the data area
		c_min = 0;
		c_max = parameters.window_size;
		r_min = row;
		r_max = r_min + parameters.window_size;
		if( (freq_type == 1) || (freq_type == 3) ){ 			// accumulating pixel values
			for(r = r_min; r < r_max; r++) {		// these loops start at the upper left of the window in the buffered image
				for(c = c_min; c < c_max; c++) {
					temp_int = (*(mat_temp + (r * n_cols_buf) + c));
					(*(freqptr1 + temp_int))++;
				}
			}
		}
		if( (freq_type == 2) || (freq_type == 3) ){ 			// accumulating adjacency values
			// use three loops to avoid lots of if statements
			for(r = r_min; r < r_max - 1; r++) {	// excludes last row in window
				for(c = c_min; c < c_max - 1; c++) {  // excludes last column in window
					t1 = (*(mat_temp + (r * n_cols_buf) + c));  		// this pixel
                    t2 = (*(mat_temp + ((r+1) * n_cols_buf) + c)); 		// pixel below
                    // accumulate this adjacency
					temp_int = (t1 * 102) + t2;							
                    (*(freqptr2 + temp_int))++;
                    t2 = (*(mat_temp + (r * n_cols_buf) + c + 1)); 		// pixel at right
					// accumulate this adjacency
                    temp_int = (t1 * 102) + t2;					
                    (*(freqptr2 + temp_int))++;
                }
			}
			// process last column in window
			for(r = r_min; r < r_max - 1; r++) {	// excludes last row in window
                t1 = (*(mat_temp + (r * n_cols_buf) + (c_max - 1)));  		// this pixel
                t2 = (*(mat_temp + ((r+1) * n_cols_buf) + (c_max - 1))); 	// pixel below
                // accumulate this adjacency
				temp_int = (t1 * 102) + t2;							
                (*(freqptr2 + temp_int))++;
			}
			// process last row in window
			for(c = c_min; c < c_max - 1; c++) {   // excludes last column in window
				t1 = (*(mat_temp + ((r_max-1) * n_cols_buf) + c));  // this pixel
				t2 = (*(mat_temp + ((r_max-1) * n_cols_buf) + c + 1)); 	// pixel at right
                // accumulate this adjacency
				temp_int = (t1 * 102) + t2;							
                (*(freqptr2 + temp_int))++;
            }
		}
		// calculate the metric for the seed window
		metric_value = Metric_Calculator(freqptr1, freqptr2);
		// store the metric value in the buffered output image
		temp_int = ((r_min + buff_b) * n_cols_buf) + (c_min + buff_b);
		(*(mat_out + temp_int)) = metric_value;
		// Proceed to the right, subtracting and adding from the accumulator
        for(col = 1; col < ncols_in; col++) {		// skip col 0
            new_c_min = c_min + 1;
            new_c_max = c_max + 1;
            if( (freq_type == 1) || (freq_type == 3) ) {
				for(r = r_min; r < r_max; r++) {
                    // Subtract from the left
					temp_int = (*(mat_temp + (r * n_cols_buf) + c_min));
                    (*(freqptr1 + temp_int))--;
                     // Add from the right
					temp_int = (*(mat_temp + (r * n_cols_buf) + c_max));
                    (*(freqptr1 + temp_int))++;
                }
            }
			if( (freq_type == 2) || (freq_type == 3) ) {
				for(r = r_min; r < r_max - 1; r++) {   // all but last row in window
                    // subtract from the left, doing every old column
					t1 = (*(mat_temp + (r * n_cols_buf) + c_min));  	// this pixel
                    t2 = (*(mat_temp + ((r+1) * n_cols_buf) + c_min));  // pixel below
                    temp_int = (t1 * 102) + t2;
                    (*(freqptr2 + temp_int))--;
                    t2 = (*(mat_temp + (r * n_cols_buf) + c_min + 1)); 		// pixel at right
                    temp_int = (t1 * 102) + t2;
                    (*(freqptr2 + temp_int))--;
                    // Add from the right, looking left and down
                    // the new material comes from the joins on the left
                    t1 = (*(mat_temp + (r * n_cols_buf) + c_max));  	// this pixel
                        t2 = (*(mat_temp + ((r+1) * n_cols_buf) + c_max)); 	// pixel below
                        temp_int = (t1 * 102) + t2;
                        (*(freqptr2 + temp_int))++;
                        t2 = (*(mat_temp + (r * n_cols_buf) + c_max -1)); 	// pixel at left
                        //note order of t1 and t2 switched in the following, because need
						// to store in same order as they will be deleted later... 
                        temp_int = (t2 * 102) + t1;
                        (*(freqptr2 + temp_int))++;
				}
				// look at last row in window
				t1 = (*(mat_temp + ((r_max-1) * n_cols_buf) + c_min));  	// this pixel
                t2 = (*(mat_temp + ((r_max-1) * n_cols_buf) + c_min + 1));  // pixel at right
                temp_int = (t1 * 102) + t2;
                (*(freqptr2 + temp_int))--;
 				// add from the right, but looking left again
                t1 = (*(mat_temp + (r * n_cols_buf) + c_max));  	// this pixel
                t2 = (*(mat_temp + (r * n_cols_buf) + c_max - 1)); 	// pixel at left
                // note the switch of order of t1,t2...see comment above
                temp_int = (t2 * 102) + t1;
                (*(freqptr2 + temp_int))++;
            }
            // Update c_min and c_max prior to executing next column
            c_min = new_c_min;
            c_max = new_c_max;
			// calculate the metric for the  window
			metric_value = Metric_Calculator(freqptr1, freqptr2);
			// store the metric value in the buffered output image
			temp_int = ((r_min + buff_b) * n_cols_buf) + (c_min + buff_b);
			(*(mat_out + temp_int)) = metric_value;	
		}		
		free(freqptr1); // release memory for this row
		free(freqptr2);		
	}	// end of omp parallel for loop
	return(0);
}
/*  *********************
    Float2Byte
	Convert float mat_out to byte mat_out_byte.
	255b reserved for missing output
    *********************
*/	
long int Float2Byte()
{	
	long int nrows, ncols, index, OK, row, col;
	float temp_float, min, max, range, inv_range, target_max;
	nrows = parameters.nrows;
	ncols = parameters.ncols;
	max = 0.0;
	min = 0.0;
	if(parameters.byte_stretch == 1) {	//  From metric value in [0.0, 1.0] to byte in [0, 100] with 255 missing
										// Check_Parameters_Set_Controls already checked for incorrect use of byte stretch = 1 with unbounded metrics
		printf("\nGraySpatCon: Re-scaling from float in [0.0, 1.0] to byte in [0, 100]; 255 byte represents missing.");
#pragma omp parallel  for  	 private (row, col, index, temp_float)		
		for(row = 0; row < nrows; row++) {
			for(col = 0; col < ncols; col++) {
				index = (row * ncols) + col;
				temp_float = (*(mat_out + index));
				if(temp_float < 0.0) {	// metric is -0.01 if missing 
					(*(mat_out_byte + index)) = 255;	// 255 reserved for missing
				}
				else {
					(*(mat_out_byte + index)) = round( (temp_float * 100.) );
				}
			}
		} // end omp
		return(0);
	}
	if(parameters.byte_stretch == 2) {	// From metric value in [0.0, 1.0] to byte in [0, 254] with 255 missing
										// Check_Parameters_Set_Controls already checked for incorrect use of byte stretch = 2 with unbounded metrics
		printf("\nGraySpatCon: Re-scaling from float in [0.0, 1.0] to byte in [0, 254]; 255 byte represents missing.");
#pragma omp parallel  for  	 private (row, col, index, temp_float)		
		for(row = 0; row < nrows; row++) {
			for(col = 0; col < ncols; col++) {
				index = (row * ncols) + col;
				temp_float = (*(mat_out + index));
				if(temp_float < 0.0) {	// metric is -0.01 if missing 
					(*(mat_out_byte + index)) = 255;	// 255 reserved for missing
				}
				else {
					(*(mat_out_byte + index)) =  round( (temp_float * 254.)	 ); 
				}
			}
		} // end omp
		return(0);
	}
	if(parameters.byte_stretch == 3) {	// stretch from [min, max] to [0,254] with 255 missing
		// scan for min and max values, excluding missing -0.01
		OK = 0;
		for(row = 0; row < nrows; row++) {	// scan to find first non-missing value, do not parallelize
			for(col = 0; col < ncols; col++) {
				index = (row * ncols) + col;
				if((*(mat_out + index)) >= 0.0) {
					min = (*(mat_out + index));		//initialize min and max
					max = (*(mat_out + index));
					OK = 1;
				}
				if(OK == 1) {break;}
			}
		}
		if(OK == 0) {
			printf("\nGraySpatCon: Cannot re-scale to [min, max] byte output; all output values are missing.\n"); exit(22);
		}
		for(row = 0; row < nrows; row++) {	// do not parallelize
			for(col = 0; col < ncols; col++) {
				index = (row * ncols) + col;
				if((*(mat_out + index)) > max) {
					max = (*(mat_out + index));
				}
				if((*(mat_out + index)) >= 0.0) {  // skip missing values -0.01
					if((*(mat_out + index)) < min) {
						min = (*(mat_out + index));
					}
				}
			}
		}
		// stretch from [min, max} to [0,254], reserving 255 for missing
		printf("\nGraySpatCon: Re-scaling from float in [Min, Max] [%f, %f] to byte in [0, 254]; 255 byte represents missing.", min, max);
		range = max - min;
		if(range == 0.0) {
			printf("\nGraySpatCon: Cannot re-scale to [min, max] byte output; min = max.\n"); exit(22);
		}
		inv_range = 1.0 / range;
#pragma omp parallel  for  	 private (row, col, index, temp_float)		
		for(row = 0; row < nrows; row++) {
			for(col = 0; col < ncols; col++) {
				index = (row * ncols) + col;
				if( (*(mat_out + index)) < 0.0) {	// metric is -0.01 if missing
					(*(mat_out_byte + index)) = 255;	// 255 reserved for missing
				}
				else {
					temp_float =  ( (*(mat_out + index)) - min);	// re-scale to min=0
					(*(mat_out_byte + index)) =  round( (inv_range * temp_float * 254.) ); 
				}
			}
		} // end OMP
		return(0);
	}
	if( (parameters.byte_stretch == 4) || (parameters.byte_stretch == 5) ){	// stretch from [0, max] to [0,254], or to [0, 100], with 255 missing
		// scan for max value, excluding missing -0.01
		OK = 0;
		for(row = 0; row < nrows; row++) {	// scan to find first non-missing value, do not parallelize
			for(col = 0; col < ncols; col++) {
				index = (row * ncols) + col;
				if((*(mat_out + index)) >= 0.0) {
					max = (*(mat_out + index));		//initialize max
					OK = 1;
				}
				if(OK == 1) {break;}
			}
		}
		if(OK == 0) {
			printf("\nGraySpatCon: Cannot convert to [0, max] byte output; all output values are missing."); exit(22);
		}
		for(row = 0; row < nrows; row++) {	// do not parallelize
			for(col = 0; col < ncols; col++) {
				index = (row * ncols) + col;
				if((*(mat_out + index)) > max) {
					max = (*(mat_out + index));
				}
			}
		}
		// stretch from [0, max] to [0,254], or to [0, 100], reserving 255 for missing
		printf("\nGraySpatCon: Re-scaling from float in [0, Max] [0, %f] to byte in [0, 254]; 255 byte represents missing.", max);
		range = max;
		if(range == 0.0) {
			printf("\nGraySpatCon: Cannot convert to [0, max] byte output; max = 0.\n"); exit(22);
		}
		inv_range = 1.0 / range;
		target_max = 254.;	// default for byte_stretch = 4
		if(parameters.byte_stretch == 5) {
			target_max = 100.;
		}
#pragma omp parallel  for  	 private (row, col, index, temp_float)		
		for(row = 0; row < nrows; row++) {
			for(col = 0; col < ncols; col++) {
				index = (row * ncols) + col;
				if( (*(mat_out + index)) < 0.0) {	// metric is -0.01 if missing
					(*(mat_out_byte + index)) = 255;	// 255 reserved for missing
				}
				else {
					temp_float = (*(mat_out + index));
					(*(mat_out_byte + index)) =  round( (inv_range * temp_float * target_max) ); 
				}
			}
		} // end OMP
		return(0);
	}
	if(parameters.byte_stretch == 6) {		// only for metrics that must have original metric value; just convert float to byte
		printf("\nGraySpatCon: Converting metric from float to byte; 255 byte represents missing.");
#pragma omp parallel  for  	 private (row, col, index, temp_float)		
		for(row = 0; row < nrows; row++) {
			for(col = 0; col < ncols; col++) {
				index = (row * ncols) + col;
				temp_float = (*(mat_out + index));
				if(temp_float < 0.0) {	// metric is -0.01 if missing 
					(*(mat_out_byte + index)) = 255;	// 255 reserved for missing
				}
				else {
					(*(mat_out_byte + index)) = round(temp_float);
				}
			}
		} // end omp
		return(0);
	}
	return(1);
}	

/*  *********************
    Metric_Calculator
	Calculate the metric for one window placement
	The frequencies arrive as integers in an indexed array.
    *********************
*/
float Metric_Calculator(long int *freqptr1, long int * freqptr2)
{
	long int index, max_npix, max_nadj, temp_int, temp_int2, temp_int3, temp_max, temp_index, index1, index2, num_values, num_adjacencies, num_missing, position, numpix, sumx, sumx2, min, max, range, kval, nobs;
	float numerator, denominator, inv_denominator, metric_value, inv_num_adjacencies, temp_float, temp_float2, temp_float3, p1, p2, p3, mu_x, mu_y, sigma_x, sigma_y, temp_out;
	// Must malloc/free the following within some metrics else it will mess with parallel coding elsewhere
		long int *karray;		// kdif and ksum needed for difference entropy and sum entropy metrics,
		float *rowp;			//  tables by row and column needed for correlation
		float *colp;
	metric_value = -0.01;
	max_npix = control.npix_window; 	// default is for moving window
	max_nadj = control.nadj_window;
	if(parameters.global == 1) {		// reset if global analysis
		max_npix = control.npix_map;
		max_nadj = control.nadj_map;
	}
	switch(parameters.metric) {
		case 1:{	// mean pixel value
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){  // mean pixel value; including pixel value 0
				// number of non-missing pixels
				temp_int = max_npix - (*(freqptr1 + 101)); 	
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * temp_int);
				}
				// weighted sum of the integer pixel values (i.e., integer index values)
				temp_int = 0;
				for(index = 0; index < 101; index++) {	// includes 0, excludes missing 
						temp_int += (*(freqptr1 + index)) * index;
				}
				temp_out = inv_denominator * (1.0 * temp_int);
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){ // 1001 mean pixel value; excluding pixel value 0
				// number of non-missing pixels, excluding zero
				temp_int = max_npix - (*(freqptr1 + 0)) - (*(freqptr1 + 101)); 	
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * temp_int);
				}
				// weighted sum of the integer pixel values (i.e., integer index values)
				temp_int = 0;
				for(index = 1; index < 101; index++) {	// excludes 0 and missing 
						temp_int += (*(freqptr1 + index)) * index;
				}
				temp_out = inv_denominator * (1.0 * temp_int);
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			break;
		}
		case 2:{	// Shannon evenness of adjacency matrix, including pixel value 0, with regard to pixel order in adjacencies
					// based on equation 21 (and 12) in Li & Reynolds 1993; DOI: 10.1007/BF00125347
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// need to know the number of different byte values in the window, excluding missing
				num_values = 0;
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						if( ( (*(freqptr2 + temp_int)) > 0 ) || ( (*(freqptr2 + temp_int2)) > 0 ) ) {
							num_values++;
						}
					}
				}
				if(num_values == 0) { // no edges if no pixels
					metric_value = -0.01;
					break;
				}
				if(num_values == 1) {	// only one type of adjacency is possible; this to avoid later divide-by-zero
					metric_value = -0.01;
					break;
				}
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * log(Pij)
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * log(Pij)
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int);	// Pij
							numerator += (temp_float * log(temp_float));
						}
					}
				}
				numerator = -1.0 * numerator;  // minus the sum of Pij*log(Pij)
				denominator = 2.0 * log( (1.0 * num_values) ); // maximum numerator for given num_values
				temp_out = numerator / denominator;	// larger numbers indicate more "evenness"
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1002 Shannon evenness of adjacency matrix, excluding pixel value 0, with regard to pixel order in adjacencies
												// based on equation 5 in Riitters et al 1996; DOI: 10.1007/BF02071810
				// need to know the number of different byte values in the window, excluding zero and missing
				num_values = 0;
				for(index1 = 1; index1 < 101; index1++) {			// excludes zero and missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes zero and missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						if( ( (*(freqptr2 + temp_int)) > 0 ) || ( (*(freqptr2 + temp_int2)) > 0 ) ) {
							num_values++;
						}
					}
				}
				if(num_values == 0) { // no edges if no pixels
					metric_value = -0.01;
					break;
				}
				if(num_values == 1) {	// only one type of adjacency is possible; this to avoid later divide-by-zero
					metric_value = -0.01;
					break;
				}
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * log(Pij)
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 								// sum of the Pij * log(Pij)
				for(index1 = 1; index1 < 101; index1++) {		// excludes zero and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes zero and missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int);	// Pij
							numerator += (temp_float * log(temp_float));
						}
					}
				}
				numerator = -1.0 * numerator;  // minus the sum of Pij*log(Pij)
				denominator = 2.0 * log( (1.0 * num_values) ); // maximum numerator for given num_values
				temp_out = numerator / denominator;	// larger numbers indicate more "evenness"
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 3:{	// Shannon evenness of adjacency matrix, including pixel value 0, without regard to pixel order in adjacencies
					// based on equation 6 in Riitters et al 1996; DOI: 10.1007/BF02071810
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// need to know the number of different byte values in the window, excluding missing
				num_values = 0;
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						if( ( (*(freqptr2 + temp_int)) > 0 ) || ( (*(freqptr2 + temp_int2)) > 0 ) ) {
							num_values++;
						}
					}
				}
				if(num_values == 0) { // no edges if no pixels
					metric_value = -0.01;
					break;
				}
				if(num_values == 1) {	// only one type of adjacency is possible; this to avoid later divide-by-zero
					metric_value = -0.01;
					break;
				}
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * log(Pij)
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * log(Pij)
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						// avoid double-counting diagonal elements
						if(temp_int == temp_int2) {
							temp_int3 = (*(freqptr2 + temp_int));
						}
						else {
							temp_int3 = (*(freqptr2 + temp_int)) + (*(freqptr2 + temp_int2));	// Nij + Nji
						}
						if(temp_int3 > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int3);	// either Pii or Pij + Pji
							numerator += (temp_float * log(temp_float));
						}
					}
				}
				numerator = -1.0 * numerator;  // minus the sum of (Pij+Pji)*log(Pij+Pji))
				temp_int = (num_values * num_values) + num_values; // t2 + t
				denominator = log( (1.0 * temp_int) ) - log(2.0); // maximum numerator for given num_values
				temp_out = numerator / denominator;	// larger numbers indicate more "evenness"
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1003 Shannon evenness of adjacency matrix, excluding pixel value 0, without regard to pixel order in adjacencies
												// equation 6 in Riitters et al 1996; DOI: 10.1007/BF02071810
				// need to know the number of different byte values in the window, excluding missing and zero
				num_values = 0;
				for(index1 = 1; index1 < 101; index1++) {			// excludes zero and missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes zero and missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						if( ( (*(freqptr2 + temp_int)) > 0 ) || ( (*(freqptr2 + temp_int2)) > 0 ) ) {
							num_values++;
						}
					}
				}
				if(num_values == 0) { // no edges if no pixels
					metric_value = -0.01;
					break;
				}
				if(num_values == 1) {	// only one type of adjacency is possible; this to avoid later multiply by -1 to get -0
					metric_value = -0.01;
					break;
				}
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * log(Pij)
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * log(Pij)
				for(index1 = 1; index1 < 101; index1++) {			// excludes zero and missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes zero and missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						// avoid double-counting diagonal elements
						if(temp_int == temp_int2) {
							temp_int3 = (*(freqptr2 + temp_int));
						}
						else {
							temp_int3 = (*(freqptr2 + temp_int)) + (*(freqptr2 + temp_int2));	// Nij + Nji
						}
						if(temp_int3 > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int3);	// Pij + Pji
							numerator += (temp_float * log(temp_float));
						}
					}
				}
				numerator = -1.0 * numerator;  // minus the sum of (Pij+Pji)*log(Pij+Pji))
				temp_int = (num_values * num_values) + num_values; // t2 + t
				denominator = log( (1.0 * temp_int) ) - log(2.0); // maximum numerator for given num_values
				temp_out = numerator / denominator;	// larger numbers indicate more "evenness"
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 4:{	// Shannon entropy of adjacency matrix, including pixel value 0, with regard to pixel order in adjacencies
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// want to know the number of different byte values in the window; entropy is zero when only one value
				num_values = 0;
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						if( ( (*(freqptr2 + temp_int)) > 0 ) || ( (*(freqptr2 + temp_int2)) > 0 ) ) {
							num_values++;
						}
					}
				}
				if(num_values == 0) { // no edges if no pixels
					metric_value = -0.01;
					break;
				}
				if(num_values == 1) {	// only one type of adjacency is possible; this to avoid later multiply by -1 to get -0
					metric_value = 0.0;
					break;
				}
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * log(Pij)
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * log(Pij)
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int);	// Pij
							numerator += (temp_float * log(temp_float));
						}
					}
				}
				temp_out = -1.0 * numerator;  // minus the sum of Pij*log(Pij)
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1004 Shannon entropy of adjacency matrix, excluding pixel value 0, with regard to pixel order in adjacencies
			// need to know the number of different byte values in the window, excluding missing and zero
				num_values = 0;
				for(index1 = 1; index1 < 101; index1++) {			// excludes zero and missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes zero and missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						if( ( (*(freqptr2 + temp_int)) > 0 ) || ( (*(freqptr2 + temp_int2)) > 0 ) ) {
							num_values++;
						}
					}
				}
				if(num_values == 0) { // no edges if no pixels
					metric_value = -0.01;
					break;
				}
				if(num_values == 1) {	// only one type of adjacency is possible; this to avoid later multiply by -1 to get -0
					metric_value = 0.0;
					break;
				}
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * log(Pij)
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 								// sum of the Pij * log(Pij)
				for(index1 = 1; index1 < 101; index1++) {		// excludes zero and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes zero and missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int);	// Pij
							numerator += (temp_float * log(temp_float));
						}
					}
				}
				temp_out = -1.0 * numerator;  // minus the sum of Pij*log(Pij)
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 5:{	// Shannon entropy of adjacency matrix, including pixel value 0, without regard to pixel order in adjacencies
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// want to know the number of different byte values in the window; entropy is zero when only one value
				num_values = 0;
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						if( ( (*(freqptr2 + temp_int)) > 0 ) || ( (*(freqptr2 + temp_int2)) > 0 ) ) {
							num_values++;
						}
					}
				}
				if(num_values == 0) { // no edges if no pixels
					metric_value = -0.01;
					break;
				}
				if(num_values == 1) {	// only one type of adjacency is possible; this to avoid later multiply by -1 to get -0
					metric_value = 0.0;
					break;
				}
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * log(Pij)
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * log(Pij)
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						// avoid double-counting diagonal elements
						if(temp_int == temp_int2) {
							temp_int3 = (*(freqptr2 + temp_int));
						}
						else {
							temp_int3 = (*(freqptr2 + temp_int)) + (*(freqptr2 + temp_int2));	// Nij + Nji
						}
						if(temp_int3 > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int3);	// Pij + Pji
							numerator += (temp_float * log(temp_float));
						}
					}
				}
				temp_out = -1.0 * numerator;  // minus the sum of (Pij+Pji)*log(Pij+Pji))
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1005 Shannon entropy of adjacency matrix, excluding pixel value 0, without regard to pixel order in adjacencies
				// need to know the number of different byte values in the window, excluding missing and zero
				num_values = 0;
				for(index1 = 1; index1 < 101; index1++) {			// excludes zero and missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes zero and missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						if( ( (*(freqptr2 + temp_int)) > 0 ) || ( (*(freqptr2 + temp_int2)) > 0 ) ) {
							num_values++;
						}
					}
				}
				if(num_values == 0) { // no edges if no pixels
					metric_value = -0.01;
					break;
				}
				if(num_values == 1) {	// only one type of adjacency is possible; this to avoid later multiply by -1 to get -0
					metric_value = 0.0;
					break;
				}
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * log(Pij)
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * log(Pij)
				for(index1 = 1; index1 < 101; index1++) {			// excludes zero and missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes zero and missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						// avoid double-counting diagonal elements
						if(temp_int == temp_int2) {
							temp_int3 = (*(freqptr2 + temp_int));
						}
						else {
							temp_int3 = (*(freqptr2 + temp_int)) + (*(freqptr2 + temp_int2));	// Nij + Nji
						}
						if(temp_int3 > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int3);	// Pij + Pji
							numerator += (temp_float * log(temp_float));
						}
					}
				}
				temp_out = -1.0 * numerator;  // minus the sum of (Pij+Pji)*log(Pij+Pji))
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 6:{ 	// Sum of diagonals contagion of adjacency matrix, including pixel value 0
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pii
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pii
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					temp_int = (*(freqptr2 + ((index1 * 102) + index1)));
					temp_float = inv_num_adjacencies * (1.0 * temp_int);	// Pii
					numerator += temp_float;
				}
				temp_out = numerator;	// larger numbers indicate more "contagion"
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1006 Sum of diagonals contagion of adjacency matrix, excluding pixel value 0
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pii
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pii
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing and zero values
					temp_int = (*(freqptr2 + ((index1 * 102) + index1)));
					temp_float = inv_num_adjacencies * (1.0 * temp_int);	// Pii
					numerator += temp_float;
				}
				temp_out = numerator;	// larger numbers indicate more "contagion"
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 7:{	// Shannon entropy of pixel values; including pixel value 0
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// the number of unique byte values in the window, excluding missing
				num_values = 0;
				for(index = 0; index < 101; index++) {	// excludes missing values
					if( (*(freqptr1 + index)) > 0 ) {
						num_values++;
					}
				}
				if(num_values == 1){ 		// zero entropy if only one value
					metric_value = 0.0;
					break;
				}
				// number of non-missing pixels
				temp_int = max_npix - (*(freqptr1 + 101)); 	
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * temp_int);
				}
				// sum of Pi * log(Pi)
				numerator = 0.0;
				for(index = 0; index < 101; index++) {	// includes 0, excludes missing 
					temp_int = (*(freqptr1 + index));
					if(temp_int > 0) {
						temp_float = inv_denominator * temp_int;
						numerator += temp_float * log(temp_float);
					}
				}
				temp_out = -1.0 * numerator;
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1007 Shannon entropy of pixel values; excluding pixel value 0
				// the number of unique byte values in the window, excluding missing
				num_values = 0;
				for(index = 1; index < 101; index++) {	// excludes missing and zero values
					if( (*(freqptr1 + index)) > 0 ) {
						num_values++;
					}
				}
				if(num_values == 1){ 		// zero entropy if only one value
					metric_value = 0.0;
					break;
				}
				// number of non-missing pixels, note 0 value is missing
				temp_int = max_npix - (*(freqptr1 + 0)) - (*(freqptr1 + 101)); 	
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * temp_int);
				}
				// sum of Pi * log(Pi)
				numerator = 0.0;
				for(index = 1; index < 101; index++) {	// excludes 0, excludes missing 
					temp_int = (*(freqptr1 + index));
					if(temp_int > 0) {
						temp_float = inv_denominator * temp_int;
						numerator += temp_float * log(temp_float);
					}
				}
				temp_out = -1.0 * numerator;
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 8:{	// Shannon evenness of pixel values; including pixel value 0
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// need to know the number of different byte values in the window, excluding missing
				num_values = 0;
				for(index = 0; index < 101; index++) {	// excludes missing values
					if( (*(freqptr1 + index)) > 0 ) {
						num_values++;
					}
				}
				if(num_values == 0) { // missing if no pixels
					metric_value = -0.01;
					break;
				}
				if(num_values == 1) {	// set to missing; this avoids later divide-by-zero
					metric_value = -0.01;
					break;
				}
				// number of non-missing pixels
				temp_int = max_npix - (*(freqptr1 + 101)); 	
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * temp_int);
				}
				// sum of Pi * log(Pi)
				numerator = 0.0;
				for(index = 0; index < 101; index++) {	// includes 0, excludes missing 
					temp_int = (*(freqptr1 + index));
					if(temp_int > 0) {
						temp_float = inv_denominator * temp_int;
						numerator += temp_float * log(temp_float);
					}
				}
				numerator = -1.0 * numerator;  // minus the sum of Pij*log(Pij)
				denominator = log( (1.0 * num_values) ); // maximum numerator for given num_values
				temp_out = ( numerator / denominator );	// larger numbers indicate more "evenness"
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1008 Shannon evenness of pixel values; excluding pixel value 0
				// need to know the number of different byte values in the window, excluding zero and missing
				num_values = 0;
				for(index = 1; index < 101; index++) {	// excludes zero and missing values
					if( (*(freqptr1 + index)) > 0 ) {
						num_values++;
					}
				}
				if(num_values == 0) { // missing if no pixels
					metric_value = -0.01;
					break;
				}
				if(num_values == 1) {	// set to missing; this avoids later divide-by-zero
					metric_value = -0.01;
					break;
				}
				// number of non-missing pixels, note 0 value is missing
				temp_int = max_npix - (*(freqptr1 + 0)) - (*(freqptr1 + 101)); 	// excludes zero and missing
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * temp_int);
				}
				// sum of Pi * log(Pi)
				numerator = 0.0;
				for(index = 1; index < 101; index++) {	// includes 0, excludes missing 
					temp_int = (*(freqptr1 + index));
					if(temp_int > 0) {
						temp_float = inv_denominator * temp_int;
						numerator += temp_float * log(temp_float);
					}
				}
				numerator = -1.0 * numerator;  // minus the sum of Pij*log(Pij)
				denominator = log( (1.0 * num_values) ); // maximum numerator for given num_values
				temp_out = ( numerator / denominator );	// larger numbers indicate more "evenness"
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 9:{	// Median pixel value; including pixel value 0
					// this is sometimes approximate for even Npixels, because it's based on the cumulative frequency distribution
					// (this choice is faster and simpler than expanding the sorted list to get a better approximation)
					// it is not approximate for odd Npixels.
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){	
				// number of non-missing pixels
				temp_int = max_npix - (*(freqptr1 + 101)); 	
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				// find the position of the median value
				if( (temp_int % 2) == 0 ) 	// even number of pixels
				{
					position = temp_int / 2;    // approximate if position is at the boundary of the pixel value classes
				}
				else {						// odd number of pixels
					position = (temp_int / 2) + 1;  // not approximate
				}
				// calc the cumulative frequency distribution from the already-ordered (already sorted) pixel counts
				// the median is the pixel value for which half the total pixel count is accumulated
				temp_int = 0;
				for(index = 0; index < 101; index++) {	// includes 0, excludes missing 
					temp_int += (*(freqptr1 + index));	// accumulate the frequencies
					if(temp_int >= position) {			// break when position is achieved, for pixel value = index
						metric_value = 1.0 * index;
						break;
					}
				}
				break;
			}
			if(parameters.exclude_zero == 1){	// 1009 Median pixel value; excluding pixel value 0
				// number of non-zero and non-missing pixels
				temp_int = max_npix - (*(freqptr1 + 0)) - (*(freqptr1 + 101)); 	
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				// find the position of the median value
				if( (temp_int % 2) == 0 ) 	// even number of pixels
				{
					position = temp_int / 2;    // approximate if position is at the boundary of the pixel value classes
				}
				else {						// odd number of pixels
					position = (temp_int / 2) + 1;  // not approximate
				}
				// calc the cumulative frequency distribution from the already-ordered (already sorted) pixel counts
				// the median is the pixel value for which half the total pixel count is accumulated
				temp_int = 0;
				for(index = 1; index < 101; index++) {	// excludes 0, excludes missing 
					temp_int += (*(freqptr1 + index));	// accumulate the frequencies
					if(temp_int >= position) {			// break when position is achieved, for pixel value = index
						metric_value = 1.0 * index;
						break;
					}
				}
				break;
			}
		}
		case 10:{	// Gini-Simpson diversity of pixel values, including pixel value 0
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){	
				// number of non-missing pixels
				temp_int = max_npix - (*(freqptr1 + 101)); 	
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * temp_int);
				}
				// sum of Pi * Pi
				numerator = 0.0;
				for(index = 0; index < 101; index++) {	// includes 0, excludes missing 
					temp_int = (*(freqptr1 + index));
					if(temp_int > 0) {
						temp_float = inv_denominator * temp_int;
						numerator += temp_float * temp_float;
					}
				}
				// Gini-Simpson is 1 minus Simpson's index
				temp_out = 1.0 - numerator;	// larger numbers mean more diversity
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){ 	// 1010 Gini-Simpson diversity of pixel values, excluding pixel value 0	
			// number of non-missing and non-zero pixels
				temp_int = max_npix - (*(freqptr1 + 0)) - (*(freqptr1 + 101)); 	
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * temp_int);
				}
				// sum of Pi * Pi
				numerator = 0.0;
				for(index = 1; index < 101; index++) {	// excludes 0, excludes missing 
					temp_int = (*(freqptr1 + index));
					if(temp_int > 0) {
						temp_float = inv_denominator * temp_int;
						numerator += temp_float * temp_float;
					}
				}
				// Gini-Simpson is 1 minus Simpson's index
				temp_out = 1.0 - numerator;	// larger numbers mean more diversity
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 11:{	// Gini-Simpson evenness of pixel values; including pixel value 0
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){	
				// need to know the number of different byte values in the window, excluding missing
				num_values = 0;
				for(index = 0; index < 101; index++) {	// excludes missing values
					if( (*(freqptr1 + index)) > 0 ) {
						num_values++;
					}
				}
				if(num_values == 0) { // missing if no pixels
					metric_value = -0.01;
					break;
				}
				if(num_values == 1) {	// set to missing; this avoids later divide-by-zero
					metric_value = -0.01;
					break;
				}
				// number of non-missing pixels
				temp_int = max_npix - (*(freqptr1 + 101)); 	
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * temp_int);
				}
				// sum of Pi * Pi
				numerator = 0.0;
				for(index = 0; index < 101; index++) {	// includes 0, excludes missing 
					temp_int = (*(freqptr1 + index));
					if(temp_int > 0) {
						temp_float = inv_denominator * temp_int;
						numerator += temp_float * temp_float;
					}
				}
				// Gini-Simpson diversity is 1 minus Simpson's index
				numerator = 1.0 - numerator;	// larger numbers mean more diversity (lower evenness)
				denominator = 1.0 - ( 1.0 / (1.0 * num_values) ); // maximum numerator for given num_values
				temp_out = numerator / denominator;		// larger numbers indicate more "evenness"
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1011 Gini-Simpson evenness of pixel values; excluding pixel value 0
				// need to know the number of different byte values in the window, excluding zero and missing
				num_values = 0;
				for(index = 1; index < 101; index++) {	// excludes zero and missing values
					if( (*(freqptr1 + index)) > 0 ) {
						num_values++;
					}
				}
				if(num_values == 0) { // missing if no pixels
					metric_value = -0.01;
					break;
				}
				if(num_values == 1) {	// set to missing; this avoids later divide-by-zero
					metric_value = -0.01;
					break;
				}
				// number of non-missing and non-zero pixels
				temp_int = max_npix - (*(freqptr1 + 0)) - (*(freqptr1 + 101)); 	
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * temp_int);
				}
				// sum of Pi * Pi
				numerator = 0.0;
				for(index = 1; index < 101; index++) {	// excludes 0, excludes missing 
					temp_int = (*(freqptr1 + index));
					if(temp_int > 0) {
						temp_float = inv_denominator * temp_int;
						numerator += temp_float * temp_float;
					}
				}
				// Gini-Simpson diversity is 1 minus Simpson's index
				numerator = 1.0 - numerator;	// larger numbers mean more diversity (lower evenness)
				denominator = 1.0 - ( 1.0 / (1.0 * num_values) ); // maximum numerator for given num_values
				temp_out = numerator / denominator;		// larger numbers indicate more "evenness"
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 12:{	// Gini-Simpson evenness of adjacency matrix, including pixel value 0, with regard to pixel order in adjacencies
					// Equation 6 in Wickham & Riitters 1995; DOI: 10.1080/01431169508954647
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){	
				// need to know the number of different byte values in the window, excluding missing
				num_values = 0;
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						if( ( (*(freqptr2 + temp_int)) > 0 ) || ( (*(freqptr2 + temp_int2)) > 0 ) ) {
							num_values++;
						}
					}
				}
				if(num_values == 0) { // no edges if no pixels
					metric_value = -0.01;
					break;
				}
				if(num_values == 1) {	// only one type of adjacency is possible; this to avoid later divide-by-zero
					metric_value = -0.01;
					break;
				}
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * Pij
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * Pij
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int);	// Pij
							numerator += (temp_float * temp_float);
						}
					}
				}
				numerator = 1.0 - numerator;  // G-S diversity is one minus the sum of Pij*Pij
				denominator = 1.0 - (1.0 / (1.0 * (num_values * num_values) ) ); // maximum numerator for given num_values
				temp_out = numerator / denominator;		// larger numbers indicate more "evenness"
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1012 Gini-Simpson evenness of adjacency matrix, excluding pixel value 0, with regard to pixel order in adjacencies	
				// need to know the number of different byte values in the window, excluding 0 and missing
				num_values = 0;
				for(index1 = 1; index1 < 101; index1++) {	// excludes zero and missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes zero and missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						if( ( (*(freqptr2 + temp_int)) > 0 ) || ( (*(freqptr2 + temp_int2)) > 0 ) ) {
							num_values++;
						}
					}
				}
				if(num_values == 0) { // no edges if no pixels
					metric_value = -0.01;
					break;
				}
				if(num_values == 1) {	// only one type of adjacency is possible; this to avoid later divide-by-zero
					metric_value = -0.01;
					break;
				}
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * Pij
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * Pij
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes 0 and missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int);	// Pij
							numerator += (temp_float * temp_float);
						}
					}
				}
				numerator = 1.0 - numerator;  // G-S diversity is one minus the sum of Pij*Pij
				denominator = 1.0 - (1.0 / (1.0 * (num_values * num_values) ) ); // maximum numerator for given num_values
				temp_out = numerator / denominator;		// larger numbers indicate more "evenness"
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 13:{	// Gini-Simpson evenness of adjacency matrix, including pixel value 0, without regard to pixel order in adjacencies
					// Apparently un-published metric due to novel calculation of maximum possible value
					//  G-S diversity = 1 - sum Pij*Pij where summation is over ((t*t)+t)/2 possible types of adjacencies
					// Noting that when all Pij are equal, they all equal 2/((t*t)+t) (the inverse of the number of adjacency types)
					// Maximum G-S diversity = 1 - (2/((t*t)+t))
					// Larger G-S diversity means larger evenness, so
					// G-S evenness = G-S diversity/max G-S diversity
					//  =  [ 1 - (sumPij*Pij) ] / [ 1 - (2/((t*t)+t)) ]
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){	
				// need to know the number of different byte values in the window, excluding missing
				num_values = 0;
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						if( ( (*(freqptr2 + temp_int)) > 0 ) || ( (*(freqptr2 + temp_int2)) > 0 ) ) {
							num_values++;
						}
					}
				}
				if(num_values == 0) { // no edges if no pixels
					metric_value = -0.01;
					break;
				}
				if(num_values == 1) {	// only one type of adjacency is possible; this to avoid later divide-by-zero
					metric_value = -0.01;
					break;
				}
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * Pij
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * Pij
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						// avoid double-counting diagonal elements
						if(temp_int == temp_int2) {
							temp_int3 = (*(freqptr2 + temp_int));
						}
						else {
							temp_int3 = (*(freqptr2 + temp_int)) + (*(freqptr2 + temp_int2));	// Nij + Nji
						}
						if(temp_int3 > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int3);	// either Pii or Pij + Pji
							numerator += (temp_float * temp_float);
						}
					}
				}
				numerator = 1.0 - numerator;  // G-S diversity is one minus the sum in the numerator
				denominator = 1.0 - (2.0 / ( (num_values * num_values) + num_values) ); // maximum numerator for given num_values
				temp_out = numerator / denominator;		// larger numbers indicate more "evenness"
				metric_value = roundf(temp_out * 100000) / 100000;
				break;		
			}
			if(parameters.exclude_zero == 1){	// 1013 Gini-Simpson evenness of adjacency matrix, excluding pixel value 0, without regard to pixel order in adjacencies	
				// need to know the number of different byte values in the window, excluding missing
				num_values = 0;
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes 0 and missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						if( ( (*(freqptr2 + temp_int)) > 0 ) || ( (*(freqptr2 + temp_int2)) > 0 ) ) {
							num_values++;
						}
					}
				}
				if(num_values == 0) { // no edges if no pixels
					metric_value = -0.01;
					break;
				}
				if(num_values == 1) {	// only one type of adjacency is possible; this to avoid later divide-by-zero
					metric_value = -0.01;
					break;
				}
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * Pij
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * Pij
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes 0 and missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						// avoid double-counting diagonal elements
						if(temp_int == temp_int2) {
							temp_int3 = (*(freqptr2 + temp_int));
						}
						else {
							temp_int3 = (*(freqptr2 + temp_int)) + (*(freqptr2 + temp_int2));	// Nij + Nji
						}
						if(temp_int3 > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int3);	// either Pii or Pij + Pji
							numerator += (temp_float * temp_float);
						}
					}
				}
				numerator = 1.0 - numerator;  // G-S diversity is one minus the sum in the numerator
				denominator = 1.0 - (2.0 / ( (num_values * num_values) + num_values) ); // maximum numerator for given num_values
				temp_out = numerator / denominator;		// larger numbers indicate more "evenness"
				metric_value = roundf(temp_out * 100000) / 100000;
				break;	
			}
		}
		case 14:{	// Gini-Simpson diversity of adjacency matrix, including pixel value 0, with regard to pixel order in adjacencies
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window; G-S diversity is zero when only one value
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * Pij
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * Pij
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int);	// Pij
							numerator += (temp_float * temp_float);
						}
					}
				}
				temp_out = 1.0 - numerator;  // G-S diversity is one minus the sum of Pij*Pij
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1014 Gini-Simpson diversity of adjacency matrix, excluding pixel value 0, with regard to pixel order in adjacencies
				// No need to know the number of different byte values in the window; G-S diversity is zero when only one value
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * Pij
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * Pij
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes 0 and missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int);	// Pij
							numerator += (temp_float * temp_float);
						}
					}
				}
				temp_out = 1.0 - numerator;  // G-S diversity is one minus the sum of Pij*Pij
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 15:{	// Gini-Simpson diversity of adjacency matrix, including pixel value 0, without regard to pixel order in adjacencies
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window; G-S diversity is zero when only one value
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * Pij
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * Pij
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						// avoid double-counting diagonal elements
						if(temp_int == temp_int2) {
							temp_int3 = (*(freqptr2 + temp_int));
						}
						else {
							temp_int3 = (*(freqptr2 + temp_int)) + (*(freqptr2 + temp_int2));	// Nij + Nji
						}
						if(temp_int3 > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int3);	// either Pii or Pij + Pji
							numerator += (temp_float * temp_float);
						}
					}
				}
				temp_out = 1.0 - numerator;  // G-S diversity is one minus the sum in the numerator
				metric_value = roundf(temp_out * 100000) / 100000;
				break;	
			}
			if(parameters.exclude_zero == 1){	// 1015 Gini-Simpson diversity of adjacency matrix, excluding pixel value 0, without regard to pixel order in adjacencies
				// No need to know the number of different byte values in the window; G-S diversity is zero when only one value
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * Pij
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * Pij
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes 0 and missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						// avoid double-counting diagonal elements
						if(temp_int == temp_int2) {
							temp_int3 = (*(freqptr2 + temp_int));
						}
						else {
							temp_int3 = (*(freqptr2 + temp_int)) + (*(freqptr2 + temp_int2));	// Nij + Nji
						}
						if(temp_int3 > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int3);	// either Pii or Pij + Pji
							numerator += (temp_float * temp_float);
						}
					}
				}
				temp_out = 1.0 - numerator;  // G-S diversity is one minus the sum in the numerator
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 16:{	// Majority pixel value including pixel value 0
					// If there is a tie, the largest tied pixel value is returned
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// number of non-missing pixels
				temp_int = max_npix - (*(freqptr1 + 101)); 	// excludes missing
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				// seed with the number of pixels with value 0
				temp_max = (*(freqptr1 + 0));
				temp_index = 0;		// pixel value 0 has current max
				for(index = 1; index < 101; index++) {	// excludes missing 
					temp_int = (*(freqptr1 + index));
					if(temp_int > temp_max) {
						temp_max = temp_int;
						temp_index = index;
					}
				}
				metric_value = 1.0 * temp_index;		
				break;
			}
			if(parameters.exclude_zero == 1){ 	// 1016 Majority pixel value; excluding pixel value 0
				// number of non-missing pixels
				temp_int = max_npix - (*(freqptr1 + 0)) - (*(freqptr1 + 101)); 	// excludes 0 and missing
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				// seed with the number of pixels with value 1
				temp_max = (*(freqptr1 + 1));
				temp_index = 1;		// pixel value 1 has current max
				for(index = 2; index < 101; index++) {	// excludes 0 and missing 
					temp_int = (*(freqptr1 + index));
					if(temp_int > temp_max) {
						temp_max = temp_int;
						temp_index = index;
					}
				}
				metric_value = 1.0 * temp_index;			
				break;
			}
		}
		case 17:{	// Landscape mosaic; 19 LM classes
					// Calculate tri-polar classification using input values 1, 2, and 3
					// 19-class version has thresholds on each axis at 0.1, 0.6, and 1.0
			metric_value = -0.01;
			// number of non-missing pixels
			temp_int = max_npix - (*(freqptr1 + 101)); 	
			if(temp_int == 0) {
				metric_value = -0.01;
				break;
			}
			else {
				inv_denominator = 1.0 / (1.0 * temp_int);
			}
			// Calculate percentages of each type
			p1 = inv_denominator * (1.0 *(*(freqptr1 + 1)));	// p1 is spatcon p_agr
			p2 = inv_denominator * (1.0 *(*(freqptr1 + 2)));	// p2 is spatcon p_for
			p3 = inv_denominator * (1.0 *(*(freqptr1 + 3)));	// p3 is spatcon p_dev
			// tri-polar classification of p1, p2, p3
			if(p2 >= 0.60){      
				if(p2 == 1.0){   
					metric_value = 17.;
					break;
				}
				if(p3 > p1){    
					if(p1 >= 0.10){
						metric_value = 12.;
						break;
					}
					if(p3 >= 0.10){
						metric_value = 9.; 
						break;
					}
				}
				else {
					if(p3 >= 0.10){
						metric_value = 12.;
						break;
					}
					if(p1 >= 0.10){
						metric_value = 8.; 
						break;
					}
				}
				metric_value = 3.; 
				break;
			} // end of if p2 > .6 
			if(p1 >= 0.60){
				if(p1 == 1.0){
					metric_value = 18.;
					break;
				}
				if(p3 > p2){
					if(p2 >= 0.10){
						metric_value = 10.; 
						break;
					}
					if(p3 >= 0.10){
						metric_value = 4.; 
						break;
					}
				}
				else{
					if(p3 >= 0.10){
						metric_value = 10.; 
						break;
					}
					if(p2 >= 0.10){
						metric_value = 5.;
						break;
					}
				}
				metric_value = 1.; 
				break;
			} // end of if p1 > .6
			if(p3 >= 0.60){
				if(p3 == 1.0){
					metric_value = 19.;
					break;
				}
				if(p2 > p1){
					if(p1 >= 0.10){
						metric_value = 11.;
						break;
					}
					if(p2 >= 0.10){
						metric_value = 6.;
						break;
					}
				}
				else{
					if(p2 >= 0.10){
						metric_value = 11.;
						break;
					}
					if(p1 >= 0.10){
						metric_value = 7.; 
						break;
					}
				}
				metric_value = 2.;
				break;
			} // end of if p3 > .6
			// None of the 3 classes was > 60%. Just find those >10%
			// Note that if only 1 class is > 10%, then it was handled as above
			if(p1 >= 0.10){
				if(p3 >= 0.10){
					if(p2 >= 0.10){
						metric_value = 16.;
						break;
					}
					else{
						metric_value = 13.;
						break;
					}
				}
				else{
					if(p2 >= 0.10){
						metric_value = 14.;
						break;
					}
				}
			}
			else{    // p1 is less than 10%; it must be the other two
				metric_value = 15.;
				break;
			}
		break;
		}
		case 18:{	// Landscape mosaic; 103 LM classes
					// Calculate tri-polar classification using input values 1, 2, and 3
					// 103-class version has thresholds on each axis at 0.1 intervals
			metric_value = -0.01;
			// number of non-missing pixels
			temp_int = max_npix - (*(freqptr1 + 101)); 	
			if(temp_int == 0) {
				metric_value = -0.01;
				break;
			}
			else {
				inv_denominator = 1.0 / (1.0 * temp_int);
			}
			// Calculate percentages of each type
			p1 = inv_denominator * (1.0 *(*(freqptr1 + 1)));	// p1 is spatcon p_agr
			p2 = inv_denominator * (1.0 *(*(freqptr1 + 2)));	// p2 is spatcon p_for
			p3 = inv_denominator * (1.0 *(*(freqptr1 + 3)));	// p3 is spatcon p_dev
			// check the corners first 
			if(p2 == 1.0){
				metric_value = 170.;
				break;
			}
			if(p3 == 1.0){
				metric_value = 190.;
				break;
			}
			if(p1 == 1.0){
				metric_value = 180.;
				break;
			}
			//work through the triangle starting at lower left. The use of "<" in if statements for p2 and p1,
			//   along with the >= in if statements for p3, ensure the threshold is like 'X% or more'
			if(p2 < 0.1){
				if(p1 < 0.1){
					if(p3 >= 0.9){
						metric_value = 191.;
						break;
					}
					metric_value = 192.;
					break;
				}
				if(p1 < 0.2){
					if(p3 >= 0.8){
						metric_value = 71.;
						break;
					}
					metric_value = 72.;
					break;
				}
				if(p1 < 0.3){
					if(p3 >= 0.7){
						metric_value = 73.;
						break;
					}
					metric_value = 74.;
					break;
				}
				if(p1 < 0.4){
					if(p3 >= 0.6){
						metric_value = 75.;
						break;
					}
					metric_value = 131.;
					break;
				}
				if(p1 < 0.5){
					if(p3 >= 0.5){
						metric_value = 132.;
						break;
					}
					metric_value = 133.;
					break;
				}
				if(p1 < 0.6){
					if(p3 >= 0.4){
						metric_value = 134.;
						break;
					}
					metric_value = 135.;
					break;
				}
				if(p1 < 0.7){
					if(p3 >= 0.3){
						metric_value = 45.;
						break;
					}
					metric_value = 44.;
					break;
				}
				if(p1 < 0.8){
					if(p3 >= 0.2){
						metric_value = 43.;
						break;
					}
					metric_value = 42.;
					break;
				}
				if(p1 < 0.9){
					if(p3 >= 0.1){
						metric_value = 41.;
						break;
					}
					metric_value = 182.;
					break;
				}
				metric_value = 181.;
				break;
			}
			if(p2 < 0.2){
				if(p1 < 0.1){
					if(p3 >= 0.8){
						metric_value = 61.;
						break;
					}
					metric_value = 62.;
					break;
				}
				if(p1 < 0.2){
					if(p3 >= 0.7){
						metric_value = 111.;
						break;
					}
					metric_value = 112.;
					break;
				}
				if(p1 < 0.3){
					if(p3 >= 0.6){
						metric_value = 114.;
						break;
					}
					metric_value = 200.;
					break;
				}
				if(p1 < 0.4){
					if(p3 >= 0.5){
						metric_value = 201.;
						break;
					}
					metric_value = 202.;
					break;
				}
				if(p1 < 0.5){
					if(p3 >= 0.4){
						metric_value = 203.;
						break;
					}
					metric_value = 204.;
					break;
				}
				if(p1 < 0.6){
					if(p3 >= 0.3){
						metric_value = 205.;
						break;
					}
					metric_value = 206.;
					break;
				}
				if(p1 < 0.7){
					if(p3 >= 0.2){
						metric_value = 103.;
						break;
					}
					metric_value = 102.;
					break;
				}
				if(p1 < 0.8){
					if(p3 >= 0.1){
						metric_value = 101.;
						break;
					}
					metric_value = 52.;
					break;
				}
				metric_value = 51.;
				break;
			}
			if(p2 < 0.3){
				if(p1 < 0.1){
					if(p3 >= 0.7){
						metric_value = 63.;
						break;
					}
					metric_value = 64.;
					break;
				}
				if(p1 < 0.2){
					if(p3 >= 0.6){
						metric_value = 113.;
						break;
					}
					metric_value = 222.;
					break;
				}
				if(p1 < 0.3){
					if(p3 >= 0.5){
						metric_value = 223.;
						break;
					}
					metric_value = 224.;
					break;
				}
				if(p1 < 0.4){
					if(p3 >= 0.4){
						metric_value = 225.;
						break;
					}
					metric_value = 226.;
					break;
				}
				if(p1 < 0.5){
					if(p3 >= 0.3){
						metric_value = 227.;
						break;
					}
					metric_value = 228.;
					break;
				}
				if(p1 < 0.6){
					if(p3 >= 0.2){
						metric_value = 207.;
						break;
					}
					metric_value = 208.;
					break;
				}
				if(p1 < 0.7){
					if(p3 >= 0.1){
						metric_value = 104.;
						break;
					}
					metric_value = 54.;
					break;
				}
				metric_value = 53.;
				break;
			}
			if(p2 < 0.4){
				if(p1 < 0.1){
					if(p3 >= 0.6){
						metric_value = 65.;
						break;
					}
					metric_value = 155.;
					break;
				}
				if(p1 < 0.2){
					if(p3 >= 0.5){
						metric_value = 221.;
						break;
					}
					metric_value = 220.;
					break;
				}
				if(p1 < 0.3){
					if(p3 >= 0.4){
						metric_value = 235.;
						break;
					}
					metric_value = 234.;
					break;
				}
				if(p1 < 0.4){
					if(p3 >= 0.3){
						metric_value = 236.;
						break;
					}
					metric_value = 230.;
					break;
				}
				if(p1 < 0.5){
					if(p3 >= 0.2){
						metric_value = 229.;
						break;
					}
					metric_value = 210.;
					break;
				}
				if(p1 < 0.6){
					if(p3 >= 0.1){
						metric_value = 209.;
						break;
					}
					metric_value = 141.;
					break;
				}
				metric_value = 55.;
				break;
			}
			if(p2 < 0.5){
				if(p1 < 0.1){
					if(p3 >= 0.5){
						metric_value = 154.;
						break;
					}
					metric_value = 153.;
					break;
				}
				if(p1 < 0.2){
					if(p3 >= 0.4){
						metric_value = 219.;
						break;
					}
					metric_value = 218.;
					break;
				}
				if(p1 < 0.3){
					if(p3 >= 0.3){
						metric_value = 233.;
						break;
					}
					metric_value = 232.;
					break;
				}
				if(p1 < 0.4){
					if(p3 >= 0.2){
						metric_value = 231.;
						break;
					}
					metric_value = 212.;
					break;
				}
				if(p1 < 0.5){
					if(p3 >= 0.1){
						metric_value = 211.;
						break;
					}
					metric_value = 143.;
					break;
				}
				metric_value = 142.;
				break;
			}
			if(p2 < 0.6){
				if(p1 < 0.1){
					if(p3 >= 0.4){
						metric_value = 152.;
						break;
					}
					metric_value = 151.;
					break;
				}
				if(p1 < 0.2){
					if(p3 >= 0.3){
						metric_value = 217.;
						break;
					}
					metric_value = 216.;
					break;
				}
				if(p1 < 0.3){
					if(p3 >= 0.2){
						metric_value = 215.;
						break;
					}
					metric_value = 214.;
					break;
				}
				if(p1 < 0.4){
					if(p3 >= 0.1){
						metric_value = 213.;
						break;
					}
					metric_value = 145.;
					break;
				}
				metric_value = 144.;
				break;
			}
			if(p2 < 0.7){
				if(p1 < 0.1){
					if(p3 >= 0.3){
						metric_value = 95.;
						break;
					}
					metric_value = 94.;
					break;
				}
				if(p1 < 0.2){
					if(p3 >= 0.2){
						metric_value = 124.;
						break;
					}
					metric_value = 122.;
					break;
				}
				if(p1 < 0.3){
					if(p3 >= 0.1){
						metric_value = 123.;
						break;
					}
					metric_value = 84.;
					break;
				}
				metric_value = 85.;
				break;
			}
			if(p2 < 0.8){
				if(p1 < 0.1){
					if(p3 >= 0.2){
						metric_value = 93.;
						break;
					}
					metric_value = 92.;
					break;
				}
				if(p1 < 0.2){
					if(p3 >= 0.1){
						metric_value = 121.;
						break;
					}
					metric_value = 82.;
					break;
				}
				metric_value = 83.;
				break;
			}
			if(p2 < 0.9){
				if(p1 < 0.1){
					if(p3 >= 0.1){
						metric_value = 91.;
						break;
					}
					metric_value = 172.;
					break;
				}
				metric_value = 81.;
				break;
			}
			metric_value = 171.;
			break;
		}
		case 19:{	// Number of unique pixel values, including pixel value 0
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// number of non-missing pixels
				temp_int = max_npix - (*(freqptr1 + 101)); 	// excludes missing
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				// the number of unique byte values in the window, excluding missing
				num_values = 0;
				for(index = 0; index < 101; index++) {	// excludes missing values
					if( (*(freqptr1 + index)) > 0 ) {
						num_values++;
					}
				}
				metric_value = 1.0 * num_values;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1019 Number of unique pixel values, excluding pixel value 0
				// number of non-missing pixels
				temp_int = max_npix - (*(freqptr1 + 0)) - (*(freqptr1 + 101)); 	// excludes 0 and missing
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				// the number of unique byte values in the window, excluding missing and zero
				num_values = 0;
				for(index = 1; index < 101; index++) {	// excludes missing values
					if( (*(freqptr1 + index)) > 0 ) {
						num_values++;
					}
				}
				metric_value = 1.0 * num_values;
				break;
			}
		}
		case 20:{	// Dominance_pixel, including pixel value 0
					// This is also known as Pmax
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// number of non-missing pixels
				temp_int = max_npix - (*(freqptr1 + 101)); 	
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * temp_int);
				}
				// seed with the number of pixels with value 0
				temp_max = (*(freqptr1 + 0));
				for(index = 1; index < 101; index++) {	// excludes missing 
					temp_int = (*(freqptr1 + index));
					if(temp_int > temp_max) {
						temp_max = temp_int;
					}
				}
				temp_out = inv_denominator * (1.0 * temp_max);
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1020 Dominance_pixel, excluding pixel value 0	
			// number of non-missing pixels
				temp_int = max_npix - (*(freqptr1 + 0)) - (*(freqptr1 + 101)); // excludes zero and missing	
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * temp_int);
				}
				// seed with the number of pixels with value 1
				temp_max = (*(freqptr1 + 1));
				for(index = 2; index < 101; index++) {	// excludes missing and zero
					temp_int = (*(freqptr1 + index));
					if(temp_int > temp_max) {
						temp_max = temp_int;
					}
				}
				temp_out = inv_denominator * (1.0 * temp_max);
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 21:{	// Area density, including pixel value 0
					// The proportion of non-missing pixels which are a specific pixel value (target code 1)
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// number of non-missing pixels
				temp_int = max_npix - (*(freqptr1 + 101)); 	
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * temp_int);
				}
				temp_int = (*(freqptr1 + parameters.target_code_1));
				temp_out = inv_denominator * (1.0 * temp_int);
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1021 Area density, excluding pixel value 0
				// number of non-missing pixels
				temp_int = max_npix - (*(freqptr1 + 0)) - (*(freqptr1 + 101)); 	
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * temp_int);
				}
				temp_int = (*(freqptr1 + parameters.target_code_1));
				temp_out = inv_denominator * (1.0 * temp_int);
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 22:{	// Focal adjacency_1: Proportion of total adjacencies, including 0, which involve one specific pixel value.
					// Target_code_1 is the one specific pixel value
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				temp_int2 = 0; // accumulator for number of adjacencies
				/* accumulate across code_1's row in the adjacency matrix */
				for(index = 0; index < 101; index++){		// excludes missing
					temp_int = (parameters.target_code_1 * 102) + index;
					temp_int2 += (*(freqptr2 + temp_int));
				}
				/* accumulate across code_1's column in the adjacency matrix */
				for(index = 0; index < 101; index++){		// excludes missing
					temp_int = (index * 102) + parameters.target_code_1;
					temp_int2 += (*(freqptr2 + temp_int));
				}
				/* the diagonal cell has been counted twice, so subtract one of them */
				temp_int = (parameters.target_code_1 * 102) + parameters.target_code_1;
				temp_int2 -= (*(freqptr2 + temp_int));
				temp_out = inv_num_adjacencies * (1.0 * temp_int2);
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1022 Focal adjacency_2: Proportion of total adjacencies, excluding 0, which involve one specific pixel value.
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				temp_int2 = 0; // accumulator for number of adjacencies
				/* accumulate across code_1's row in the adjacency matrix */
				for(index = 1; index < 101; index++){		// excludes zero and missing
					temp_int = (parameters.target_code_1 * 102) + index;
					temp_int2 += (*(freqptr2 + temp_int));
				}
				/* accumulate across code_1's column in the adjacency matrix */
				for(index = 1; index < 101; index++){		// excludes zero and missing
					temp_int = (index * 102) + parameters.target_code_1;
					temp_int2 += (*(freqptr2 + temp_int));
				}
				/* the diagonal cell has been counted twice, so subtract one of them */
				temp_int = (parameters.target_code_1 * 102) + parameters.target_code_1;
				temp_int2 -= (*(freqptr2 + temp_int));
				temp_out = inv_num_adjacencies * (1.0 * temp_int2);
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 23:{	// Focal adjacency_2: Proportion of total adjacencies, including 0, which involve two specific pixel values.
					// Target_code_1 and target_code_2 are the two specific pixel values
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				temp_int2 = 0; // accumulator for number of adjacencies
				temp_int = (parameters.target_code_1 * 102) + parameters.target_code_2;
				temp_int2 += (*(freqptr2 + temp_int));
				if(parameters.target_code_1 != parameters.target_code_2){ 		// don't count the diagonal twice 
					temp_int = (parameters.target_code_2 * 102) + parameters.target_code_1;
					temp_int2 += (*(freqptr2 + temp_int));
				}
				temp_out = inv_num_adjacencies * (1.0 * temp_int2);
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1023 Focal adjacency_4: Proportion of total adjacencies, excluding 0, which involve two specific pixel values.
			// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				temp_int2 = 0; // accumulator for number of adjacencies
				temp_int = (parameters.target_code_1 * 102) + parameters.target_code_2;
				temp_int2 += (*(freqptr2 + temp_int));
				if(parameters.target_code_1 != parameters.target_code_2){ 		// don't count the diagonal twice 
					temp_int = (parameters.target_code_2 * 102) + parameters.target_code_1;
					temp_int2 += (*(freqptr2 + temp_int));
				}
				temp_out = inv_num_adjacencies * (1.0 * temp_int2);
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 24:{	// Focal adjacency_3: Proportion of total adjacencies for one specific pixel value, including 0, which involve a second specific pixel value. 
					// Target_code_1 and target_code_2 are the two specific pixel values
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// count the adjacencies involving target_code_1
				temp_int2 = 0; // accumulator for number of adjacencies
				/* accumulate across code_1's row in the adjacency matrix */
				for(index = 0; index < 101; index++){		// excludes missing
					temp_int = (parameters.target_code_1 * 102) + index;
					temp_int2 += (*(freqptr2 + temp_int));
				}
				/* accumulate across code_1's column in the adjacency matrix */
				for(index = 0; index < 101; index++){		// excludes missing
					temp_int = (index * 102) + parameters.target_code_1;
					temp_int2 += (*(freqptr2 + temp_int));
				}
				/* the diagonal cell has been counted twice, so subtract one of them */
				temp_int = (parameters.target_code_1 * 102) + parameters.target_code_1;
				temp_int2 -= (*(freqptr2 + temp_int));
				// bug fix in 1.1.1
				if(temp_int2 == 0) {  // no adjacencies for target code 1
					metric_value = -0.01;
					break;
				}
				//
				inv_num_adjacencies = 1.0 / (1.0 * temp_int2);  // divisor is the number of non-missing adjacencies for target code 1
				// count the adjacencies involving both target codes
				temp_int2 = 0; // accumulator for number of adjacencies
				temp_int = (parameters.target_code_1 * 102) + parameters.target_code_2;
				temp_int2 += (*(freqptr2 + temp_int));
				if(parameters.target_code_1 != parameters.target_code_2){ 		// don't count the diagonal twice 
					temp_int = (parameters.target_code_2 * 102) + parameters.target_code_1;
					temp_int2 += (*(freqptr2 + temp_int));
				}
				temp_out = inv_num_adjacencies * (1.0 * temp_int2);
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1024 Focal adjacency_6: Proportion of total adjacencies for one specific pixel value, excluding 0, which involve a second specific pixel value. 
				// count the adjacencies involving target_code_1
				temp_int2 = 0; // accumulator for number of adjacencies
				/* accumulate across code_1's row in the adjacency matrix */
				for(index = 1; index < 101; index++){		// excludes zero and missing
					temp_int = (parameters.target_code_1 * 102) + index;
					temp_int2 += (*(freqptr2 + temp_int));
				}
				/* accumulate across code_1's column in the adjacency matrix */
				for(index = 1; index < 101; index++){		// excludes zero and missing
					temp_int = (index * 102) + parameters.target_code_1;
					temp_int2 += (*(freqptr2 + temp_int));
				}
				/* the diagonal cell has been counted twice, so subtract one of them */
				temp_int = (parameters.target_code_1 * 102) + parameters.target_code_1;
				temp_int2 -= (*(freqptr2 + temp_int));
				// bug fix in 1.1.1
				if(temp_int2 == 0) {  // no adjacencies for target code 1
					metric_value = -0.01;
					break;
				}
				//
				inv_num_adjacencies = 1.0 / (1.0 * temp_int2);  // divisor is the number of non-missing adjacencies for target code 1
				// count the adjacencies involving both target codes
				temp_int2 = 0; // accumulator for number of adjacencies
				temp_int = (parameters.target_code_1 * 102) + parameters.target_code_2;
				temp_int2 += (*(freqptr2 + temp_int));
				if(parameters.target_code_1 != parameters.target_code_2){ 		// don't count the diagonal twice 
					temp_int = (parameters.target_code_2 * 102) + parameters.target_code_1;
					temp_int2 += (*(freqptr2 + temp_int));
				}
				temp_out = inv_num_adjacencies * (1.0 * temp_int2);
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 25:{	// Population standard deviation of pixel values, including 0
					// calculated here the efficient way without first calculating the mean
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// number of non-missing pixels
				numpix = max_npix - (*(freqptr1 + 101)); 	
				if(numpix == 0) {
					metric_value = -0.01;
					break;
				}
				else{
					inv_denominator = 1.0 / (1.0 * numpix);
				}
				// 1.1.1
				sumx = 0;
				for(index = 0; index < 101; index++) {	// includes 0, excludes missing 
					sumx += (*(freqptr1 + index)) * index;
				}
				mu_x = inv_denominator * (1.0 * sumx);
				temp_float2 = 0.0;
				for(index = 0; index < 101; index++) {	// includes 0, excludes missing
					temp_float = 1.0 * index;
					temp_float2 += (1.0 * (*(freqptr1 + index)) ) * ( (temp_float - mu_x) * (temp_float - mu_x) );
				}
				temp_float = inv_denominator * temp_float2;
				temp_out = sqrt(temp_float);
				metric_value = roundf(temp_out * 100000) / 100000;
				//
				break;
			}
			if(parameters.exclude_zero == 1){	// 1025 Population standard deviation of pixel values, excluding 0
				// number of non-missing pixels, excluding zero
				numpix = max_npix - (*(freqptr1 + 0)) - (*(freqptr1 + 101)); 	
				if(numpix == 0) {
					metric_value = -0.01;
					break;
				}
				else{
					inv_denominator = 1.0 / (1.0 * numpix);
				}
				// 1.1.1
				sumx = 0;
				for(index = 1; index < 101; index++) {	// excludes 0, excludes missing 
					sumx += (*(freqptr1 + index)) * index;
				}
				mu_x = inv_denominator * (1.0 * sumx);
				temp_float2 = 0.0;
				for(index = 1; index < 101; index++) {	// excludes 0, excludes missing
					temp_float = 1.0 * index;
					temp_float2 += (1.0 * (*(freqptr1 + index)) ) * ( (temp_float - mu_x) * (temp_float - mu_x) );
				}
				temp_float = inv_denominator * temp_float2;
				temp_out = sqrt(temp_float);
				metric_value = roundf(temp_out * 100000) / 100000;
				//
				break;
			}
		}
		case 26:{	// Population coefficient of variation of pixel values, including 0
					// calculate SD, divide by mean, multiply by 100
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// number of non-missing pixels
				numpix = max_npix - (*(freqptr1 + 101)); 	
				if(numpix == 0) {
					metric_value = -0.01;
					break;
				}
				else{
					inv_denominator = 1.0 / (1.0 * numpix);
				}
				// 1.1.1
				sumx = 0;
				for(index = 0; index < 101; index++) {	// includes 0, excludes missing 
					sumx += (*(freqptr1 + index)) * index;
				}
				mu_x = inv_denominator * (1.0 * sumx);
				mu_x = roundf(mu_x * 100000) / 100000; // round to 5 digits for comparision to zero
				if(mu_x == 0.0){
					metric_value = -0.01;
					break;
				}
				temp_float2 = 0.0; // sum of (x-mu) * (x-mu)
				for(index = 0; index < 101; index++) {	// includes 0, excludes missing
					temp_float = 1.0 * index;
					temp_float2 += (1.0 * (*(freqptr1 + index)) ) * ( (temp_float - mu_x) * (temp_float - mu_x) );
				}
				temp_float = inv_denominator * temp_float2; // divide by N
				sigma_x = sqrt(temp_float);
				temp_out = 100.0 * (sigma_x / mu_x);
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
				//
			}
			if(parameters.exclude_zero == 1){	// 1026 Population coefficient of variation of pixel values, excluding 0
				// number of non-missing pixels, excluding zero
				numpix = max_npix - (*(freqptr1 + 0)) - (*(freqptr1 + 101)); 	
				if(numpix == 0) {
					metric_value = -0.01;
					break;
				}
				else{
					inv_denominator = 1.0 / (1.0 * numpix);
				}
				// 1.1.1
				sumx = 0;
				for(index = 1; index < 101; index++) {	// excludes 0, excludes missing 
					sumx += (*(freqptr1 + index)) * index;
				}
				mu_x = inv_denominator * (1.0 * sumx);
				mu_x = roundf(mu_x * 100000) / 100000; // round to 5 digits for comparision to zero
				if(mu_x == 0.0){
					metric_value = -0.01;
					break;
				}
				temp_float2 = 0.0; // sum of (x-mu) * (x-mu)
				for(index = 1; index < 101; index++) {	// excludes 0, excludes missing
					temp_float = 1.0 * index;
					temp_float2 += (1.0 * (*(freqptr1 + index)) ) * ( (temp_float - mu_x) * (temp_float - mu_x) );
				}
				temp_float = inv_denominator * temp_float2; // divide by N
				sigma_x = sqrt(temp_float);
				temp_out = 100.0 * (sigma_x / mu_x);
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
				//
			}
		}
		case 27:{	// Range of pixel values, including 0 (max - min)
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				min = 0;
				max = 0;
				for(index = 0; index < 101; index++) {	// scan to find first non-zero frequency, excludes missing
					if( (*(freqptr1 + index)) > 0) {
						min = index;		// this is the minimum pixel value
						break;
					}
				}
				for(index = 100; index >= 0; index--) {	// now re-scan for the maximum
					if( (*(freqptr1 + index)) > 0) {
						max = index;
						break;
					}
				}
				metric_value = max - min;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1027 Range of pixel values, excluding 0 (max - min)
				min = 0;
				max = 0;
				for(index = 1; index < 101; index++) {	// scan to find first non-zero frequency, excludes zero and missing
					if( (*(freqptr1 + index)) > 0) {
						min = index;		// this is the minimum pixel value
						break;
					}
				}
				for(index = 100; index >= 1; index--) {	// now re-scan for a  maximum, excludes zero and missing
					if( (*(freqptr1 + index)) > 0) {
						max = index;
						break;
					}
				}
				metric_value = max - min;
				break;
			}
		}
		case 28:{ 	// Dissimilarity, including 0
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window; dissimilarity is zero when only one value
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * |i-j| as 'numerator'
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * |i-j|
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						if(temp_int > 0) { 	
							temp_int2 = abs(index1 - index2);	// |i-j|
							temp_float = inv_num_adjacencies * temp_int;	// Pij
							numerator += (temp_float *  temp_int2);
						}
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){ 	// 1028 Dissimilarity, excluding 0
				// No need to know the number of different byte values in the window; dissimilarity is zero when only one value
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * |i-j| as 'numerator'
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * |i-j|
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes 0 and missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > 0) { 
							temp_int2 = abs(index1 - index2);	// |i-j|
							temp_float = inv_num_adjacencies * temp_int;	// Pij
							numerator += (temp_float *  temp_int2);
						}
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 29:{	// Contrast, including 0
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window; contrast is zero when only one value
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * (i-j)**2
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * (i-j)**2
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						if(temp_int > 0) { 	
							temp_int2 = (index1 - index2)*(index1 - index2);	// (i-j)**2
							temp_float = inv_num_adjacencies * temp_int;	// Pij
							numerator += (temp_float *  temp_int2);
						}
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1029 Contrast, excluding 0
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * (i-j)**2 as 'numerator'
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * (i-j)**2
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes 0 and missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > 0) { 
							temp_int2 = (index1 - index2)*(index1 - index2);	// (i-j)**2
							temp_float = inv_num_adjacencies * temp_int;	// Pij
							numerator += (temp_float *  temp_int2);
						}
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 30:{	// Uniformity of adjacency matrix, including pixel value 0, with regard to pixel order in adjacencies
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window; uniformity is one when only one value
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * Pij
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * Pij
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int);	// Pij
							numerator += (temp_float * temp_float);
						}
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1030 Uniformity of adjacency matrix, excluding pixel value 0, with regard to pixel order in adjacencies
				// No need to know the number of different byte values in the window; uniformity is one when only one value
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * Pij
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * Pij
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes 0 and missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int);	// Pij
							numerator += (temp_float * temp_float);
						}
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 31:{	// Uniformity of adjacency matrix, including pixel value 0, without regard to pixel order in adjacencies
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window; uniformity is one when only one value
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * Pij
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * Pij
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						// avoid double-counting diagonal elements
						if(temp_int == temp_int2) {
							temp_int3 = (*(freqptr2 + temp_int));
						}
						else {
							temp_int3 = (*(freqptr2 + temp_int)) + (*(freqptr2 + temp_int2));	// Nij + Nji
						}
						if(temp_int3 > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int3);	// either Pii or Pij + Pji
							numerator += (temp_float * temp_float);
						}
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;	
			}
			if(parameters.exclude_zero == 1){	// 1031 Uniformity of adjacency matrix, excluding pixel value 0, without regard to pixel order in adjacencies
				// No need to know the number of different byte values in the window; uniformity is one when only one value
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * Pij
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * Pij
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes 0 and missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						// avoid double-counting diagonal elements
						if(temp_int == temp_int2) {
							temp_int3 = (*(freqptr2 + temp_int));
						}
						else {
							temp_int3 = (*(freqptr2 + temp_int)) + (*(freqptr2 + temp_int2));	// Nij + Nji
						}
						if(temp_int3 > 0) { 	
							temp_float = inv_num_adjacencies * (1.0 * temp_int3);	// either Pii or Pij + Pji
							numerator += (temp_float * temp_float);
						}
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 32:{	// Homogeneity, including 0
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window; homogeneity is one when only one value
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij /[1+(i-j)**2] as 'numerator'
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij /[1+(i-j)**2]
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						if(temp_int > 0) { 	
							temp_float = inv_num_adjacencies * temp_int;		// Pij
							temp_int2 = (index1 - index2)*(index1 - index2);	// (i-j)**2
							denominator = 1.0 + (1.0 * temp_int2);				// 1+(i-j)**2
							numerator += (temp_float / denominator);
						}
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1032 Homogeneity, excluding 0
				// No need to know the number of different byte values in the window; homogeneity is one when only one value
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij /[1+(i-j)**2] as 'numerator'
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of thePij /[1+(i-j)**2]
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes 0 and missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > 0) { 
							temp_float = inv_num_adjacencies * temp_int;		// Pij
							temp_int2 = (index1 - index2)*(index1 - index2);	// (i-j)**2
							denominator = 1.0 + (1.0 * temp_int2);				// 1+(i-j)**2
							numerator += (temp_float / denominator);
						}
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 33:{	// Inverse difference, including 0
					// Clausi (2002). Computing formula in Lofstedt. Indept of pixel order. Inverse difference = sum Pij/(1+|i-j|)
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window; inverse difference is one when only one value
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij/(1+|i-j|) as 'numerator'
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij/(1+|i-j|)
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						if(temp_int > 0) { 	
							temp_float = inv_num_adjacencies * temp_int;		// Pij
							temp_int2 = abs(index1 - index2);					// |i-j|
							denominator = 1.0 + (1.0 * temp_int2);				// 1+|i-j|
							numerator += (temp_float / denominator);
						}
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1033 Inverse difference, excluding 0
				// No need to know the number of different byte values in the window; inverse difference is one when only one value
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij/(1+|i-j|) as 'numerator'
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * |i-j|
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes 0 and missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > 0) { 
							temp_float = inv_num_adjacencies * temp_int;		// Pij
							temp_int2 = abs(index1 - index2);					// |i-j|
							denominator = 1.0 + (1.0 * temp_int2);				// 1+|i-j|
							numerator += (temp_float / denominator);
						}
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 34:{	// Similarity_1, including 0.
					// Similarity_1 = 1 - sum( Pij * { |i-j| / R1 } )	where R1 = 100 is the maximum possible range
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window; Similarity_1 is one when only one value
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * |i-j| * 0.01 as 'numerator'
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * |i-j| * 0.01
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						if(temp_int > 0) { 	
							temp_int2 = abs(index1 - index2);	// |i-j|
							temp_float = inv_num_adjacencies * temp_int;	// Pij
							temp_float2 = 0.01 * temp_int2;		// |i-j|/100
							numerator += temp_float *  temp_float2;
						}
					}
				}
				temp_out = 1.0 - numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1034 Similarity_1, excluding 0.
				// No need to know the number of different byte values in the window; similarity_1 is one when only one value
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * |i-j| / 100 as 'numerator'
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * |i-j|
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes 0 and missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > 0) { 
							temp_int2 = abs(index1 - index2);	// |i-j|
							temp_float = inv_num_adjacencies * temp_int;	// Pij
							temp_float2 = 0.01 * temp_int2;		// |i-j|/100
							numerator += temp_float *  temp_float2;
						}
					}
				}
				temp_out = 1.0 - numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 35:{ 	// Similarity_2, including 0.
					// = 1 - sum( Pij * { |i-j| / R2 } )	where R2 is the observed range in the entire image (control.global_range)
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				if(control.global_range == 0){	// undefined if range is 0
					break;
				}
				inv_denominator = 1.0 / (1.0 * control.global_range);
				// No need to know the number of different byte values in the window. 
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * (|i-j|/global_range) as 'numerator'
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * (|i-j|/global_range)
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						if(temp_int > 0) { 	
							temp_int2 = abs(index1 - index2);	// |i-j|
							temp_float = inv_num_adjacencies * temp_int;	// Pij
							temp_float2 = inv_denominator * temp_int2;		// |i-j|/global_range
							numerator += temp_float *  temp_float2;
						}
					}
				}
				temp_out = 1.0 - numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1035 Similarity_2, excluding 0.
				if(control.global_range == 0){	// undefined if range is 0
					break;
				}
				inv_denominator = 1.0 / (1.0 * control.global_range);
				// No need to know the number of different byte values in the window.
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * (|i-j|/global_range) as 'numerator'
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * (|i-j|/global_range)
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes 0 and missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > 0) { 
							temp_int2 = abs(index1 - index2);	// |i-j|
							temp_float = inv_num_adjacencies * temp_int;	// Pij
							temp_float2 = inv_denominator * temp_int2;		// |i-j|/global_range
							numerator += temp_float *  temp_float2;
						}
					}
				}
				temp_out = 1.0 - numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 36:{ 	// Similarity_3, including 0.
					// = 1 - sum( Pij * { |i-j| / R3 } )	where R3 is the observed range in the window
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// find the min, max, and range in the window
				min = 0;
				max = 0;
				for(index = 0; index < 101; index++) {	// scan to find first non-zero frequency, excludes missing
					if( (*(freqptr1 + index)) > 0) {
						min = index;		// this is the minimum pixel value
						break;
					}
				}
				for(index = 100; index >= 0; index--) {	// now re-scan for the maximum
					if( (*(freqptr1 + index)) > 0) {
						max = index;
						break;
					}
				}
				range = max - min;
				if(range == 0){		// return missing if range is zero
					break;
				}
				inv_denominator = 1.0 / (1.0 * range);
				// No need to know the number of different byte values in the window. 
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * (|i-j|/range) as 'numerator'
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * (|i-j|/range)
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						if(temp_int > 0) { 	
							temp_int2 = abs(index1 - index2);	// |i-j|
							temp_float = inv_num_adjacencies * temp_int;	// Pij
							temp_float2 = inv_denominator * temp_int2;		// |i-j|/range
							numerator += temp_float *  temp_float2;
						}
					}
				}
				temp_out = 1.0 - numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){ 	// 1036 Similarity_3, excluding 0.
												// = 1 - sum( Pij * { |i-j| / R3 } )	where R3 is the observed range in the window
				// find the min, max, and range in the window
				min = 0;
				max = 0;
				for(index = 1; index < 101; index++) {	// scan to find first non-zero frequency, excludes zero and missing
					if( (*(freqptr1 + index)) > 0) {
						min = index;		// this is the minimum pixel value
						break;
					}
				}
				for(index = 100; index >= 1; index--) {	// now re-scan for a  maximum, excludes zero and missing
					if( (*(freqptr1 + index)) > 0) {
						max = index;
						break;
					}
				}
				range = max - min;
				if(range == 0){		// return missing if range is zero
					break;
				}
				inv_denominator = 1.0 / (1.0 * range);
				// No need to know the number of different byte values in the window.
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * (|i-j|/range) as 'numerator'
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * (|i-j|/range)
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes 0 and missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > 0) { 
							temp_int2 = abs(index1 - index2);	// |i-j|
							temp_float = inv_num_adjacencies * temp_int;	// Pij
							temp_float2 = inv_denominator * temp_int2;		// |i-j|/range
							numerator += temp_float *  temp_float2;
						}
					}
				}
				temp_out = 1.0 - numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 37:{	// Dominance_adjacency, including 0, with regard to pixel order
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				// 
				temp_max = 0;
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > temp_max) { 	
							temp_max = temp_int;
						}
					}
				}
				temp_out = inv_num_adjacencies * temp_max;
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1037 Dominance_adjacency, excluding 0, with regard to pixel order
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				// 
				temp_max = 0;
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes 0 and missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > temp_max) { 	
							temp_max = temp_int;
						}
					}
				}
				temp_out = inv_num_adjacencies * temp_max;
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 38:{	// Dominance_adjacency, including 0, without regard to pixel order
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				//
				temp_max = 0;
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						// avoid double-counting diagonal elements
						if(temp_int == temp_int2) {
							temp_int3 = (*(freqptr2 + temp_int));
						}
						else {
							temp_int3 = (*(freqptr2 + temp_int)) + (*(freqptr2 + temp_int2));	// Nij + Nji
						}
						if(temp_int3 > temp_max) { 	
							temp_max = temp_int3;
						}
					}
				}
				temp_out = inv_num_adjacencies * temp_max;
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1038 Dominance_adjacency, excluding 0, without regard to pixel order
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				// 
				temp_max = 0;
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = index1; index2 < 101; index2++) {	// excludes 0 and missing values
						temp_int = (index1 * 102) + index2;
						temp_int2 = (index2 * 102) + index1;
						// avoid double-counting diagonal elements
						if(temp_int == temp_int2) {
							temp_int3 = (*(freqptr2 + temp_int));
						}
						else {
							temp_int3 = (*(freqptr2 + temp_int)) + (*(freqptr2 + temp_int2));	// Nij + Nji
						}
						if(temp_int3 > temp_max) { 	
							temp_max = temp_int3;
						}
					}
				}
				temp_out = inv_num_adjacencies * temp_max;
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 39:{	// Difference entropy, including 0
					// not the same as Haralick 1973. see next metric
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				// malloc and zero out a frequency table for 101 possible values of |i-j|, from 0 to 100
				if( (karray = (long int *)calloc(101, sizeof(long int) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				for(index = 0; index < 101; index++){
					(*(karray+index)) = 0;
				}
				// populate the frequencies of |i-j| = k
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						kval = abs(index1 - index2);			// abs(i-j)
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						(*(karray + kval)) += temp_int;			// these are frequencies
					}
				}
				// calculate the difference entropy
				// count number of nonzero kvalues as num_values
				num_values = 0;	
				// accumulate Pk*log(Pk) as 'numerator'
				numerator = 0.0;
				for(kval = 0; kval < 101; kval++){
					temp_float = inv_num_adjacencies * (*(karray + kval)); // Pk
					if(temp_float > 0){
						num_values ++;
						numerator += temp_float * log(temp_float);
					}				
				}
				if(num_values == 1){		// only one kvalue, entropy is zero
					metric_value = 0.0;		// do this here to avoid getting negative zero in next step
				}
				else{
					temp_out = -1.0 * numerator;
					metric_value = roundf(temp_out * 100000) / 100000;
				}
				free(karray);			
				break;
			}
			if(parameters.exclude_zero == 1){	// 1039 Difference entropy, excluding 0
												// Haralick 1973. indept of pixel order; use Lofstedt equations
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				// malloc and zero out a frequency table for 101 possible values of |i-j|, from 0 to 100
				if( (karray = (long int *)calloc(101, sizeof(long int) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				for(index = 0; index < 101; index++){
					(*(karray+index)) = 0;
				}
				// populate the frequencies of |i-j| = k
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes 0 and missing values
						kval = abs(index1 - index2);			// abs(i-j)
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						(*(karray + kval)) += temp_int;			// these are frequencies
					}
				}
				// calculate the difference entropy
				// count number of nonzero kvalues as num_values
				num_values = 0;	
				// accumulate Pk*log(Pk) as 'numerator'
				numerator = 0.0;
				for(kval = 0; kval < 101; kval++){
					temp_float = inv_num_adjacencies * (*(karray + kval)); // Pk
					if(temp_float > 0){
						num_values ++;
						numerator += temp_float * log(temp_float);
					}				
				}
				if(num_values == 1){		// only one kvalue, entropy is zero
					metric_value = 0.0;		// do this here to avoid getting negative zero in next step
				}
				else{
					temp_out = -1.0 * numerator;
					metric_value = roundf(temp_out * 100000) / 100000;
				}
				free(karray);			
				break;
			}
		}
		case 40:{	// Difference evenness, including 0
					// new metric
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				// malloc and zero out a frequency table for 101 possible values of |i-j|, from 0 to 100
				if( (karray = (long int *)calloc(101, sizeof(long int) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				for(index = 0; index < 101; index++){
					(*(karray+index)) = 0;
				}
				// populate the frequencies of |i-j| = k
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						kval = abs(index1 - index2);			// abs(i-j)
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						(*(karray + kval)) += temp_int;			// these are frequencies
					}
				}
				// calculate the difference entropy
				// count number of nonzero kvalues as num_values
				num_values = 0;	
				// accumulate Pk*log(Pk) as 'numerator'
				numerator = 0.0;
				for(kval = 0; kval < 101; kval++){
					temp_float = inv_num_adjacencies * (*(karray + kval)); // Pk
					if(temp_float > 0){
						num_values ++;
						numerator += temp_float * log(temp_float);
					}				
				}
				if(num_values == 1){		// only one kvalue, evenness is missing
					metric_value = -0.01;		// missing is returned above if no adjacencies
				}
				else{
					numerator = -1.0 * numerator;
					denominator = log( (1.0 * num_values) ); // maximum numerator for given num_values
					temp_out = ( numerator / denominator );	// larger numbers indicate more "evenness"
					metric_value = roundf(temp_out * 100000) / 100000;
				}
				free(karray);			
				break;
			}
			if(parameters.exclude_zero == 1){	// 1040 Difference evenness, excluding 0
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				// malloc and zero out a frequency table for 101 possible values of |i-j|, from 0 to 100
				if( (karray = (long int *)calloc(101, sizeof(long int) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				for(index = 0; index < 101; index++){
					(*(karray+index)) = 0;
				}
				// populate the frequencies of |i-j| = k
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes 0 and missing values
						kval = abs(index1 - index2);			// abs(i-j)
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						(*(karray + kval)) += temp_int;			// these are frequencies
					}
				}
				// calculate the difference entropy
				// count number of nonzero kvalues as num_values
				num_values = 0;	
				// accumulate Pk*log(Pk) as 'numerator'
				numerator = 0.0;
				for(kval = 0; kval < 101; kval++){
					temp_float = inv_num_adjacencies * (*(karray + kval)); // Pk
					if(temp_float > 0){
						num_values ++;
						numerator += temp_float * log(temp_float);
					}				
				}
				if(num_values == 1){		// only one kvalue, evenness is one
					metric_value = -0.01;		// missing is returned above if no adjacencies
				}
				else{
					numerator = -1.0 * numerator;
					denominator = log( (1.0 * num_values) ); // maximum numerator for given num_values
					temp_out = ( numerator / denominator );	// larger numbers indicate more "evenness"
					metric_value = roundf(temp_out * 100000) / 100000;
				}
				free(karray);			
				break;
			}
		}
		case 41:{	// Sum entropy, including 0
					// 
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				// malloc and zero out a frequency table for 201 possible values of |i-j|, from 0 to 200
				if( (karray = (long int *)calloc(201, sizeof(long int) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				for(index = 0; index < 201; index++){
					(*(karray+index)) = 0;
				}
				// populate the frequencies of i+j = k
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						kval = index1 + index2;			// i+j
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						(*(karray + kval)) += temp_int;			// these are frequencies
					}
				}
				// calculate the sum entropy
				// count number of nonzero kvalues as num_values
				num_values = 0;	
				// accumulate Pk*log(Pk) as 'numerator'
				numerator = 0.0;
				for(kval = 0; kval < 201; kval++){
					temp_float = inv_num_adjacencies * (*(karray + kval)); // Pk
					if(temp_float > 0){
						num_values ++;
						numerator += temp_float * log(temp_float);
					}				
				}
				if(num_values == 1){		// only one kvalue, entropy is zero
					metric_value = 0.0;		// do this here to avoid getting negative zero in next step
				}
				else{
					temp_out = -1.0 * numerator;
					metric_value = roundf(temp_out * 100000) / 100000;
					}
				free(karray);			
				break;
			}
			if(parameters.exclude_zero == 1){	// 1041 Sum entropy, excluding 0
												// Haralick 1973. indept of pixel order; use Lofstedt equations
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				// malloc and zero out a frequency table for 201 possible values of i+j, from 0 to 200
				if( (karray = (long int *)calloc(201, sizeof(long int) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				for(index = 0; index < 201; index++){
					(*(karray+index)) = 0;
				}
				// populate the frequencies of i+j = k
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes 0 and missing values
						kval = index1 + index2;			// i+j
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						(*(karray + kval)) += temp_int;			// these are frequencies
					}
				}
				// calculate the sum entropy
				// count number of nonzero kvalues as num_values
				num_values = 0;	
				// accumulate Pk*log(Pk) as 'numerator'
				numerator = 0.0;
				for(kval = 0; kval < 201; kval++){
					temp_float = inv_num_adjacencies * (*(karray + kval)); // Pk
					if(temp_float > 0){
						num_values ++;
						numerator += temp_float * log(temp_float);
					}				
				}
				if(num_values == 1){		// only one kvalue, entropy is zero
					metric_value = 0.0;		// do this here to avoid getting negative zero in next step
				}
				else{
					temp_out = -1.0 * numerator;
					metric_value = roundf(temp_out * 100000) / 100000;
				}
				free(karray);			
				break;
			}
		}
		case 42:{	// Sum evenness, including 0
					// 
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				// malloc and zero out a frequency table for 201 possible values of |i-j|, from 0 to 200
				if( (karray = (long int *)calloc(201, sizeof(long int) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				for(index = 0; index < 201; index++){
					(*(karray+index)) = 0;
				}
				// populate the frequencies of i+j = k
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						kval = index1 + index2;			// i+j
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						(*(karray + kval)) += temp_int;			// these are frequencies
					}
				}
				// calculate the sum entropy
				// count number of nonzero kvalues as num_values
				num_values = 0;	
				// accumulate Pk*log(Pk) as 'numerator'
				numerator = 0.0;
				for(kval = 0; kval < 201; kval++){
					temp_float = inv_num_adjacencies * (*(karray + kval)); // Pk
					if(temp_float > 0){
						num_values ++;
						numerator += temp_float * log(temp_float);
					}				
				}
				if(num_values == 1){		// only one kvalue, evenness is missing
					metric_value = -0.01;		// missing is returned above if no adjacencies
				}
				else{
					numerator = -1.0 * numerator;
					denominator = log( (1.0 * num_values) ); // maximum numerator for given num_values
					temp_out = ( numerator / denominator );	// larger numbers indicate more "evenness"
					metric_value = roundf(temp_out * 100000) / 100000;
				}
				free(karray);			
				break;
			}
			if(parameters.exclude_zero == 1){	// 1042 Sum evenness, excluding 0
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				// malloc and zero out a frequency table for 201 possible values of i+j, from 0 to 200
				if( (karray = (long int *)calloc(201, sizeof(long int) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				for(index = 0; index < 201; index++){
					(*(karray+index)) = 0;
				}
				// populate the frequencies of i+j = k
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes 0 and missing values
						kval = index1 + index2;			// i+j
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						(*(karray + kval)) += temp_int;			// these are frequencies
					}
				}
				// calculate the sum entropy
				// count number of nonzero kvalues as num_values
				num_values = 0;	
				// accumulate Pk*log(Pk) as 'numerator'
				numerator = 0.0;
				for(kval = 0; kval < 201; kval++){
					temp_float = inv_num_adjacencies * (*(karray + kval)); // Pk
					if(temp_float > 0){
						num_values ++;
						numerator += temp_float * log(temp_float);
					}				
				}
				if(num_values == 1){		// only one kvalue, evenness is missing
					metric_value = -0.01;		// missing is returned above if no adjacencies
				}
				else{
					numerator = -1.0 * numerator;
					denominator = log( (1.0 * num_values) ); // maximum numerator for given num_values
					temp_out = ( numerator / denominator );	// larger numbers indicate more "evenness"
					metric_value = roundf(temp_out * 100000) / 100000;
				}
				free(karray);			
				break;
			}
		}
		case 43:{	// Autocorrelation, including 0.
					// Soh (1999). indept of pixel order. Sum( Pij * (i*j)
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * (i * j)
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * (i * j)
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						if(temp_int > 0) { 	
							temp_int2 = (index1 * index2);	// (i * j)
							temp_float = inv_num_adjacencies * temp_int;	// Pij
							numerator += (temp_float *  temp_int2);
						}
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;				
			}
			if(parameters.exclude_zero == 1){	// 1043 Autocorrelation, excluding 0
				// No need to know the number of different byte values in the window; 
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * (i * j) as 'numerator'
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * (i * j)
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes 0 and missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > 0) { 
							temp_int2 = (index1 * index2);	// (i * j)
							temp_float = inv_num_adjacencies * temp_int;	// Pij
							numerator += (temp_float *  temp_int2);
						}
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 44:{	// Correlation, including 0
					// Lofstedt computing formula
			metric_value = -9000000.0;	// note the exception to the missing value rule
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					break;	// returns missing
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				// malloc 2 frequency tables needed to store row and column marginal P's
				// for 101 possible values, from 0 to 100
				if( (rowp = (float *)calloc(101, sizeof(float) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				if( (colp = (float *)calloc(101, sizeof(float) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				// calc row variables
				mu_x = 0.0;
				sigma_x = 0.0; 	// calc as sigma squared, then take sqrt
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values  // index1 is row number
					nobs = 0;		// number of adjacencies in a row
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values // index2 is col number
						nobs += (*(freqptr2 + ((index1 * 102) + index2)) );	// number of adjacencies
					}
					(*(rowp + index1)) = inv_num_adjacencies * nobs;					// Px(i) 
					mu_x += (1.0*index1) * (*(rowp + index1));							// mux
				}
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values  // index1 is row number
					sigma_x += ((1.0*index1) - mu_x) * ((1.0*index1) - mu_x) * (*(rowp + index1)); // sigma squared = sum [(i-mu)**2 * Px(i)}
				}
				// 1.1.1
				sigma_x = sqrt(sigma_x);
				sigma_x = roundf(sigma_x * 100000) / 100000; // round to 5 digits for comparision to zero
				//
				// calc col variables
				mu_y = 0.0;
				sigma_y = 0.0; 	// calc as sigma squared, then take sqrt
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values  // index1 is col number
					nobs = 0;
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values // index2 is row number
						nobs += (*(freqptr2 + ((index2 * 102) + index1)));	// number of adjacencies // note switch of index1&2
					}
					(*(colp + index1)) = inv_num_adjacencies * nobs;	// Py(j)
					mu_y += (1.0*index1) * (*(colp + index1));							// muy
				}
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values  // index1 is row number
					sigma_y += ((1.0*index1) - mu_y) * ((1.0*index1) - mu_y) * (*(colp + index1)); // sigma squared = sum [(i-mu)**2 * Px(i)}
				}
				// 1.1.1
				sigma_y = sqrt(sigma_y);
				sigma_y = roundf(sigma_y * 100000) / 100000; // round to 5 digits for comparision to zero
				// avoid potential divide by zero
				if( (sigma_x == 0.0) || (sigma_y == 0.0) ){
					break; 	// returns missing
				}
				// accumulate the entire metric as 'numerator'
				numerator = 0.0; 		
				for(index1 = 0; index1 < 101; index1++) {		// excludes missing values // rows
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values // columns
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						temp_float = inv_num_adjacencies * temp_int;	// Pij
						temp_float2 = ( (1.0*index1) - mu_x ) / sigma_x; 	// (i-mux)/sigmax // note index1
						temp_float3 = ( (1.0*index2) - mu_y ) / sigma_y; 	// (i-muy)/sigmay // note index2
						numerator += temp_float * temp_float2 * temp_float3;
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				// the intervediate rounding above can result in +/-1.00001
				if(metric_value > 1.0) {metric_value = 1.0;}
				if(metric_value < -1.0) {metric_value = -1.0;}
				free(rowp);
				free(colp);
				break;
			}
			if(parameters.exclude_zero == 1){	// 1044 Correlation, excluding 0
					// Lofstedt computing formula
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					break;	// returns missing
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				// malloc 2 frequency tables needed to store row and column marginal P's
				// for 101 possible values, from 0 to 100
				if( (rowp = (float *)calloc(101, sizeof(float) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				if( (colp = (float *)calloc(101, sizeof(float) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				// calc row variables
				mu_x = 0.0;
				sigma_x = 0.0; 	// calc as sigma squared, then take sqrt
				for(index1 = 1; index1 < 101; index1++) {	// excludes zero and missing values  // index1 is row number
					nobs = 0;		// number of adjacencies in a row
					for(index2 = 1; index2 < 101; index2++) {	// excludes zero and missing values // index2 is col number
						nobs += (*(freqptr2 + ((index1 * 102) + index2)) );	// number of adjacencies
					}
					(*(rowp + index1)) = inv_num_adjacencies * nobs;					// Px(i) 
					mu_x += (1.0*index1) * (*(rowp + index1));							// mux
				}
				for(index1 = 1; index1 < 101; index1++) {	// excludes zero and missing values  // index1 is row number
					sigma_x += ((1.0*index1) - mu_x) * ((1.0*index1) - mu_x) * (*(rowp + index1)); // sigma squared = sum [(i-mu)**2 * Px(i)}
				}
				// 1.1.1
				sigma_x = sqrt(sigma_x);
				sigma_x = roundf(sigma_x * 100000) / 100000; // round to 5 digits for comparision to zero
				//
				// calc col variables
				mu_y = 0.0;
				sigma_y = 0.0; 	// calc as sigma squared, then take sqrt
				for(index1 = 1; index1 < 101; index1++) {	// excludes zero and missing values  // index1 is col number
					nobs = 0;
					for(index2 = 1; index2 < 101; index2++) {	// excludes zero and missing values // index2 is row number
						nobs += (*(freqptr2 + ((index2 * 102) + index1)));	// number of adjacencies // note switch of index1&2
					}
					(*(colp + index1)) = inv_num_adjacencies * nobs;	// Py(j)
					mu_y += (1.0*index1) * (*(colp + index1));							// muy
				}
				for(index1 = 1; index1 < 101; index1++) {	// excludes zero and missing values  // index1 is row number
					sigma_y += ((1.0*index1) - mu_y) * ((1.0*index1) - mu_y) * (*(colp + index1)); // sigma squared = sum [(i-mu)**2 * Px(i)}
				}
				// 1.1.1
				sigma_y = sqrt(sigma_y);
				sigma_y = roundf(sigma_y * 100000) / 100000; // round to 5 digits for comparision to zero
				// avoid potential divide by zero
				if( (sigma_x == 0.0) || (sigma_y == 0.0) ){
					break; 	// returns missing
				}
				// accumulate the entire metric as 'numerator'
				numerator = 0.0; 		
				for(index1 = 1; index1 < 101; index1++) {		// excludes zero and missing values // rows
					for(index2 = 1; index2 < 101; index2++) {	// excludes zero and missing values // columns
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						temp_float = inv_num_adjacencies * temp_int;	// Pij
						temp_float2 = ( (1.0*index1) - mu_x ) / sigma_x; 	// (i-mux)/sigmax // note index1
						temp_float3 = ( (1.0*index2) - mu_y ) / sigma_y; 	// (i-muy)/sigmay // note index2
						numerator += temp_float * temp_float2 * temp_float3;
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				if(metric_value > 1.0) {metric_value = 1.0;}
				if(metric_value < -1.0) {metric_value = -1.0;}
				free(rowp);
				free(colp);
				break;
			}
		}
		case 45:{	// Cluster shade, including 0
					// Formula in Soh 1999 attributes it to Conners 1982.  Don't use Lofstedt formula; it is for symmetric GLCMs
			metric_value = -9000000.0;	// note exception to missing value rule
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					break;	// returns missing
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				// malloc 2 frequency tables needed to store row and column marginal P's
				// for 101 possible values, from 0 to 100
				if( (rowp = (float *)calloc(101, sizeof(float) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				if( (colp = (float *)calloc(101, sizeof(float) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				// calc row variable
				mu_x = 0.0;
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values  // index1 is row number
					nobs = 0;		// number of adjacencies in a row
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values // index2 is col number
						nobs += (*(freqptr2 + ((index1 * 102) + index2)) );	// number of adjacencies
					}
					(*(rowp + index1)) = inv_num_adjacencies * nobs;					// Px(i) 
					mu_x += (1.0*index1) * (*(rowp + index1));							// mux
				}
				// calc col variable
				mu_y = 0.0;
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values  // index1 is col number
					nobs = 0;
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values // index2 is row number
						nobs += (*(freqptr2 + ((index2 * 102) + index1)));	// number of adjacencies // note switch of index1&2
					}
					(*(colp + index1)) = inv_num_adjacencies * nobs;	// Py(j)
					mu_y += (1.0*index1) * (*(colp + index1));							// muy
				}
				// accumulate the entire metric as 'numerator'
				numerator = 0.0; 		
				for(index1 = 0; index1 < 101; index1++) {		// excludes missing values // rows
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values // columns
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						temp_float = inv_num_adjacencies * temp_int;	// Pij
						temp_float2 = (1.0 * (index1 + index2)) - mu_x - mu_y; 
						temp_float3 = temp_float2 * temp_float2 * temp_float2; 	// first term
						numerator += temp_float * temp_float3;
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				free(rowp);
				free(colp);
				break;
			}
			if(parameters.exclude_zero == 1){	// 1045 Cluster shade, excluding 0
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					break;	// returns missing
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				// malloc 2 frequency tables needed to store row and column marginal P's
				// for 101 possible values, from 0 to 100
				if( (rowp = (float *)calloc(101, sizeof(float) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				if( (colp = (float *)calloc(101, sizeof(float) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				// calc row variables
				mu_x = 0.0;
				for(index1 = 1; index1 < 101; index1++) {	// excludes zero and missing values  // index1 is row number
					nobs = 0;		// number of adjacencies in a row
					for(index2 = 1; index2 < 101; index2++) {	// excludes zero and missing values // index2 is col number
						nobs += (*(freqptr2 + ((index1 * 102) + index2)) );	// number of adjacencies
					}
					(*(rowp + index1)) = inv_num_adjacencies * nobs;					// Px(i) 
					mu_x += (1.0*index1) * (*(rowp + index1));							// mux
				}
				// calc col variables
				mu_y = 0.0;
				for(index1 = 1; index1 < 101; index1++) {	// excludes zero and missing values  // index1 is col number
					nobs = 0;
					for(index2 = 1; index2 < 101; index2++) {	// excludes zero and missing values // index2 is row number
						nobs += (*(freqptr2 + ((index2 * 102) + index1)));	// number of adjacencies // note switch of index1&2
					}
					(*(colp + index1)) = inv_num_adjacencies * nobs;	// Py(j)
					mu_y += (1.0*index1) * (*(colp + index1));							// muy
				}
				// accumulate the entire metric as 'numerator'
				numerator = 0.0; 		
				for(index1 = 1; index1 < 101; index1++) {		// excludes zero and missing values // rows
					for(index2 = 1; index2 < 101; index2++) {	// excludes zero and missing values // columns
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						temp_float = inv_num_adjacencies * temp_int;	// Pij
						temp_float2 = (1.0 * (index1 + index2)) - mu_x - mu_y; 
						temp_float3 = temp_float2 * temp_float2 * temp_float2; 	// first term
						numerator += temp_float * temp_float3;
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				free(rowp);
				free(colp);
				break;
			}
		}
		case 46:{	// Cluster prominence, including 0
					// Formula in Soh 1999 attributes it to Conners 1982.  Don't use Lofstedt formula; it is for symmetric GLCMs
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					break;	// returns missing
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				// malloc 2 frequency tables needed to store row and column marginal P's
				// for 101 possible values, from 0 to 100
				if( (rowp = (float *)calloc(101, sizeof(float) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				if( (colp = (float *)calloc(101, sizeof(float) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				// calc row variable
				mu_x = 0.0;
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values  // index1 is row number
					nobs = 0;		// number of adjacencies in a row
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values // index2 is col number
						nobs += (*(freqptr2 + ((index1 * 102) + index2)) );	// number of adjacencies
					}
					(*(rowp + index1)) = inv_num_adjacencies * nobs;					// Px(i) 
					mu_x += (1.0*index1) * (*(rowp + index1));							// mux
				}
				// calc col variable
				mu_y = 0.0;
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values  // index1 is col number
					nobs = 0;
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values // index2 is row number
						nobs += (*(freqptr2 + ((index2 * 102) + index1)));	// number of adjacencies // note switch of index1&2
					}
					(*(colp + index1)) = inv_num_adjacencies * nobs;	// Py(j)
					mu_y += (1.0*index1) * (*(colp + index1));							// muy
				}
				// accumulate the entire metric as 'numerator'
				numerator = 0.0; 		
				for(index1 = 0; index1 < 101; index1++) {		// excludes missing values // rows
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values // columns
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						temp_float = inv_num_adjacencies * temp_int;	// Pij
						temp_float2 = (1.0 * (index1 + index2)) - mu_x - mu_y; 
						temp_float3 = temp_float2 * temp_float2 * temp_float2 * temp_float2; 	// first term
						numerator += temp_float * temp_float3;
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				free(rowp);
				free(colp);break;
			}
			if(parameters.exclude_zero == 1){	// 1046 Cluster prominence, excluding 0
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					break;	// returns missing
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				// malloc 2 frequency tables needed to store row and column marginal P's
				// for 101 possible values, from 0 to 100
				if( (rowp = (float *)calloc(101, sizeof(float) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				if( (colp = (float *)calloc(101, sizeof(float) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				// calc row variables
				mu_x = 0.0;
				for(index1 = 1; index1 < 101; index1++) {	// excludes zero and missing values  // index1 is row number
					nobs = 0;		// number of adjacencies in a row
					for(index2 = 1; index2 < 101; index2++) {	// excludes zero and missing values // index2 is col number
						nobs += (*(freqptr2 + ((index1 * 102) + index2)) );	// number of adjacencies
					}
					(*(rowp + index1)) = inv_num_adjacencies * nobs;					// Px(i) 
					mu_x += (1.0*index1) * (*(rowp + index1));							// mux
				}
				// calc col variables
				mu_y = 0.0;
				for(index1 = 1; index1 < 101; index1++) {	// excludes zero and missing values  // index1 is col number
					nobs = 0;
					for(index2 = 1; index2 < 101; index2++) {	// excludes zero and missing values // index2 is row number
						nobs += (*(freqptr2 + ((index2 * 102) + index1)));	// number of adjacencies // note switch of index1&2
					}
					(*(colp + index1)) = inv_num_adjacencies * nobs;	// Py(j)
					mu_y += (1.0*index1) * (*(colp + index1));							// muy
				}
				// accumulate the entire metric as 'numerator'
				numerator = 0.0; 		
				for(index1 = 1; index1 < 101; index1++) {		// excludes zero and missing values // rows
					for(index2 = 1; index2 < 101; index2++) {	// excludes zero and missing values // columns
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						temp_float = inv_num_adjacencies * temp_int;	// Pij
						temp_float2 = (1.0 * (index1 + index2)) - mu_x - mu_y; 
						temp_float3 = temp_float2 * temp_float2 * temp_float2 * temp_float2; 	// first term
						numerator += temp_float * temp_float3;
					}
				}
				temp_out = numerator;  
				metric_value = roundf(temp_out * 100000) / 100000;
				free(rowp);
				free(colp);
				break;
			}
		}
		case 47:{	// Root mean square of pixel values, including 0.
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// number of non-missing pixels
				numpix = max_npix - (*(freqptr1 + 101)); 	
				if(numpix == 0) {
					metric_value = -0.01;
					break;
				}
				else{
					inv_denominator = 1.0 / numpix;
				}
				// sumx2 is weighted sum of squares of integer pixel values
				sumx2 = 0;
				for(index = 0; index < 101; index++) {	// includes 0, excludes missing 
					sumx2 += (*(freqptr1 + index)) * (index * index);
				}
				// note pop RMS uses N
				// 1.1.1
				temp_float = inv_denominator * (1.0 * sumx2);
				temp_out = sqrt(temp_float);
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1047 Root mean square of pixel values, excluding 0.
				// number of non-missing pixels, excluding zero and misssing
				numpix = max_npix - (*(freqptr1 + 0)) - (*(freqptr1 + 101)); 	
				if(numpix == 0) {
					metric_value = -0.01;
					break;
				}
				else{
					inv_denominator = 1.0 / numpix;
				}
				// sumx2 is weighted sum of squares of integer pixel values
				sumx2 = 0;
				for(index = 1; index < 101; index++) {	// excludes 0, excludes missing 
					sumx2 += (*(freqptr1 + index)) * (index * index);
				}
				//  note pop RMS uses N
				// 1.1.1
				temp_float = inv_denominator *  (1.0 * sumx2);
				temp_out = sqrt(temp_float);
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 48:{	// Average absolute deviation of pixel values, including 0
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// number of non-missing pixels
				temp_int = max_npix - (*(freqptr1 + 101)); 	
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * temp_int);
				}
				// weighted sum of the integer pixel values (i.e., integer index values)
				temp_int = 0;
				for(index = 0; index < 101; index++) {	// includes 0, excludes missing 
					temp_int += (*(freqptr1 + index)) * index;
				}
				mu_x = inv_denominator * (1.0 * temp_int);	// mean value
				// calc mean absolute deviation
				temp_float = 0.0;
				for(index = 0; index < 101; index++) {	// includes 0, excludes missing 
					temp_float += (*(freqptr1 + index)) * fabs( (1.0 * index) - mu_x) ; // note using fabs()
				}
				temp_out = inv_denominator * temp_float;
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){	// 1048 Average absolute deviation of pixel values, excluding 0			
				// number of non-missing pixels, excluding zero abd missing
				temp_int = max_npix - (*(freqptr1 + 0)) - (*(freqptr1 + 101)); 	
				if(temp_int == 0) {
					metric_value = -0.01;
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * temp_int);
				}
				// weighted sum of the integer pixel values (i.e., integer index values)
				temp_int = 0;
				for(index = 1; index < 101; index++) {	// excludes 0 and missing 
					temp_int += (*(freqptr1 + index)) * index;
				}
				mu_x = inv_denominator * (1.0 * temp_int);	// mean value
				// calc mean absolute deviation
				temp_float = 0.0;
				for(index = 1; index < 101; index++) {	// includes 0, excludes 0 and missing 
					temp_float += (*(freqptr1 + index)) * fabs( (1.0 * index) - mu_x) ; // note using fabs()
				}
				temp_out = inv_denominator * temp_float;
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		case 49:{	// k-contagion
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				// malloc and zero out a frequency table for 101 possible values of |i-j|, from 0 to 100
				if( (karray = (long int *)calloc(101, sizeof(long int) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				for(index = 0; index < 101; index++){
					(*(karray+index)) = 0;
				}
				// populate the frequencies of |i-j| = k
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						kval = abs(index1 - index2);			// abs(i-j)
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						(*(karray + kval)) += temp_int;			// these are frequencies
					}
				}
				// calculate the k-contagion
				// accumulate Sum(Pk/k) as 'numerator'
				numerator = 0.0;
				// add the weighted off-diagonals
				for(kval = 0; kval <= parameters.k_value; kval++){		// note loop includes the specified k value		
					numerator += (1.0 / (1.0 * (kval + 1) )) * inv_num_adjacencies * (*(karray + kval)); // Pk, first term is the weight 
				}
				temp_out = numerator;
				metric_value = roundf(temp_out * 100000) / 100000;
				free(karray);			
				break;
			}
			if(parameters.exclude_zero == 1){	// k-weighted contagion excluding 0
				// No need to know the number of different byte values in the window
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				// malloc and zero out a frequency table for 101 possible values of |i-j|, from 0 to 100
				if( (karray = (long int *)calloc(101, sizeof(long int) ) ) == NULL ) {
					printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Metric %ld.\n", parameters.metric); exit(25);
				}
				for(index = 0; index < 101; index++){
					(*(karray+index)) = 0;
				}
				// populate the frequencies of |i-j| = k
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes 0 and missing values
						kval = abs(index1 - index2);			// abs(i-j)
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						(*(karray + kval)) += temp_int;			// these are frequencies
					}
				}
				// calculate the k-contagion
				// accumulate Sum(Pk/k) as 'numerator'
				numerator = 0.0;
				// add the weighted off-diagonals
				for(kval = 0; kval <= parameters.k_value; kval++){				//  loop ends with user selected k value
					numerator += (1.0 / (1.0 * (kval + 1) )) * inv_num_adjacencies * (*(karray + kval)); // Pk, first term is the weight 			
				}
				temp_out = numerator;
				metric_value = roundf(temp_out * 100000) / 100000;
				free(karray);					
				break;
			}
		}
		case 50:{	// skewness of pixel values
			metric_value = -9000000.0;
			if(parameters.exclude_zero == 0){  //  including pixel value 0
				// number of non-missing pixels
				numpix = max_npix - (*(freqptr1 + 101)); 	
				if(numpix == 0) {
					metric_value = -9000000.0;	// note missing value because metric can be less than zero
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * numpix);
				}
				// 1.1.1
				sumx = 0;
				for(index = 0; index < 101; index++) {	// includes 0, excludes missing 
					sumx += (*(freqptr1 + index)) * index;
				}
				mu_x = inv_denominator * (1.0 * sumx);
				temp_float = 0.0;
				temp_float2 = 0.0; // accumulate (x-mu)^2
				numerator = 0.0; // accumulate (x-mu)^3
				for(index = 0; index < 101; index++) {	// includes 0, excludes missing
					temp_float = 1.0 * index;
					temp_float2 += (1.0 * (*(freqptr1 + index)) ) * ( (temp_float - mu_x) * (temp_float - mu_x) );
					numerator += ( inv_denominator * (1.0 * (*(freqptr1 + index)) ) ) * ( (temp_float - mu_x) * (temp_float - mu_x) * (temp_float - mu_x) );
				}
				// calc std dev
				temp_float = inv_denominator * temp_float2;
				sigma_x = sqrt(temp_float);
				sigma_x = roundf(sigma_x * 100000) / 100000; // round to 5 digits for comparision to zero
				if(sigma_x == 0.0){
					metric_value = -9000000.0;
					break;
				}
				temp_out = numerator / (sigma_x * sigma_x * sigma_x);
				metric_value = roundf(temp_out * 100000) / 100000;
				//
				break;
			}
			if(parameters.exclude_zero == 1){ //  excluding pixel value 0
				// number of non-missing pixels, excluding zero
				numpix = max_npix - (*(freqptr1 + 0)) - (*(freqptr1 + 101)); 	
				if(numpix == 0) {
					metric_value = -9000000.0;
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * numpix);
				}
				// 1.1.1
				sumx = 0;
				for(index = 1; index < 101; index++) {	// excludes 0, excludes missing 
					sumx += (*(freqptr1 + index)) * index;
				}
				mu_x = inv_denominator * (1.0 * sumx);
				temp_float = 0.0;
				temp_float2 = 0.0; // accumulate (x-mu)^2
				numerator = 0.0; // accumulate (x-mu)^3
				for(index = 1; index < 101; index++) {	// excludes 0, excludes missing
					temp_float = 1.0 * index;
					temp_float2 += (1.0 * (*(freqptr1 + index)) ) * ( (temp_float - mu_x) * (temp_float - mu_x) );
					numerator += ( inv_denominator * (1.0 * (*(freqptr1 + index)) ) ) * ( (temp_float - mu_x) * (temp_float - mu_x) * (temp_float - mu_x) );
				}
				// calc std dev
				temp_float = inv_denominator * temp_float2;
				sigma_x = sqrt(temp_float);
				sigma_x = roundf(sigma_x * 100000) / 100000; // round to 5 digits for comparision to zero
				if(sigma_x == 0.0){
					metric_value = -9000000.0;
					break;
				}
				temp_out = numerator / (sigma_x * sigma_x * sigma_x);
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
				//
			}
		}
		case 51:{	// kurtosis of pixel values
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){  //  including pixel value 0
				// number of non-missing pixels
				numpix = max_npix - (*(freqptr1 + 101)); 	
				if(numpix == 0) {
					metric_value = -0.01;	// note missing value because metric can be less than zero
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * numpix);
				}
				// 1.1.1
				sumx = 0;
				for(index = 0; index < 101; index++) {	// includes 0, excludes missing 
					sumx += (*(freqptr1 + index)) * index;
				}
				mu_x = inv_denominator * (1.0 * sumx);
				temp_float = 0.0;
				temp_float2 = 0.0; // accumulate (x-mu)^2
				numerator = 0.0; // accumulate (x-mu)^4
				for(index = 0; index < 101; index++) {	// includes 0, excludes missing
					temp_float = 1.0 * index;
					temp_float2 += (1.0 * (*(freqptr1 + index)) ) * ( (temp_float - mu_x) * (temp_float - mu_x) );
					numerator += ( inv_denominator * (1.0 * (*(freqptr1 + index)) ) ) * ( (temp_float - mu_x) * (temp_float - mu_x) * (temp_float - mu_x) * (temp_float - mu_x) );
				}
				// calc std dev
				temp_float = inv_denominator * temp_float2;
				sigma_x = sqrt(temp_float);
				sigma_x = roundf(sigma_x * 100000) / 100000; // round to 5 digits for comparision to zero
				if(sigma_x == 0.0){
					metric_value = -0.01;
					break;
				}
				temp_out = numerator / (sigma_x * sigma_x * sigma_x * sigma_x);
				metric_value = roundf(temp_out * 100000) / 100000;
				//
				break;
			}
			if(parameters.exclude_zero == 1){ //  excluding pixel value 0
				// number of non-missing pixels, excluding zero
				numpix = max_npix - (*(freqptr1 + 0)) - (*(freqptr1 + 101)); 	
				if(numpix == 0) {
					metric_value = -0.01;
					break;
				}
				else {
					inv_denominator = 1.0 / (1.0 * numpix);
				}
				// 1.1.1
				sumx = 0;
				for(index = 1; index < 101; index++) {	// excludes 0, excludes missing 
					sumx += (*(freqptr1 + index)) * index;
				}
				mu_x = inv_denominator * (1.0 * sumx);
				temp_float = 0.0;
				temp_float2 = 0.0; // accumulate (x-mu)^2
				numerator = 0.0; // accumulate (x-mu)^3
				for(index = 1; index < 101; index++) {	// excludes 0, excludes missing
					temp_float = 1.0 * index;
					temp_float2 += (1.0 * (*(freqptr1 + index)) ) * ( (temp_float - mu_x) * (temp_float - mu_x) );
					numerator += ( inv_denominator * (1.0 * (*(freqptr1 + index)) ) ) * ( (temp_float - mu_x) * (temp_float - mu_x) * (temp_float - mu_x) * (temp_float - mu_x) );
				}
				// calc std dev
				temp_float = inv_denominator * temp_float2;
				sigma_x = sqrt(temp_float);
				sigma_x = roundf(sigma_x * 100000) / 100000; // round to 5 digits for comparision to zero
				if(sigma_x == 0.0){
					metric_value = -0.01;
					break;
				}
				temp_out = numerator / (sigma_x * sigma_x * sigma_x * sigma_x);
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
				//
			}
			break;
		}
		case 52:{ 	// Peter's metric "Clustering". Same as metric 28 except replacing in two places "temp_int2 = abs(index1 - index2);         // |i-j|”
					//		with "temp_int2 = ( 1.0* (index1 + index2) ) / 2.0;   // average of two pixel values”
			metric_value = -0.01;
			if(parameters.exclude_zero == 0){
				// No need to know the number of different byte values in the window;
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing column, excluding last (missing) row
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * ((i+j)/2) as 'numerator'
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * ((i+j)/2)
				for(index1 = 0; index1 < 101; index1++) {	// excludes missing values
					for(index2 = 0; index2 < 101; index2++) {	// excludes missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));	// number of adjacencies
						if(temp_int > 0) { 	
							temp_float2 = (1.0 * (index1 + index2))/2.0; 	// (i+j)/2
							temp_float = inv_num_adjacencies * temp_int;	// Pij
							numerator += (temp_float *  temp_float2);
						}
					}
				}
				temp_out = 0.01 * numerator;  // re-scale to 0,1
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
			if(parameters.exclude_zero == 1){ 	// clustering, excluding 0
				// No need to know the number of different byte values in the window; 
				// find the number of non-missing adjacencies by subtracting the missing ones
				num_adjacencies = 0;
				num_missing = 0;
				// find sum of missing and zero columns, excluding first (zero) and last (missing) rows
				for(index1 = 1; index1 < 101; index1++) {	// excludes missing row
					num_missing += (*(freqptr2 + (102*index1)+0));		// count zero column
					num_missing += (*(freqptr2 + (102*index1)+101));	// count missing column
				}
				// find sum of the zero row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + index1));
				}
				// find sum of missing row, including last (missing) column
				for(index1 = 0; index1 < 102; index1++) {	// includes last column
					num_missing += (*(freqptr2 + (101*102) + index1));
				}
				num_adjacencies = max_nadj - num_missing;
				if(num_adjacencies  == 0) {  // all adjacencies are missing
					metric_value = -0.01;
					break;
				}
				// accumulate Pij * ((i+j)/2 as 'numerator'
				inv_num_adjacencies = 1.0 / (1.0 * num_adjacencies);  // divisor is the total number of non-missing adjacencies
				numerator = 0.0; 		// sum of the Pij * ((i+j)/2
				for(index1 = 1; index1 < 101; index1++) {	// excludes 0 and missing values
					for(index2 = 1; index2 < 101; index2++) {	// excludes 0 and missing values
						temp_int = (*(freqptr2 + ((index1 * 102) + index2)));
						if(temp_int > 0) { 
							temp_float2 = (1.0 * (index1 + index2))/2.0; 	// (i+j)/2
							temp_float = inv_num_adjacencies * temp_int;	// Pij
							numerator += (temp_float *  temp_float2);
						}
					}
				}
				temp_out = 0.01 * numerator;  // re-scale to 0,1
				metric_value = roundf(temp_out * 100000) / 100000;
				break;
			}
		}
		default:{	// missing if no metric is calculated
			metric_value = -0.01;
			break; 
		}
	} // end of switch statement
	return(metric_value);

}
/*  *********************
    Global_Analysis
	Find the frequencies of pixels and/or adjacencies
	Call the Metric_Calculator
	Returns a metric for the entire map extent
	*********************
*/
float Global_Analysis()
{
	long int index, temp2, t1, t2, nrows, ncols, r, c, freq_type, numval, temp_int;   
	float metric_value;
	long int *freqptr1; // pixel frequencies
	long int *freqptr2; // adjacency frequencies
	
	nrows = parameters.nrows;
	ncols = parameters.ncols;
	freq_type = control.freq_type;
	numval = 102; // number of byte values in 0,101 is 102
	metric_value = -0.01;
	// magic (see programming notes). note input image is mat_in_byte in common area
	unsigned char (*Matrix_1)[ncols] = (unsigned char (*)[ncols])mat_in_byte;
	// allocate and zero memory outside the loops
	// the implication is that the loops cannot be parallelized
	temp_int = numval;
	if( (freqptr1 = (long int *)calloc(temp_int, sizeof(long int) ) ) == NULL ) {
		printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator1 in Global_Analysis.\n"); exit(25);
	}
	for(index = 0; index < temp_int; index++) {	
		(*(freqptr1 + index)) = 0;
    }
	temp_int = numval * numval;
	if( (freqptr2 = (long int *)calloc(temp_int, sizeof(long int) ) ) == NULL ) {
		printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator2 in Global_Analysis.\n"); exit(25);
	}
	for(index = 0; index < temp_int; index++) {	
		(*(freqptr2 + index)) = 0;
	}
	if( (freq_type == 1) || (freq_type == 3) ){ 			// accumulating pixel values
		for(r = 0; r < nrows; r++) { 
			for (c = 0; c < ncols; c++) { 
				temp_int = Matrix_1 [r] [c];    //The value of this cell
				(*(freqptr1 + temp_int))++;
			}
		}
	}
	if( (freq_type == 2) || (freq_type == 3) ){ 			// accumulating adjacency values
		// make one pass (in 3 loopsets) through the matrix to count the adjacencies between different byte values  
		// These adjacency counts will be accumulated "with regard to order"
		// The first loopset excludes last row and last column
		for(r = 0; r < nrows - 1; r++) { 
			for (c = 0; c < ncols - 1; c++) { 
				t1 = Matrix_1 [r] [c];     /* The type of this cell */ 
				t2 = Matrix_1 [r+1] [c];   /* The type of the cell below */ 
				/* Increment the matrix of counts for that join */ 
				temp2 = (t1 * 102) + t2; // number of byte values in 0,101 is 102
				(*(freqptr2 + temp2))++; 
				/* Now do the same thing for the cell to the right */ 
				t2 = Matrix_1 [r] [c+1]; 
				temp2 = (t1 * 102) + t2; 
				(*(freqptr2 + temp2))++; 
			} 
		} 
	/*   Second loopset covers last row; no need to look to the cell below in this case*/ 
		for(c = 0; c < ncols - 1; c++) { 
			t1 = Matrix_1 [nrows - 1] [c];     /* The type of this cell */ 
			t2 = Matrix_1 [nrows - 1] [c + 1]; /* The type of cell to the right */ 
			/* Increment the matrix of counts for that join, and the total*/ 
			temp2 = (t1 * 102) + t2; 
			(*(freqptr2 + temp2))++; 
		} 
	/*   Third loopset covers last column; no need to look to the right */ 
		for(r = 0; r < nrows - 1; r++)  { 
			t1 = Matrix_1 [r] [ncols - 1];   /* type of this cell */ 
			t2 = Matrix_1 [r+1] [ncols - 1]; /* The type of the cell below */ 
			/* Increment the matrix of counts for that join, and the total*/ 
			temp2 = (t1 * 102) + t2; 
			(*(freqptr2 + temp2))++; 
		}
	}	
	// calculate the metric for the  window
	metric_value = Metric_Calculator(freqptr1, freqptr2);
	free(freqptr1);
	free(freqptr2);
	return(metric_value);
}
/*  *********************
    Global_Range
	Calculate the observed range of pixel values on the entire input image.
	The parameter 'flag' indicates include zero (1) or exclude zero (2)
	The input image mat_in_byte has 101 for missing values
	*********************
*/
long int Global_Range(long int flag)
{	
	long int range, r, c, index, nrows, ncols, temp_int, min, max, start;
	long int *freqptr;	// count of pixel values
	if( (freqptr = (long int *)calloc(102, sizeof(long int) ) ) == NULL ) {
		printf("\nGraySpatCon: Error -- Cannot allocate memory for accumulator in Global_Range.\n"); exit(25);
	}
	// Zero the accumulator
	for(index = 0; index < 102; index++){
		(*(freqptr+index)) = 0;
	}
	nrows = parameters.nrows;
	ncols = parameters.ncols;
	// magic (see programming notes). note input image is mat_in_byte in common area
	unsigned char (*Matrix_1)[ncols] = (unsigned char (*)[ncols])mat_in_byte;
	// Start by finding the frequency of all possible pixel values
	for(r = 0; r < nrows; r++) { 
		for (c = 0; c < ncols; c++) { 
			temp_int = Matrix_1 [r] [c];    //The value of this cell
			(*(freqptr + temp_int))++;
		}
	}
	if(flag == 1){ start = 0;}
	if(flag == 2){ start = 1;}
	min = 0;
	// Find the minimum pixel value
	for(index = start; index < 101; index++){
		if( (*(freqptr + index)) > 0){
			min = index;
			break;
		}
	}
	max = 0;
	// Find the maximum pixel value
	for(index = 100; index >= start; index--){
		if( (*(freqptr + index)) > 0){
			max = index;
			break;
		}
	}
	range = max - min;
	free(freqptr);
	return(range);
}
