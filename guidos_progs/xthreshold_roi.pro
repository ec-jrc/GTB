;+
; NAME:
;       XTHRESHOLD_ROI
;
; PURPOSE:
;
;       The purpose of this program is to interactively apply a simple
;       linear stretch to an image by moving two lines on a histogram
;       plot of the image. The portion of the image data between the
;       two lines is stretched over the available colors in the color table.
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
;      modified to suite the ROI_GUI program
;
; CATEGORY:
;
;       Graphics, Widgets
;
; CALLING SEQUENCE:
;
;       XTHRESHOLD_ROI, image
;
; INPUT PARAMETERS:
;
;       image:    The image data to be stretched.It must be 2D. (This may now
;                 be a pointer to the image data rather than the image itself.)
;
; KEYWORD PARAMETERS:
;
;       COLORTABLE: The index of a colortable you would like to load.
;                 The current colortable is used if this keyword is undefined.
;
;       _EXTRA:   This keyword collects any keyword appropriate for the
;                 Plot command.
;
;       GROUP_LEADER: Keyword to assign a group leader (so this program can be
;                 called from within another widget program).
;
;       NO_WINDOW: Set this keyword if you want no image window. This would
;                 typically be set if the NOTIFY_PRO keyword was being used,
;                 for example.
;
;       NOTIFY_OBJ: Set this keyword to a structure containing the fields OBJECT
;                 and METHOD. When the image is changed, the object identified in
;                 the OBJECT field with have the method identified in the METHOD
;                 field called.
;
;       NOTIFY_PRO: Set this keyword to the name of a procedure that should
;                 be notified when the image is changed. The procedure should
;                 be defined with one positional parameter, which will be the
;                 stretched image. The method should be written to accept one
;                 positional parameter, which is the stretched image.
;
;       MAX_VALUE: Keyword to assign a maximun value for the Histogram Plot.
;                 Images with lots of pixels of one color (e.g. black) skew
;                 the histogram. This helps make a better looking plot.
;
;       NCOLORS:  Keyword to assign the number of colors used to display
;                 the image. The default is !D.Table_Size-4.
;
;       TOP:      This keyword has no effect unless the NOTIFY_XXX keywords
;                 are used. Because of the way colors are used in this program,
;                 the display image has to be scaled into a maximum of 255-4
;                 colors. Setting this keyword will make sure that the image
;                 reported to the NOTIFY_XXX procedures will use the full
;                 dynamic range. In other words:
;
;                      reportedImage = BytScl(image, Min=minLineValue, Max=maxLineValue, $
;                           Top=topValue); where topValue = top < 255
;
; OUTPUTS:
;       None.
;
; COMMON BLOCKS:
;       None.
;
; SIDE EFFECTS:
;       None.
;
; DEPENDENCIES:
;
;       The following programs are required from the Coyote Library:
;
;           error_message.pro
;           fsc_droplist.pro
;           fsc_fileselect.pro
;           fsc_inputfield.pro
;           fsc_plotwindow.pro
;           fsc_psconfig__define.pro
;           getimage.pro
;           pswindow.pro
;           psconfig.pro
;           selectimage.pro
;           textbox.pro
;           tvimage.pro
;           tvread.pro
;
; EXAMPLE:
;
;       If you have a 2D image in the variable "image", you can run this
;       program like this:
;
;       XTHRESHOLD_ROI, image
;
; MODIFICATION HISTORY:
;
;       Written by: David W. Fanning, April 1996.
;       October, 1996 Fixed a problem with not restoring the color
;          table when the program exited. Substituted a call to XCOLORS
;          instead of XLOADCT.
;       October, 1998. Added NO_BLOCK keyword and modified to work with
;          24-bit color devices.
;       April, 1999. Made lines thicker. Offered default image. DWF.
;       April, 1999. Replaced TV command with TVIMAGE. DWF.
;       April, 1999. Made both windows resizeable. DWF.
;       April, 2000. Made several modifications to histogram plot and to
;          the way colors were handled. Added ability to pass pointer to
;          the image as well as image itself. DWF.
;       February 2001. Removed GIF file support for IDL 5.4 and fixed
;          a problem with cleaning up the pixmap. DWF.
;       October 29, 2002. Added ability to load an image file with GETIMAGE. DWF.
;       Added ability to store stretched image as main-level variable. Oct 30, 2002. DWF.
;       Fixed a problem with the image window aspect ratio being calculated
;          incorrectly. 2 Nov 2002. DWF.
;       Added ability to open formatted images as well as raw data files. 2 Nov 2002. DWF.
;       Fixed a couple of minor problems with resizing the histogram window. 4 Nov 2002. DWF.
;       Added NO_WINDOW and NOTIFY_PRO keywords. 4 Nov 2002. DWF.
;       Fixed a problem with the histogram plot when the minimum image value
;          is greater than 0. 8 Nov 2002. DWF.
;       Added NOTIFY_OBJ, TOP, and BLOCK keywords. 16 Nov 2002. DWF.
;-
;
;###########################################################################
;
; LICENSE
;
; This software is OSI Certified Open Source Software.
; OSI Certified is a certification mark of the Open Source Initiative.
;
; Copyright � 2000-2002 Fanning Software Consulting.
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

PRO Xthreshold_ROI_IMAGEWINDOWKILLED, imageWindowID

;; Turn the Save As button off.
Widget_Control, imageWindowID, Get_UValue=buttonIDs

IF Widget_Info(buttonIDs[0], /Valid_ID) THEN BEGIN
   Widget_Control, buttonIDs[0], Sensitive=0
ENDIF
END
; of Xthreshold_ROI_IMAGEWINDOWKILLED  ***************************************



PRO Xthreshold_ROI_HISTOPLOT, image, Binsize = binsize, Reverse_Indices = r, $
                            WID = wid, Color = color, Background = background, $
                            Max_Value = maxvalue, _Extra = extra, $
                            Red = red, Green = green, Blue = blue

