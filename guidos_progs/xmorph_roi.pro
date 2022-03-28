PRO Xmorph_roi_done, event
;; return the morphed image to the main program
Widget_Control, event.top, Get_UValue = info, / No_Copy
Widget_control, event.id, get_uvalue = eventValue

;; can only be APPLY or QUIT, if APPLY then perform these extra
;; settings to return the morphed image
result = 0 & resultfr = 0
IF strlowCase(eventValue) EQ 'accept' THEN BEGIN
   result = * info.image & resultfr = * info.fullres
   ;;   dummy = Routine_Names('xmorph_result', result, Store = 2)
ENDIF

;; store the result of this process
save, result, resultfr, filename = 'x_result.sav'

;; quit the application
Widget_Control, event.top, / Destroy

;; perform cleanup
IF info.newPointer THEN BEGIN
   Ptr_Free, info.image & Ptr_Free, info.mimage
   Ptr_Free, info.fullres & Ptr_Free, info.mfullres
ENDIF
Ptr_Free, info.th_struc_elem
END
;  of Xmorph_ROI_done   *********************************************


PRO Xmorph_roi_event, event
;; Error Handling.
Catch, theError
IF theError NE 0 THEN BEGIN
   Catch, / Cancel
   ok = Error_Message( / Traceback)
   IF N_Elements(info) NE 0 THEN $
    Widget_Control, event.top, Set_UValue = info, / No_Copy
   RETURN
ENDIF

;; get the info structure
Widget_Control, event.top, Get_UValue = info, / No_Copy

;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue


CASE strlowCase(eventValue) OF
   'test': BEGIN
      ;; setup the kernel and apply the selected filter on the
      ;; original image
      widget_control, info.w_thcontrast, sensitive = (info.mtype_curr EQ 4) 
      GOTO, setup
   END 

   'reset': BEGIN
      * info.image = * info.mimage
      * info.fullres = * info.mfullres
      WSet, info.drawID & tvscl, * info.image 
      widget_control, info.w_thcontrast, sensitive = 0
      widget_control, info.w_thmin, set_value = 30
      widget_control, info.w_thmax, set_value = 60
      info.thmin = 30 & info.thmax = 60
;      widget_control, info.w_morphType, set_droplist_select = 0
;      widget_control, info.w_strucType, set_droplist_select = 0
;      widget_control, info.w_strucSize, set_droplist_select = 0
;      info.mtype_curr = 0 & info.stype_curr = 0 & info.ssize_curr = 1
      GOTO, skip
   END  

   'mtype': BEGIN
      info.mtype_curr = info.morphtype_ls(event.index) 
      widget_control, info.w_thcontrast, sensitive = 0
   GOTO, skip
   END 

   'stype': BEGIN
      info.stype_curr = info.structype_ls(event.index) 
      GOTO, skip
   END 

   'ssize': BEGIN
      info.ssize_curr = info.strucsize_ls(event.index) 
      GOTO, skip
   END 

   'th_min': BEGIN 
      widget_control, info.w_thmin, get_value = mi 
      ;; ensure min is below max
      mi = mi < info.thmax & IF mi EQ info.thmax THEN mi = info.thmax - 1
      widget_control, info.w_thmin, set_value = mi & info.thmin = mi

      tmp = 255b - ( * info.mimage) ; invert brightness of original
      tmp = morph_tophat(tmp, * info.th_struc_elem) ; apply tophat
      * info.image = 255b - BytScl(tmp, min = info.thmin, max = info.thmax)
      tmp = 255b - ( * info.mfullres) ; invert brightness of original
      tmp = morph_tophat(tmp, * info.th_struc_elem) ; apply tophat
      * info.fullres = 255b - BytScl(tmp, min = info.thmin, max = info.thmax)
      WSet, info.drawID & tvscl, * info.image
      GOTO, skip
   END

   'th_max': BEGIN
      widget_control, info.w_thmax, get_value = ma
      ;; ensure min is below max
      ma = ma > info.thmin & IF ma EQ info.thmin THEN ma = info.thmin + 1
      widget_control, info.w_thmax, set_value = ma & info.thmax = ma

      tmp = 255b - (* info.mimage) ; invert brightness of original
      tmp = morph_tophat(tmp, * info.th_struc_elem); apply tophat
      * info.image = 255b - BytScl(tmp, min = info.thmin, max = info.thmax)
      tmp = 255b - ( * info.mfullres) ; invert brightness of original
      tmp = morph_tophat(tmp, * info.th_struc_elem); apply tophat
      * info.fullres = 255b - BytScl(tmp, min = info.thmin, max = info.thmax)
      WSet, info.drawID & tvscl, * info.image
      GOTO, skip
   END


ENDCASE

setup:
;; setup structure element = binary mask of default kernel of selected
;; structure type and size
radius = info.ssize_curr
def_kernel = shift(dist(2 * radius + 1), radius, radius) le radius
      
