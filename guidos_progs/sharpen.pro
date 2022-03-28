;+
; NAME:
;       Sharpen
;
; PURPOSE:
;
;        This function sharpens an image using a Laplacian kernel.
;        The final result is color adjusted to match the histogram
;        of the input image.
;
; AUTHOR:
;
;       FANNING SOFTWARE CONSULTING
;       David Fanning, Ph.D.
;       1645 Sheely Drive
;       Fort Collins, CO 80526 USA
;       Phone: 970-221-0438
;       E-mail: davidf@dfanning.com
;       Coyote's Guide to IDL Programming: http://www.dfanning.com
;
; CATEGORY:
;
;       Image Processing
;
; CALLING SEQUENCE:
;
;       sharp_image = Sharpen(image)
;
; INPUTS:
;
;       image - The input image to be sharpened. Assumed to be a 2D byte array.
;
; OUTPUTS:
;
;       sharp_image - The sharpened image.
;
; INPUT KEYWORDS:
;
;       KERNEL -- By default the image is convolved with this 3-by-3 Laplacian kernel:
;           [ [-1, -1, -1], [-1, +8, -1], [-1, -1, -1] ].  You can pass in any square kernel
;           of odd width. The filtered image is added back to the original image to provide
;           the sharpening effect.
;
;       DISPLAY -- If this keyword is set a window is opened and the details of the sharpening
;           process are displayed.
;
; OUTPUT KEYWORDS:
;
;       None.
;
; DEPENDENCIES:
;
;       None.
;
; METHOD:
;
;       This function is based on the Laplacian kernel sharpening method on pages 128-131
;       of Digital Image Processing, 2nd Edition, Rafael C. Gonzalez and Richard E. Woods,
;       ISBN 0-20-118075-8.
;
; EXAMPLE:
;
;       There is an example program at the end of this file.
;
; MODIFICATION HISTORY:
;
;       Written by David W. Fanning, January 2003.
;-
;
;###########################################################################
;
; LICENSE
;
; This software is OSI Certified Open Source Software.
; OSI Certified is a certification mark of the Open Source Initiative.
;
; Copyright © 2003 Fanning Software Consulting.
;
; This software is provided "as-is", without any express or
; implied warranty. In no event will the authors be held liable
; for any damages arising from the use of this software.
;
; Permission is granted to anyone to use this software for any
; purpose, including commercial applications, and to alter it and
; redistribute it freely, subject to the following restrictions:
;
; 1. The origin of this software must not be misrepresented; you must
;    not claim you wrote the original software. If you use this software
;    in a product, an acknowledgment in the product documentation
;    would be appreciated, but is not required.
;
; 2. Altered source versions must be plainly marked as such, and must
;    not be misrepresented as being the original software.
;
; 3. This notice may not be removed or altered from any source distribution.
;
; For more information on Open Source Software, visit the Open Source
; web site: http://www.opensource.org.
;
;###########################################################################


FUNCTION Sharpen_HistoMatch, image, histogram_to_match

;; Error handling.
Catch, theError
IF theError NE 0 THEN BEGIN
   Catch, / Cancel

   ;; Get the call stack and the calling routine's name.
   Help, Calls = callStack
   IF Float(!Version.Release) GE 5.2 THEN $
    callingRoutine = (StrSplit(StrCompress(callStack[1]), " ", $
                               / Extract))[0] ELSE $
    callingRoutine = (Str_Sep(StrCompress(callStack[1]), " "))[0]

   ;; Print a traceback.
   Help, / Last_Message, Output = traceback
   Print, ''
   Print, 'Traceback Report from ' + StrUpCase(callingRoutine) + ':'
   Print, ''
   FOR j = 0, N_Elements(traceback) - 1 DO Print, "     " + traceback[j]
   IF N_Elements(image) NE 0 THEN RETURN, image ELSE RETURN, - 1L
ENDIF

;; We require two input parameters.
IF N_Params() NE 2 THEN Message, $
 'Two arguments required. Please read the program documentation.'

;; Must have 2D image array.
IF Size(image, / N_Dimensions) NE 2 THEN Message, $
 'Image argument must be 2D. Returning.'