;; This is a utility program to draw a histogram plot in a
;; display window.

;; Catch any error in the histogram display routine.
Catch, theError
IF theError NE 0 THEN BEGIN
   Catch, Cancel = 1
   ok = Error_Message( / Traceback)
   RETURN
ENDIF


IF N_Elements(red) NE 0 THEN TVLCT, red, green, blue

;; Calculate binsize.
range = Max( * image) - (Min( * image) < 0)
binsize = 1.0 > (range / 255.)
histdata = Histogram(Byte( * image), Binsize = binsize, Max = Max( * image))

;; Plot the histogram of the display image.
cWinID = !D.Window
IF N_Elements(wid) NE 0 THEN WSet, wid
bins = Findgen(N_Elements(histdata)) * binsize + (Min( * image) < 0)
xrange = [Min(bins), Max(bins) + 1]
yrange = [0, maxValue]
Plot, bins, histdata, YTitle = 'Pixel density', $
 Background = background, Color = color, / NoData, $
 XRange = xrange, XStyle = 1, Max_Value = maxValue,  $
 XTitle = 'Image value', _Extra = extra, $ ;, Title = 'Image Histogram'
 XTickformat = '(I6)', YTickformat = '(I6)', YRange = yrange, YStyle = 1, $
 Position = [0.22, 0.2, 0.93, 0.85], charsize = 1.1
FOR j = 0L, N_Elements(bins) - 2 DO BEGIN
   PlotS, [bins[j], bins[j], bins[j + 1], bins[j + 1]], $
    [0, histdata[j] < !Y.Crange[1], histdata[j] < !Y.Crange[1], 0], $
    Color = color
ENDFOR

IF cWinID GT 0 THEN WSet, cWinID
END
;  of Xthreshold_ROI_HISTOPLOT   *********************************************


PRO XTHRESHOLD_ROI_done, event
;; return the threshold stretched image to the main program

Widget_Control, event.top, Get_UValue = info_xstr, / No_Copy

Widget_control, event.id, get_uvalue = button_set
;; can only be APPLY or QUIT, if APPLY then perform these extra
;; settings to output the
result = 0 & xt_min = 0 & xt_top = 0
IF button_set EQ 'ACCEPT' THEN BEGIN
;   * info_xstr.threshold_image = $
;    BytScl( * info_xstr.image GE info_xstr.minThresh, Top = info_xstr.top)
;    BytScl( * info_xstr.image, Top = info_xstr.top,  $
;            Max = info_xstr.maxThresh, Min = info_xstr.minThresh)
   xt_min = info_xstr.minThresh & xt_top = info_xstr.top
   result = BytScl( * info_xstr.image GE xt_min, Top = xt_top)
   ;;dummy = Routine_Names('xthreshold_result', result, Store = 2)
ENDIF
save, result, xt_min, xt_top, filename = 'x_result.sav'

;; quit the application
Widget_Control, event.top, / Destroy

;; perform cleanup
IF info_xstr.newPointer THEN Ptr_Free, info_xstr.image
WDelete, info_xstr.pixmap
END
;  of Xthreshold_ROI_done   *********************************************




PRO XTHRESHOLD_ROI_SAVEAS, event

;; Error handling
Catch, theError
IF theError NE 0 THEN BEGIN
   Catch, / Cancel
   ok = Error_Message( / Traceback)
   IF N_Elements(info_xstr) NE 0 THEN $
    Widget_Control, event.top, Set_UValue = info_xstr, / No_Copy
   RETURN
ENDIF

;; Save as various file types.
Widget_Control, event.top, Get_UValue = info_xstr, / No_Copy
Widget_Control, event.id, Get_UValue = saveAsType

;; Set the current graphics window
cWinID = !D.Window
WSet, info_xstr.windex
TVLCT, info_xstr.r, info_xstr.g, info_xstr.b

;; What kind of file do you want?
filename = 'threshold_roi'
CASE saveAsType OF
   'JPEG': dummy = TVRead(Filename = filename, / JPEG)
   'PNG': dummy = TVRead(Filename = filename, / PNG)
;   'TIFF': dummy = TVRead(Filename = filename, / TIFF)
   'GIF': dummy = TVRead(Filename = filename, / GIF)
   'PICT': dummy = TVRead(Filename = filename, / PICT)
   'BMP': dummy = TVRead(Filename = filename, / BMP)
;   'MAIN_VARIABLE': BEGIN
;         varname = TextBox(Title='Provide Main-Level Variable Name...', $
;            Label='Variable Name: ', Cancel=cancelled, XSize=200, $
;                           Value = 'thresholded_image')
            ; The ROUTINE_NAMES function is not documented in IDL,
            ; so it may not always work. This capability has been
            ; tested in IDL versions 5.3 through 5.6 and found to work.
;      IF NOT cancelled THEN BEGIN
;         displayImage = $
;          BytScl( * info_xstr.image, Top = info_xstr.top,  $
;                  Max = info_xstr.maxThresh, Min = info_xstr.minThresh)
;         dummy = Routine_Names('xthreshold_image', displayImage, Store = 1)
;      ENDIF
;   END
ENDCASE

IF cWinID GT 0 THEN WSet, cWinID
;; Put the info_xstr structure back.
Widget_Control, event.top, Set_UValue = info_xstr, / No_Copy
END
; of Xthreshold_roi_saveas   *********************************************



PRO XTHRESHOLD_ROI_PROCESS_EVENTS, event
;; This event handler ONLY responds to button down events from the
;; draw widget. If it gets a DOWN event, it does two things: (1) finds
;; out which threshold line is to be moved, and (2) changes the
;; event handler for the draw w;idget to XTHRESHOLD_ROI_MOVELINE.

