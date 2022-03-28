# GTB
**GTB** is a desktop software application for generic raster image processing. Precompiled executables (macOS, Linux, MS-Windows) of the 
application can be downloaded on the project homepage (https://forest.jrc.ec.europa.eu/en/activities/lpa/gtb/).
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