CASE info.stype_curr OF 
   0: BEGIN ; horizontal
      def_kernel( * , 0:radius - 1) = 0 & def_kernel( *, radius + 1: *) = 0 
   END 
   1: BEGIN ; vertical
      def_kernel(0:radius - 1, * ) = 0 & def_kernel(radius + 1: * , * ) = 0 
   END 
   2: BEGIN ; diagonal down
      def_kernel = rotate(byte(identity(2 * radius + 1)), 1)
   END 
   3: BEGIN ; diagonal up
      def_kernel = byte(identity(2 * radius + 1))
   END 
   4: BEGIN ;circular
      def_kernel = def_kernel
   END 
ENDCASE

;; offer the user to modify the default kernel and setup the structure
;; element
cancel = ptr_new(1b)
selected_kernel = ptr_new(def_kernel)
;; get the kernel settings
get_kernel, selected_kernel = selected_kernel, / morph, $
 cancel = cancel, Group_Leader = event.top, / binary
;; set the default or new kernel if cancel was not selected
set_skip = 0
IF * cancel EQ 0b THEN struc_elem = * selected_kernel ELSE set_skip = 1
;; free and delete the temporary pointers
ptr_free, cancel & undefine, cancel
ptr_free, selected_kernel & undefine, selected_kernel
IF set_skip EQ 1 THEN GOTO, skip

;; apply the structure element according to the selected morph type on
;; the original image
image = * info.mimage & dims = size(image, / dimension)
fullres = * info.mfullres & dimsfr = size(fullres, / dimension)

CASE info.mtype_curr OF 
   0: BEGIN ; dilate
      ;; Dilation adds pixels to perimeters of objects, brightens the image,
      ;; and fills in holes smaller than the structuring element 
      ;;Add a border to the image to avoid generating indeterminate
      ;;values when passing the structuring element over objects along the
      ;;edges of an image. If the starting origin of the structuring element
      ;;is not specified in the call to ERODE, the origin defaults to one
      ;;half the width of the structuring element. Therefore, creating a
      ;;border equal to one half of the structuring element width (equal to
      ;;the radius) is sufficient to avoid indeterminate values. Create
      ;;padded images for both the erode operation (using the maximum array
      ;;value for the border), and the dilate operation (using the minimum
      ;;array value for the border) as follows: 
      dilateimage = replicate(min(image), dims[0] + radius, dims[1] + radius) 
      r2 = radius / 2
      dilateimage [r2, r2] = image
      dilateimage = dilate(dilateimage, struc_elem, / gray)
      ;; go back to original size
      dilateimage = dilateimage[r2:dims(0), r2:dims(1)]
      * info.image = temporary(dilateimage)
      ;; now for fullres
      ;;==================
      dilateimage = replicate(min(fullres), dimsfr[0] + radius, dimsfr[1] + radius) 
      r2 = radius / 2
      dilateimage [r2, r2] = fullres
      dilateimage = dilate(dilateimage, struc_elem, / gray)
      ;; go back to original size
      dilateimage = dilateimage[r2:dimsfr(0), r2:dimsfr(1)]
      * info.fullres = temporary(dilateimage)
   END 
   1: BEGIN ; erode
      ;; Erosion removes pixels from perimeters of objects, decreases the
      ;; overall brightness of the grayscale image and removes objects
      ;; smaller than the structuring element.
      erodeimage = replicate(max(image), dims[0] + radius, dims[1] + radius) 
      r2 = radius / 2
      erodeimage [r2, r2] = image 
      erodeimage = erode(erodeimage, struc_elem, / gray)
      ;; go back to original size
      erodeimage = erodeimage[r2:dims(0), r2:dims(1)]
      * info.image = temporary(erodeimage)
      ;; now for fullres
      ;;==================
      erodeimage = replicate(max(fullres), dimsfr[0] + radius, dimsfr[1] + radius) 
      r2 = radius / 2
      erodeimage [r2, r2] = fullres 
      erodeimage = erode(erodeimage, struc_elem, / gray)
      ;; go back to original size
      erodeimage = erodeimage[r2:dimsfr(0), r2:dimsfr(1)]
      * info.fullres = temporary(erodeimage)
   END 
   2: BEGIN ; open = performing erosion followed by dilation 
      * info.image = morph_open(image, struc_elem, / gray)
      * info.fullres = morph_open(fullres, struc_elem, / gray)
   END 
   3: BEGIN ; close = performing dilation followed by erosion
      * info.image = morph_close(image, struc_elem, / gray)
      * info.fullres = morph_close(fullres, struc_elem, / gray)
   END 
   4: BEGIN ; tophat
      tmp = 255b - image ;; invert brightness
      tmp = morph_tophat(tmp, struc_elem)
      * info.th_struc_elem = struc_elem
      * info.image = 255b - BytScl(tmp, min = info.thmin, max = info.thmax)
      tmp = 255b - fullres ;; invert brightness
      tmp = morph_tophat(tmp, struc_elem)
      * info.fullres = 255b - BytScl(tmp, min = info.thmin, max = info.thmax)
   END 
   5: BEGIN ; Gradient
      * info.image = morph_gradient(image, struc_elem)
      * info.fullres = morph_gradient(fullres, struc_elem)
   END 