;; Error Handling.
Catch, theError
IF theError NE 0 THEN BEGIN
   Catch, / Cancel
   ok = Error_Message( / Traceback)
   IF N_Elements(info_xstr) NE 0 THEN $
    Widget_Control, event.top, Set_UValue = info_xstr, / No_Copy
   RETURN
ENDIF

possibleEventTypes = [ 'DOWN', 'UP', 'MOTION', 'SCROLL' ]
thisEvent = possibleEventTypes(event.type)
IF thisEvent NE 'DOWN' THEN RETURN

;; Must be DOWN event to get here, so get info_xstr structure.
Widget_Control, event.top, Get_UValue = info_xstr, / No_Copy

;; Make sure you have the correct plotting environment.
current_bangp = !P
Current_bangx = !X
Current_bangy = !Y

!P = Info_Xstr.pbang
!X = Info_Xstr.xbang
!Y = Info_Xstr.ybang

;; Convert the device coordinates to data coordinates.
;; Have to have scaling factors for conversion.
cWinID = !D.Window
Wset, info_xstr.histo_wid
TVLCT, info_xstr.r, info_xstr.g, info_xstr.b
coords = Convert_Coord(event.x, event.y, 0, / Device, / To_Data)

;; Is this event close to a line? If not, ignore it.
;; Click has to be inside the graph in the y direction.
IF coords(1) LT info_xstr.ymin OR coords(1) GT info_xstr.ymax THEN BEGIN
   Widget_Control, event.top, Set_UValue = info_xstr, / No_Copy
   ;; Put the info_xstr structure back into its storage location
   ;; and set things back.
   !P = Current_bangp
   !X = Current_bangx
   !Y = Current_bangy
   IF cWinID GT 0 THEN WSet, cWinID
   RETURN
ENDIF

;; How close to either line are you?
closemin = Abs(info_xstr.minthresh - coords(0))
closemax = Abs(info_xstr.maxthresh - coords(0))
IF closemin LE closemax THEN info_xstr.lineby = 'MIN' ELSE info_xstr.lineby = 'MAX'
;; If you are not close to a line, goodbye!
CASE info_xstr.lineby OF
   'MIN': BEGIN
      IF closemin GT info_xstr.close THEN BEGIN
         Widget_Control, event.top, Set_UValue = info_xstr, / No_Copy
         ;; Put the info_xstr structure back into its storage location and
         ;; set things back.
         !P = Current_bangp
         !X = Current_bangx
         !Y = Current_bangy
         IF cWinID GT 0 THEN WSet, cWinID
         RETURN
      ENDIF
   END

   'MAX': BEGIN
      IF closemax GT info_xstr.close THEN BEGIN
         Widget_Control, event.top, Set_UValue = info_xstr, / No_Copy
         ;; Put the info_xstr structure back into its storage location and
         ;; set things back.
         !P = Current_bangp
         !X = Current_bangx
         !Y = Current_bangy
         IF cWinID GT 0 THEN WSet, cWinID
         RETURN
      ENDIF
   END
ENDCASE

;; Change the event handler for the draw widget and turn MOTION events ON.
Widget_Control, event.id, Event_Pro = 'XTHRESHOLD_ROI_MOVELINE', $
 Draw_Motion_Events = 1

;; Put the info_xstr structure back into its storage location and set things back.
!P = Current_bangp
!X = Current_bangx
!Y = Current_bangy
IF cWinID GT 0 THEN WSet, cWinID

Widget_Control, event.top, Set_UValue = info_xstr, / No_Copy
END
; of XTHRESHOLD_ROI_PROCESS_EVENTS *********************************************



PRO Xthreshold_ROI_MOVELINE, event
;; This event handler continuously draws and erases a threshold line
;; until it receives an UP event from the draw widget. Then it turns
;; draw widget motion events OFF and changes the event handler for the
;; draw widget back to XTHRESHOLD_ROI_PROCESS_EVENTS.

;; Error Handling.
Catch, theError
IF theError NE 0 THEN BEGIN
   Catch, / Cancel
   ok = Error_Message( / Traceback)
   IF N_Elements(info_xstr) NE 0 THEN $
    Widget_Control, event.top, Set_UValue = info_xstr, / No_Copy
   RETURN
ENDIF

;; Get the info_xstr structure out of the top-level base.
Widget_Control, event.top, Get_UValue = info_xstr, / No_Copy

;; Make sure you have the correct plotting environment.
current_bangp = !P
Current_bangx = !X
Current_bangy = !Y
!P = info_xstr.pbang
!X = Info_Xstr.xbang
!Y = Info_Xstr.ybang

cWinID = !D.Window

;; Load image colors.
TVLCT, info_xstr.r, info_xstr.g, info_xstr.b

;; What type of an event is this?
possibleEventTypes = [ 'DOWN', 'UP', 'MOTION', 'SCROLL' ]
thisEvent = possibleEventTypes(event.type)

