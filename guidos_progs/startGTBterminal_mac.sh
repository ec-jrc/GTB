#!/bin/bash
#
# we are in directory GuidosToolbox/data
PATH=/Library/Frameworks/GDAL.framework/Programs:$PATH
osxv=`sw_vers -productVersion| awk -F '.' '{ print $2 }'`

# launch the xterm
if [ $osxv -ge 10 ]; then
/opt/X11/bin/xterm -j -sb -sl 5000 -title "http://www.gdal.org/gdal_utilities.html" -e "ls /Library/Frameworks/GDAL.framework/Programs/; echo ''; echo '********************************************************************************'; echo 'More information on the commands listed above can be found on the webpage:'; echo 'http://www.gdal.org/gdal_utilities.html'; echo 'For example, to get information on the GuidosToolbox sample input image enter:'; echo 'gdalinfo input.tif'; echo '********************************************************************************'; $SHELL"

else
/usr/X11/bin/xterm -j -sb -sl 5000 -title "http://www.gdal.org/gdal_utilities.html" -e "ls /Library/Frameworks/GDAL.framework/Programs/; echo ''; echo '********************************************************************************'; echo 'More information on the commands listed above can be found on the webpage:'; echo 'http://www.gdal.org/gdal_utilities.html'; echo 'For example, to get information on the GuidosToolbox sample input image enter:'; echo 'gdalinfo input.tif'; echo '********************************************************************************'; $SHELL"

fi
exit 0;
