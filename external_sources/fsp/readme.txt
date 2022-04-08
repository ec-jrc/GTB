fsp is an amended version of mspa.c designed to work with GTB/GWB.
Modifications include parsing mspa input parameters and the option to write temporary files out to disk.
The original version of mspa.c is available at: https://github.com/ec-jrc/jeolib-miallib/blob/master/core/c/mspa.c

In addition to the files provided here, compilation of fsp.c requires the following files from the miallib repository: https://github.com/ec-jrc/jeolib-miallib
- miallib.h
- mialtypes.h
- op.h
- libmiallib_generic.a (following the build instructions in jeolib-miallib/core/c/Makefile)


Then run the included Makefile to compile mspa_lin64 for use in GTB/GWB.