IF thisEvent EQ 'UP' THEN BEGIN
   ;; If this is an UP event, set the draw widget's event handler back
   ;; to XTHRESHOLD_ROI_PROCESS_EVENTS, turn MOTION events OFF, and apply the
   ;; new threshold parameters to the image.

   ;; Erase the last theshold line drawn.
   cWinID = !D.Window
   WSet, info_xstr.histo_wid
   TVLCT, info_xstr.r, info_xstr.g, info_xstr.b
   Device, Copy = [0, 0, info_xstr.pix_xsize, info_xstr.pix_ysize, 0, 0, info_xstr.pixmap]

   ;; Turn motion events off and redirect the events
   ;; to XTHRESHOLD_ROI_PROCESS_EVENTS.
   Widget_Control, event.id, Draw_Motion_Events = 0, $
    Event_Pro = 'Xthreshold_Roi_Process_Events'

   ;; Convert the event device coordinates to data coordinates.
   coord = Convert_Coord(event.x, event.y, / Device, / To_Data)

   ;; Make sure the coordinate is between the other line and
   ;; still inside the plot.
   CASE info_xstr.lineby OF
      'MIN': BEGIN
         coord(0) = coord(0) > info_xstr.xmin
         coord(0) = coord(0) < (info_xstr.maxThresh - 1)
      END
      'MAX': BEGIN
         coord(0) = coord(0) > (info_xstr.minThresh + 1)
         coord(0) = coord(0) < info_xstr.xmax
      END
   ENDCASE

   ;; Draw both of the threshold lines again.
   CASE info_xstr.lineby OF
      'MIN': BEGIN
         cmax = Convert_Coord(info_xstr.maxThresh, 0, / Data, / To_Normal)
         cmin = Convert_Coord(coord[0], 0, / Data, / To_Normal)
         PlotS, [coord(0), coord(0)], [info_xstr.ymin, info_xstr.ymax], $
          Color = info_xstr.minColor, Thick = 3
;         PlotS, [info_xstr.maxThresh, info_xstr.maxThresh],  $
;          [info_xstr.ymin, info_xstr.ymax], $
;          Color = info_xstr.maxColor, Thick = 3
         info_xstr.minthresh = coord(0)
         XYOuts, cmin[0], 0.90, / Normal, $
          StrTrim(Fix(info_xstr.minThresh), 2), $
          Color = info_xstr.minColor, Font = 0, Alignment = 0.5
;         XYOuts, cmax[0], 0.90, / Normal, $
;          StrTrim(Fix(info_xstr.maxThresh), 2), $
;          Color = info_xstr.maxColor, Alignment = 0.5, Font = 0
      END
      'MAX': BEGIN
         cmax = Convert_Coord(coord[0], 0, / Data, / To_Normal)
         cmin = Convert_Coord(info_xstr.minThresh, 0, / Data, / To_Normal)
;         PlotS, [coord(0), coord(0)], [info_xstr.ymin, info_xstr.ymax],  $
;          Color = info_xstr.maxColor, Thick = 3
         PlotS, [info_xstr.minThresh, info_xstr.minThresh],  $
          [info_xstr.ymin, info_xstr.ymax], Color = info_xstr.minColor, $
          Thick = 3
         info_xstr.maxthresh = coord(0)
         XYOuts, cmin[0], 0.90, / Normal, $
          StrTrim(Fix(info_xstr.minThresh), 2), $
          Color = info_xstr.minColor, Font = 0, Alignment = 0.5
;         XYOuts, cmax[0], 0.90, / Normal, $
;          StrTrim(Fix(info_xstr.maxThresh), 2), $
;          Color = info_xstr.maxColor, Alignment = 0.5, Font = 0
      END
   ENDCASE

   ;; Update the image display by appling the threshold parameters.
   ;; Be sure the image draw widget is still around. Make it if it isn't.
   displayImage = BytScl( * info_xstr.image GE info_xstr.minThresh, $
                          Top = info_xstr.ncolors - 1)

;BytScl( * info_xstr.image, Top = info_xstr.ncolors - 1,  $
;                          Max = info_xstr.maxThresh, Min = info_xstr.minThresh)
   IF info_xstr.notify_pro NE "" THEN $
    Call_Procedure, $
    info_xstr.notify_pro, displayImage, _Extra = info_xstr.extra

;BytScl( * info_xstr.image, Top = info_xstr.top,  $
;                             Max = info_xstr.maxThresh, Min = info_xstr.minThresh), _Extra = info_xstr.extra
   IF Obj_Valid(info_xstr.notify_obj.object) THEN $
    Call_Method, info_xstr.notify_obj.method, info_xstr.notify_obj.object, $
    displayImage, _Extra = info_xstr.extra

   IF Widget_Info(info_xstr.image_draw, / Valid_ID) THEN BEGIN
      WSet, info_xstr.windex
      TVLCT, info_xstr.r, info_xstr.g, info_xstr.b
      TVImage, displayImage, _Extra = info_xstr.extra
      IF info_xstr.notify_pro NE "" THEN Call_Procedure, $
       info_xstr.notify_pro, $
       displayImage, _Extra = info_xstr.extra
      IF Obj_Valid(info_xstr.notify_obj.object) THEN $
       Call_Method, info_xstr.notify_obj.method, $
       info_xstr.notify_obj.object, $
       displayImage, _Extra = info_xstr.extra
   ENDIF ELSE BEGIN
      imageSize = Size( * info_xstr.image)
      xsize = imageSize(1)
      ysize = imageSize(2)
      aspect = Float(xsize) / ysize
      IF xsize GT 512 OR ysize GT 512 THEN BEGIN
         IF xsize NE ysize THEN BEGIN
            aspect = Float(ysize) / xsize
            IF aspect LT 1 THEN BEGIN
               xsize = 512
               ysize = (512 * aspect) < 512
            ENDIF ELSE BEGIN
               ysize = 512
               xsize = (512 / aspect) < 512
            ENDELSE
         ENDIF
      ENDIF
      Widget_Control, event.top, TLB_Get_Offset = offsets, $
       TLB_Get_Size = sizes
      xoff = offsets[0] + sizes[0] + 20
      yoff = offsets[1]
      image_tlb = $
       Widget_Base(Row = 1, Group = event.top, Title = 'Threshold image', $
                   XOffSet = xoff, YOffSet = yoff, TLB_frame_attr = 17)
      image_draw = Widget_Draw(image_tlb, XSize = xsize, YSize = ysize)
      Widget_Control, image_tlb, / Realize, Set_UValue = event.top
      Widget_Control, image_draw, Get_Value = windex
      info_xstr.image_draw = image_draw
      info_xstr.windex = windex
      TVImage, displayImage, _Extra = info_xstr.extra

      XManager, 'xthreshold_roi_image', image_tlb, / No_Block
      Widget_Control, info_xstr.saveas, Sensitive = 1
   ENDELSE

   ;; Update the pixmap with histogram with no threshold lines.
   Xthreshold_Roi_Histoplot, info_xstr.image, $
    Background = info_xstr.backColor, $
    Color = info_xstr.drawColor, _Extra = info_xstr.extra, $
    Max_Value = info_xstr.maxValue, WID = info_xstr.pixmap, $
    Red = info_xstr.r, Green = info_xstr.g, Blue = info_xstr.b

   ;; Put the info_xstr structure back into its storage location and then,
   ;; out of here!
   Widget_Control, event.top, Set_UValue = info_xstr, / No_Copy
   IF cWinID GT 0 THEN WSet, cWinID
   RETURN
