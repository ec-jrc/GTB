# GTB
**GTB** is a desktop software application for generic raster image processing. Full installation packages, including precompiled executables (macOS, Linux, MS-Windows) of the application, can be downloaded from the project homepage (https://forest.jrc.ec.europa.eu/en/activities/lpa/gtb/). GTB is written in the IDL language, and you must be the legal owner of an IDL licence to compile the IDL source code. Further information on the IDL software can be found at: https://www.harrisgeospatial.com. Alternative to using IDL, feel free to recode the IDL source code to the programming language of your choice.

**Reference:** Vogt P. and Riitters K. (2017). GuidosToolbox: universal digital image object analysis. European Journal of Remote Sensing, 50, 1, pp. 352-361, doi: 10.1080/22797254.2017.1330650


This repository provides information on the GTB source code:

a) directory: data
-----------
-   input.tif: test image for GTB

b) directory: external_sources
-------
C-source code of GTB-external programs:
-   ggeo: https://github.com/ec-jrc/jeolib-miallib/blob/master/core/c/ggeo.c
-   mspa: https://github.com/ec-jrc/jeolib-miallib/blob/master/core/c/mspa.c; requiring miallib: https://github.com/ec-jrc/jeolib-miallib
-   combinelpt: combinelpt28Sept2021.c
-   recode: recode28Sept2021.c
-   spatcon: spatcon30Sept2021.c
-   orig/spatcon: spatcon2July2018.c; older version of spatcon, needed for the GTB-Dominance module

c) directory: guidos_progs
------
GTB-required directories and files:
-   mspatmp: used for intermediate processing
-   tmp: used for intermediate processing
-   *.pro: IDL-subroutines
-   *.sav: IDL-settings and colortables
-   *.png: images
-   *.jpg: images
-   *.txt: descriptions
-   startGTBterminal.sh: bash-script to launch a terminal session 

d) file: guidostoolbox.pro
------
-   guidostoolbox.pro: GTB IDL-source code

e) Additional documents of the current GTB-version:
-----
-   changelog: https://ies-ows.jrc.ec.europa.eu/gtb/GTB/changelog.txt
-   GTB Manual: https://ies-ows.jrc.ec.europa.eu/gtb/GTB/GuidosToolbox_Manual.pdf
-   MSPA Guide: https://ies-ows.jrc.ec.europa.eu/gtb/GTB/MSPA_Guide.pdf