;; Is the histogram_to_match variable a 1D or 2D array? Branch accordingly.
CASE Size(histogram_to_match, / N_Dimensions) OF
   1: BEGIN
      IF N_Elements(histogram_to_match) NE 256 THEN $
       Message, 'Histogram to match has incorrect size. Returning.'
      match_histogram = histogram_to_match
   END
   2: match_histogram = Histogram(Byte(histogram_to_match), $
                                  Min = 0, Max = 255, Binsize = 1)
   ELSE: Message, $
    'Histogram to match has incorrect number of dimensions. Returning.'
ENDCASE

;; Calculate the histogram of the input image.
h = Histogram(Byte(image), Binsize = 1, Min = 0, Max = 255)

;; Make sure the two histograms have the same number of pixels. This will
;; be a problem if the two images are different sizes, you are matching a
;; histogram from an image subset, etc.
totalPixels = Float(N_Elements(image))
totalHistogramPixels = Float(Total(match_histogram))

IF totalPixels NE totalHistogramPixels THEN $
 factor = totalPixels / totalHistogramPixels ELSE $
 factor = 1.0
match_histogram = match_histogram * factor

;; Find a mapping from the input pixels to the transformation function s.
s = FltArr(256)
FOR k = 0, 255 DO BEGIN
   s[k] = Total(h(0:k) / totalPixels)
ENDFOR

;; Find a mapping from input histogram to the transformation function v.
v = FltArr(256)
FOR q = 0, 255 DO BEGIN
   v[q] = Total(match_histogram(0:q) / Total(match_histogram))
ENDFOR

;; Find probablitly density function z from v and s.
z = BytArr(256)
FOR j = 0, 255 DO BEGIN
   i = Where(v LT s[j], count)
   IF count GT 0 THEN z[j] = (Reverse(i))[0] ELSE z[j] = 0
ENDFOR

;; Create the matched image.
matchedImage = z[Byte(image)]
RETURN, matchedImage
END
; ----------------------------------------------------------------------------

FUNCTION Sharpen, image, Kernel = kernel

;; Error handling.
Catch, theError
IF theError NE 0 THEN BEGIN
   Catch, / Cancel

   ;; Get the call stack and the calling routine's name.
   Help, Calls=callStack
   IF Float(!Version.Release) GE 5.2 THEN $
    callingRoutine = (StrSplit(StrCompress(callStack[1]), " ", $
                               / Extract))[0] ELSE $
    callingRoutine = (Str_Sep(StrCompress(callStack[1]), " "))[0]

   ;; Print a traceback.
   Help, / Last_Message, Output = traceback
   Print, ''
   Print, 'Traceback Report from ' + StrUpCase(callingRoutine) + ':'
   Print, ''
   FOR j = 0, N_Elements(traceback) - 1 DO Print, "     " + traceback[j]
   IF N_Elements(image) NE 0 THEN RETURN, image ELSE RETURN, - 1L
ENDIF

;; If an image is not provided. Issue an error message.
IF N_Elements(image) EQ 0 THEN $
 Message, 'A 2D image is required as an argument.'
IF Size(image, / N_Dimensions) NE 2 THEN $
 Message, 'Image must be a 2D array in this program.'

;; Resize the image, if required.
previewSize = 256
wxsize = previewSize
wysize = previewSize

;; Set up the convolution kernel for Laplacian filtering.
IF N_Elements(kernel) EQ 0 THEN BEGIN
   k = Replicate(-1, 3, 3)
   k[1,1] = 8
ENDIF ELSE BEGIN
   s = Size(kernel, / Dimensions)
   IF s[0] NE s[1] THEN Message, 'Kernel must be a square array.'
   IF s[0] MOD 2 NE 1 THEN Message, 'Kernel must be an odd width.'
   k = kernel
ENDELSE
thisImage = image

;; Create the Laplacian filtered image.
filteredImage = Convol(Float(thisImage), k, Center = 1, / Edge_Truncate, / NAN)

;; Scale the Laplacian filtered image. Note conversion of
;; image to integer values and use of 255 as a FLOAT value.
filteredImage = filteredImage - (Min(filteredImage))
filteredImage = filteredImage * (255. / Max(filteredImage))

;; Create the sharpened image by adding the Laplacian filtered image
;; back to the original image and re-scaling.
sharpened = thisImage + filteredImage
sharpened = sharpened - (Min(sharpened))
sharpened = sharpened * (255. / Max(sharpened))

;; Adjust the sharpened image to match the histogram of the original.
adjusted = Sharpen_HistoMatch(sharpened, image)

RETURN, adjusted
END