ENDIF                           ;; thisEvent = UP


;; Most of the action in this event handler occurs here while we are waiting
;; for an UP event to occur. As long as we don't get it, keep erasing the
;; old threshold line and drawing a new one.

;; Get current window and scaling parameters in order.
WSet, info_xstr.histo_wid
TVLCT, info_xstr.r, info_xstr.g, info_xstr.b
!P = Info_Xstr.pbang
!X = Info_Xstr.xbang
!Y = Info_Xstr.ybang
coord = Convert_Coord(event.x, event.y, / Device, / To_Data)

;; Draw the "other" line on the pixmap (so you don't have to draw
;; it all the time).
WSet, info_xstr.pixmap
CASE info_xstr.lineby OF
   'MIN': BEGIN
      cmax = Convert_Coord(info_xstr.maxThresh, 0, / Data, / To_Normal)
;      PlotS, [info_xstr.maxthresh, info_xstr.maxthresh], $
;       [info_xstr.ymin, info_xstr.ymax],  $
;       Color = info_xstr.maxColor, Thick = 3
;      XYOuts, cmax[0], 0.90, / Normal, StrTrim(Fix(info_xstr.maxThresh), 2), $
;       Color = info_xstr.maxColor, Font = 0, Alignment = 0.5
   END
   'MAX': BEGIN
      cmin = Convert_Coord(info_xstr.minThresh, 0, / Data, / To_Normal)
      PlotS, [info_xstr.minthresh, info_xstr.minthresh], $
       [info_xstr.ymin, info_xstr.ymax],  $
       Color = info_xstr.minColor, Thick = 3
      XYOuts, cmin[0], 0.90, / Normal, StrTrim(Fix(info_xstr.minThresh), 2), $
       Color = info_xstr.minColor, Alignment = 0.5, Font = 0
   END
ENDCASE

;; Erase the old threshold line.
WSet, info_xstr.histo_wid
TVLCT, info_xstr.r, info_xstr.g, info_xstr.b
Device, Copy = [0, 0, info_xstr.pix_xsize, info_xstr.pix_ysize, 0, 0, $
                info_xstr.pixmap]

;; Draw the new line at the new coordinate. Make sure the coordinate
;; is inside the plot and doesn't go over the other line.
CASE info_xstr.lineby OF
   'MIN': BEGIN
      coord(0) = coord(0) > (info_xstr.xmin)
      coord(0) = coord(0) < (info_xstr.maxThresh - 1)
   END
   'MAX': BEGIN
      coord(0) = coord(0) > (info_xstr.minThresh + 1)
      coord(0) = coord(0) < (info_xstr.xmax )
   END
ENDCASE
cmax = Convert_Coord(info_xstr.maxThresh, 0, / Data, / To_Normal)
cmin = Convert_Coord(info_xstr.minThresh, 0, / Data, / To_Normal)

CASE info_xstr.lineby OF
   'MIN': BEGIN
      PlotS, [coord(0), coord(0)], [info_xstr.ymin, info_xstr.ymax], $
       Color = info_xstr.minColor, Thick = 3
      XYOuts, Float(event.x) / !D.X_Size, 0.90, / Normal, $
       StrTrim(Fix(coord(0)), 2), Color = info_xstr.minColor, $
       Alignment = 0.5, Font = 0
   END
   'MAX': BEGIN
;      PlotS, [coord(0), coord(0)], [info_xstr.ymin, info_xstr.ymax], $
;       Color = info_xstr.maxColor, Thick = 3
;      XYOuts, Float(event.x) / !D.X_Size, 0.90, / Normal, $
;       StrTrim(Fix(coord(0)), 2), Color = info_xstr.maxColor, $
;       Alignment = 0.5, Font = 0
   END
ENDCASE

;; Set things back.
!P = Current_bangp
!X = Current_bangx
!Y = Current_bangy
IF cWinID GT 0 THEN WSet, cWinID

;; Put the info_xstr structure back into its storage location.
Widget_Control, event.top, Set_UValue = info_xstr, / No_Copy
END
;; of XTHRESHOLD_ROI_MOVELINE **************************************


PRO XTHRESHOLD_ROI_MAXVALUE, event
;; Error Handling.
Catch, theError
IF theError NE 0 THEN BEGIN
   Catch, / Cancel
   ok = Error_Message( / Traceback)
   IF N_Elements(info_xstr) NE 0 THEN $
    Widget_Control, event.top, Set_UValue = info_xstr, / No_Copy
   RETURN
ENDIF

Widget_Control, event.top, Get_UValue = info_xstr, / No_Copy
cWinID = !D.Window

;; Get the new max value.
Widget_Control, event.id, Get_UValue = maxValue
info_xstr.maxValue = maxValue

;; Update the histogram plot.
Xthreshold_Roi_Histoplot, info_xstr.image, Background = info_xstr.backColor, $
 Color = info_xstr.drawColor, _Extra = info_xstr.extra, $
 Max_Value = info_xstr.maxValue, WID = info_xstr.histo_wid, Red = info_xstr.r, $
 Green = info_xstr.g, Blue = info_xstr.b