ENDCASE

;; update the image
WSet, info.drawID & tvscl, * info.image

skip:
;; return the info structure
Widget_Control, event.top, Set_UValue = info, / No_Copy
END                             
; of Xmorph_roi_event *********************************************





PRO Xmorph_roi, theImage, theFullres, $
                binary = binary, Group_Leader = group, block = block
;;**************************************************************************
;; Error Handling.
Catch, theError
IF theError NE 0 THEN BEGIN
   Catch, /Cancel
   ok = Error_Message( / Traceback);Dialog_Message(!Error_State.Msg)
;   IF destroy_groupleader THEN Widget_Control, groupleader, /Destroy
;   cancel = 1
ENDIF

On_Error, 1
Device, Decomposed = 0

;; Is image a pointer? If not, make it one.
IF Size(theImage, / TName) NE 'POINTER' THEN BEGIN
   image = Ptr_New(theImage) & fullres = Ptr_New(theFullres)
   mimage = Ptr_New(theImage) & mfullres = Ptr_New(theFullres)
   newPointer = 1
ENDIF ELSE BEGIN
   image = theImage & fullres = theFullres
   mimage = theImage & mfullres = theFullres
   newPointer = 0
ENDELSE

;; image size
s = Size( * image) & xsize = s(1) & ysize = s(2)

;; Create the tlb base widget./ Modal,
tlb = $
 Widget_Base(Title = 'Preview', / column, xOffset = 100, yOffset = 100)
menubase = widget_base(tlb, / row)
;; Create the rest of the widgets.
boxmorph = widget_base(menubase, / column, / frame)
lab = widget_label(boxmorph, value = 'Morph type')
morphtype = ['Dilate', 'Erode', 'Open', 'Close', 'Tophat', 'Gradient']
morphtype_ls = bindgen(6)
w_morphtype = $
 widget_droplist(boxmorph, value = morphtype, uvalue = 'mtype')


boxstruc = widget_base(menubase, / column, / frame)
lab = widget_label(boxstruc, value = 'Structure type and size')

ss = widget_base(boxstruc, / row)
strucType = ['Horizontal', 'Vertical', 'Diagonal down', 'Diagonal up', $
             'Circular']
strucType_ls = bindgen(5)
w_strucType = $
 widget_droplist(ss, value = strucType, uvalue = 'stype')

strucSize = strtrim(indgen(10) + 1, 2)
strucSize_ls = bindgen(10) + 1b
w_strucSize = $
 widget_droplist(ss, value = strucSize, uvalue = 'ssize')

dummy = $
 Widget_Button(menubase, Value = 'Test', uvalue = 'test')

dummy = $
 Widget_Button(menubase, Value = 'Reset', uvalue = 'reset')

dummy = $
 Widget_Button(menubase, Value = 'Quit', uvalue = 'quit', $
               Event_Pro = 'Xmorph_Roi_Done')
dummy = $
 Widget_Button(menubase, Value = 'Accept', uvalue = 'accept', $
               Event_Pro = 'Xmorph_Roi_Done')

w_thcontrast = widget_base(tlb, / row, / frame, sensitive = 0)
wthlab = widget_label(w_thcontrast,value = 'Set tophat contrast:')
w_thmin = widget_slider(w_thcontrast, minimum = 0, maximum = 254, $
                title = "Minimum", value = 30, uvalue = 'th_min')
w_thmax = widget_slider(w_thcontrast, minimum = 1, maximum = 255, $
                title = "Maximum", value = 60, uvalue = 'th_max')


w_draw = $
 Widget_Draw(tlb, XSize = s(1), YSize = s(2))
Widget_Control, tlb, / Realize


widget_control, w_draw, Get_Value = drawID
WSet, drawID & tvscl, * image
widget_control, w_thmin, get_value = thmin
widget_control, w_thmax, get_value = thmax


th_struc_elem = ptr_new(replicate(0, 3, 3))
info = {image:image, $
        mimage:mimage, $
        fullres:fullres, $
        mfullres:mfullres, $
        newPointer:newPointer, $ ; flag 2 indicate if we made a pointer or not.
        w_morphtype:w_morphtype, $
        morphtype_ls:morphtype_ls, $
        w_strucsize:w_strucsize, $
        strucsize_ls:strucsize_ls, $
        w_structype:w_structype, $
        structype_ls:structype_ls, $
        mtype_curr:0l, $
        stype_curr:0l, $
        ssize_curr:1l, $
        w_thcontrast:w_thcontrast, $
        w_thmin:w_thmin, $
        w_thmax:w_thmax, $
        thmin:thmin, $
        thmax:thmax, $
        th_struc_elem:th_struc_elem, $
        drawID:drawID $
       }                     

Widget_Control, tlb, Set_UValue=info, /No_Copy

XManager, 'xmorph_roi', tlb, Group = group, No_Block = 1 - Keyword_Set(block)
END