;; Draw threshold lines on the histogram plot.
WSet, info_xstr.histo_wid
PlotS, [info_xstr.minThresh, info_xstr.minThresh], $
 [!Y.Crange(0), !Y.Crange(1)], $
 Color = info_xstr.minColor, Thick = 3
;PlotS, [info_xstr.maxThresh, info_xstr.maxThresh], $
; [!Y.Crange(0), !Y.Crange(1)], $
; Color = info_xstr.maxColor, Thick = 3
cmax = Convert_Coord(info_xstr.maxThresh, 0, / Data, / To_Normal)
cmin = Convert_Coord(info_xstr.minThresh, 0, / Data, / To_Normal)
XYOuts, cmin[0], 0.90, / Normal, StrTrim(Fix(info_xstr.minThresh), 2), $
 Color = info_xstr.minColor, Alignment = 0.5, Font = 0
;XYOuts, cmax[0], 0.90, / Normal, StrTrim(Fix(info_xstr.maxThresh), 2), $
; Color = info_xstr.maxColor, Alignment = 0.5, Font = 0

;; Update the pixmap with histogram with no threshold lines.
Xthreshold_Roi_Histoplot, info_xstr.image, Background = info_xstr.backColor, $
 Color = info_xstr.drawColor, _Extra = info_xstr.extra, $
 Max_Value = info_xstr.maxValue, WID = info_xstr.pixmap, Red = info_xstr.r, $
 Green = info_xstr.g, Blue = info_xstr.b
IF cWinID GT 0 THEN WSet, cWinID

Widget_Control, event.top, Set_UValue = info_xstr, / No_Copy
END
;; of XTHRESHOLD_ROI_IMAGE_maxvalue ******************************************




;PRO Xthreshold_ROI_CLEANUP, tlb
;Widget_Control, tlb, Get_UValue = info_xstr
;IF N_Elements(info_xstr) NE 0 THEN BEGIN
;   IF info_xstr.newPointer THEN Ptr_Free, info_xstr.image
;   WDelete, info_xstr.pixmap
;ENDIF
;END
;; of xthreshold_roi_cleanup  ******************************************



;PRO Xthreshold_ROI_QUIT, event
;Widget_Control, event.top, / Destroy
;END
;; of XTHRESHOLD_ROI_QUIT ******************************************




PRO Xthreshold_ROI, theImage, Group_Leader = group, NColors = ncolors, $
                    Max_Value = maxValue, Colortable = ctable, _EXTRA = extra, $
                    Notify_Pro = notify_pro, $
                    XPos = xpos, YPos = ypos, Title = title, $
                    Notify_Obj = notify_obj, Block = block, Top = top ;, $
;                    threshold_image = threshold_image, No_Window = no_window
;; Error Handling.
Catch, theError
IF theError NE 0 THEN BEGIN
   Catch, / Cancel
   ok = Error_Message( / Traceback)
   RETURN
ENDIF
cWinID = !D.Window

histXsize = 300 ; 500
histYsize = 210 ; 350

On_Error, 1
Device, Decomposed = 0

;; Is image a pointer? If not, make it one.
IF Size(theImage, / TName) NE 'POINTER' THEN BEGIN
   image = Ptr_New(theImage)
   newPointer = 1
ENDIF ELSE BEGIN
   image = theImage
   newPointer = 0
ENDELSE

imgsize = Size( * image)
IF imgsize(0) NE 2 THEN $
 Message, 'First positional parameter must be a 2D image.'
xsize = imgsize(1)
ysize = imgsize(2)

;; Default values for keywords.
IF N_Elements(maxValue) EQ 0 THEN BEGIN
   numPixels = N_Elements( * image)
   maxValue =  15000.0
   IF numPixels LE 65536L THEN maxValue = 5000.0
   IF numPixels GT 65536L AND numPixels LE 262144L THEN maxValue = 20000.0
ENDIF
IF N_Elements(extra) EQ 0 THEN extra = {TITLE:''}
IF N_Elements(notify_pro) EQ 0 THEN notify_pro = ""
IF N_Elements(notify_obj) EQ 0 THEN $
 notify_obj = {object:Obj_New(), method:""} ELSE BEGIN
   IF Size(notify_obj, / TNAME) NE 'STRUCT' THEN $
    Message, 'NOTIFY_OBJ keyword requires structure variable'
   names = Tag_Names(notify_obj)
   index = Where(names EQ "METHOD", count)
   IF count EQ 0 THEN Message, 'NOTIFY_OBJ structure requires METHOD field.'
   index = Where(names EQ "OBJECT", count)
   IF count EQ 0 THEN Message, 'NOTIFY_OBJ structure requires OBJECT field.'
   IF Obj_Valid(notify_obj.object) EQ 0 THEN $
    Message, 'NOTIFY_OBJ object is invalid.'
ENDELSE
;no_window = Keyword_Set(no_window)
IF N_Elements(xpos) EQ 0 THEN xpos = 100
IF N_Elements(ypos) EQ 0 THEN ypos = 100
IF N_Elements(title) EQ 0 THEN title = 'Drag colorbar...'
IF N_Elements(top) EQ 0 THEN top = 255 ELSE top = top < 255

;; Check for availability of GIF files.
thisVersion = Float(!Version.Release)
IF thisVersion LT 5.4 THEN haveGif = 1 ELSE haveGIF = 0

IF N_Elements(ncolors) EQ 0 THEN BEGIN
   ;; Find out how many colors you have.
   ncolors = !D.Table_Size - 4

   IF ncolors LT 24 THEN BEGIN
      Message, 'Not enough colors available to continue. Returning.'
      RETURN
   ENDIF

   minColor = ncolors
   maxColor = ncolors + 1
   backColor = ncolors + 2
   drawColor = ncolors + 3
ENDIF ELSE BEGIN
   ;; We will scale to as many colors as we have, less 4 drawing colors.
   ;; Must have at least 20 data colors.
   officialColors = !D.Table_Size
   ncolors = (ncolors - 4) < (officialColors - 4)
   IF ncolors LT 24 THEN BEGIN
      Message, 'Not enough colors available to continue. Returning.'
      RETURN
   ENDIF
   minColor = ncolors
   maxColor = ncolors + 1
   backColor = ncolors + 2
   drawColor = ncolors + 3
ENDELSE

;; Create the histogram widget.
histo_tlb = $
 Widget_Base(Title = title, XOffset = xpos, / column, $
             YOffset = ypos, tlb_frame_attr = 17)
; Widget_Base(Row = 1, Title = title, MBar = menubaseID, XOffset = xpos, $
;             YOffset = ypos, tlb_frame_attr = 17)
toolbase = $
 Widget_Base(histo_tlb, / row)

histo_draw = $
 Widget_Draw(histo_tlb, XSize = histXsize, YSize = histYsize, $
             Button_Events = 1, Event_Pro = 'Xthreshold_Roi_Process_Events')
controlID = $
 Widget_Button(toolbase, Value = 'Controls', $
               Event_Pro = 'Xthreshold_Roi_MaxValue', / menu)
; Widget_Button(menubaseID, Value = 'Controls', $
;               Event_Pro = 'Xthreshold_Roi_MaxValue')
dummy = $
 Widget_Button(toolbase, Value = 'Quit', uvalue = 'QUIT', $
               Event_Pro = 'Xthreshold_Roi_Done')
dummy = $
 Widget_Button(toolbase, Value = 'Accept', uvalue = 'ACCEPT', $
               Event_Pro = 'Xthreshold_Roi_Done')
saveAs = $
 Widget_Button(controlID, Value = 'Save Image As', $
               Event_Pro = "Xthreshold_Roi_SaveAs", / Menu)
dummy = Widget_Button(saveAs, Value = 'BMP File', UValue = 'BMP')
dummy = Widget_Button(saveAs, Value = 'JPEG File', UValue = 'JPEG')
dummy = Widget_Button(saveAs, Value = 'PNG File', UValue = 'PNG')
dummy = Widget_Button(saveAs, Value = 'PICT File', UValue = 'PICT')
;dummy = Widget_Button(saveAs, Value = 'TIFF File', UValue = 'TIFF')
IF havegif THEN dummy = Widget_Button(saveAs, Value = 'GIF File', $
                                      UValue = 'GIF')
;dummy = Widget_Button(saveAs, Value = 'Main Level Variable...', $
;                      UValue = 'MAIN_VARIABLE')
maxID = Widget_Button(controlID, Value = 'Max pixel density', / Menu, $
                      / Separator)
dummy = Widget_Button(maxID, Value = '1000', UValue = 1000.0)
dummy = Widget_Button(maxID, Value = '2000', UValue = 2000.0)
dummy = Widget_Button(maxID, Value = '5000', UValue = 5000.0)
dummy = Widget_Button(maxID, Value = '10000', UValue = 10000.0)
dummy = Widget_Button(maxID, Value = '20000', UValue = 20000.0)
dummy = Widget_Button(maxID, Value = '30000', UValue = 30000.0)
dummy = Widget_Button(maxID, Value = '50000', UValue = 50000.0)
;quitter = Widget_Button(controlID, Value = 'Quit', $
;                        Event_Pro = 'Xthreshold_Roi_Quit', / Separator)
Widget_Control, histo_tlb, / Realize

;; Create a pixmap window for moving and erasing the histogram
;; threshold bars.
Window, Pixmap = 1, XSize = histXsize, YSize = histYsize, / Free
pixmap = !D.Window

;; Create an image window for displaying the image.
;IF NOT Keyword_Set(no_window) THEN BEGIN
   Widget_Control, histo_tlb, TLB_Get_Offset = offsets, TLB_Get_Size = sizes
   xoff = offsets[0] + sizes[0] + 20
   yoff = offsets[1]
   aspect = Float(xsize) / ysize
   IF xsize GT 512 OR ysize GT 512 THEN BEGIN
      IF xsize NE ysize THEN BEGIN
         aspect = Float(ysize) / xsize
         IF aspect LT 1 THEN BEGIN
            xsize = 512
            ysize = (512 * aspect) < 512
         ENDIF ELSE BEGIN
            ysize = 512
            xsize = (512 / aspect) < 512
         ENDELSE
      ENDIF
   ENDIF
   image_tlb = $
    Widget_Base(Row = 1, Group_Leader = histo_tlb, Title = 'Threshold image', $
                XOffSet = xoff, YOffSet = yoff, TLB_frame_attr = 17)
   image_draw = $
    Widget_Draw(image_tlb, XSize = xsize, YSize = ysize, $
                Kill_Notify = 'Xthreshold_Roi_ImageWindowKilled', $
                UValue = [saveAs])
   Widget_Control, image_tlb, / Realize

   ;; Get window index numbers for the draw widgets.
   Widget_Control, image_draw, Get_Value = windex
;ENDIF ELSE BEGIN
;   image_tlb = - 1L
;   image_draw = - 1L
;   windex = - 1L
;ENDELSE

Widget_Control, histo_draw, Get_Value = histo_wid

;; Load a colortable if requested.
IF N_Elements(ctable) NE 0 THEN $
 LoadCt, 0 > ctable < 40, NColors = ncolors, / Silent ELSE $
 LoadCT, 0, NColors = ncolors, / Silent

;; Load drawing colors.
TVLct, 255b, 255b, 0b, minColor ; Yellow color.
TVLct, 0b, 255b, 0b, maxColor   ; Green color
TVLct, 0b, 0b, 0b, backColor    ; black color
TvLct, 255b, 255b, 255b, drawColor ; White color

;; Get the current color table vectors for storage.
TVLCT, r, g, b, / Get

;; Start with 2% linear threshold on both ends.
IF size( * image, / type) EQ 1 THEN $
 maxVal = Max( * image) + 1 < 255 ELSE maxVal = Max( * image)
maxThresh = 1.0 * maxVal
minVal = Min( * image) < 0.0
;minThresh = minVal + (0.02 * maxVal)
minThresh = minVal + (0.5 * maxVal)
Xthreshold_Roi_Histoplot, image, Background = backColor, Color = drawColor, $
 Max_Value = maxValue, _Extra = extra, WID = histo_wid

;; Store the plotting system variables for later recall.
pbang = !P
xbang = !X
ybang = !Y
ymin = !Y.CRange[0]
ymax = !Y.CRange[1]
xmin = !X.CRange[0]
xmax = !X.CRange[1]

;; Put the same plot in the pixmap.
WSet, pixmap
Device, Copy = [0, 0, histXsize, histYsize, 0, 0, histo_wid]

;; Save the scaling factors for calculating data coordinates.
xs = !X.S
ys = !Y.S

;; Draw threshold lines.
WSet, histo_wid
PlotS, [minThresh, minThresh], [!Y.Crange(0), !Y.Crange(1)], $
 Color = minColor, Thick = 3
;PlotS, [maxThresh, maxThresh], [!Y.Crange(0), !Y.Crange(1)], $
; Color = maxColor, Thick = 3
;cmax = Convert_Coord(maxThresh, 0, / Data, / To_Normal)
cmin = Convert_Coord(minThresh, 0, / Data, / To_Normal)
XYOuts, cmin[0], 0.90, / Normal, StrTrim(Fix(minThresh), 2), $
 Color = minColor, Alignment = 0.5, Font = 0
;XYOuts, cmax[0], 0.90, / Normal, StrTrim(Fix(maxThresh), 2), $
; Color = maxColor, Alignment = 0.5, Font = 0


;; Display the image after thresholding.
displayImage = BytScl( * image GE minThresh, Top = ncolors - 1) ;
;, Max = maxThresh, Min = minThresh)
;IF NOT Keyword_Set(no_window) THEN BEGIN
   WSet, windex
   TVImage, displayImage, _Extra = extra
;ENDIF
IF notify_pro NE "" THEN Call_Procedure, notify_pro, $
 displayImage, _Extra = extra
; BytScl( * image, Top = top, Max = maxThresh, Min = minThresh), _Extra = extra
IF Obj_Valid(notify_obj.object) THEN $
 Call_Method, notify_obj.method, notify_obj.object, $
 displayImage, _Extra = extra

;; Calculate a value to tell you if you are "close" to a threshold line.
;close = 0.05 * (maxval - minval)
close = 5.0

;; Make an info_xstr structure with all info_xstr to run the program.
info_xstr = {image:image, $          ; A pointer to the image data
;        threshold_image:threshold_image, $ ;the output image if threshold applied
        minThresh:minThresh, $  ; The minimum threshold
        maxThresh:maxThresh, $  ; The maximum threshold
        ncolors:ncolors, $      ; The number of colors
        top:top, $              ; The top of the byte scaling.
        minColor:minColor, $    ; The minimum drawing color index
        maxColor:maxColor, $    ; The maximum drawing color index
        backColor:backColor, $  ; The background drawing color index
        drawColor:drawColor, $  ; The plot drawing color index
        histo_wid:histo_wid, $  ; The histogram window index number
        histo_draw:histo_draw, $ ; The histogram draw widget ID.
        image_draw:image_draw, $ ; The image draw widget ID.
        maxValue:maxValue, $    ; The maximum value of the plot
        windex:windex, $        ; The image window index
        ymin:ymin, $            ; The ymin in data coordinates
        ymax:ymax, $            ; The ymax in data coordinates
        xmin:xmin, $            ; The xmin in data coordinates
        xmax:xmax, $            ; The xmax in data coordinates
        pbang:pbang, $          ; The !P system variable.
        xbang:xbang, $          ; The !X system variable.
        ybang:ybang, $          ; The !Y system variable.
        lineby:'MIN', $         ; The line you are close to.
        linex:minThresh, $      ; The x coordinate of line (data coords).
        pixmap:pixmap, $        ; The pixmap window index
        minval:minval, $        ; The minimum intensity value of the data
        maxval:maxval, $        ; The maximum intensity value of the data
        r:r, $                  ; The red color table vector.
        g:g, $                  ; The green color table vector.
        b:b, $                  ; The blue color table vector.
        notify_pro:notify_pro, $ ;procedurename 2 notify when image is stretched
        notify_obj:notify_obj, $ ;object refer+method 2 notify when ...
;        no_window:no_window, $  ; A flag that, if set, means no image window.
        extra:extra, $          ; The extra keywords for the Plot command.
        pix_xsize:histXsize, $  ; The X size of the pixmap.
        pix_ysize:histYsize, $  ; The Y size of the pixmap.
        newPointer:newPointer, $ ; flag 2 indicate if we made a pointer or not.
        saveAs:saveAs, $        ; The SaveAs button widget identifier.
        close:close}            ; A value to indicate closeness to line

;; Save the info_xstr structure and bring the histogram window forward with SHOW.
Widget_Control, histo_tlb, Set_UValue = info_xstr, / No_Copy, / Show
IF cWinID GE 0 THEN WSet, cWinID

;IF NOT no_window THEN BEGIN
;   Widget_Control, image_tlb, Set_UValue = histo_tlb
;   XManager, 'xthreshold_roi_image', image_tlb, No_Block = 1
;ENDIF
XManager, 'xthreshold_roi', histo_tlb, Group = group, $
 No_Block = 1 - Keyword_Set(block);, Cleanup = 'Xthreshold_Roi_Cleanup'
END
