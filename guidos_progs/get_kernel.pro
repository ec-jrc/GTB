PRO Get_kernel_CenterTLB, tlb

;; This utility routine centers the TLB.
Device, Get_Screen_Size = screenSize
xCenter = screenSize(0) / 2
yCenter = screenSize(1) / 2

geom = Widget_Info(tlb, / Geometry)
xHalfSize = geom.Scr_XSize / 2
yHalfSize = geom.Scr_YSize / 2

Widget_Control, tlb, XOffset = xCenter - xHalfSize, $
 YOffset = yCenter - yHalfSize

END 
; of get_kernel_centerTLB   *********************************************



PRO Get_kernel_done, event
;; return the selected kernel to the main program

Widget_Control, event.top, Get_UValue = info, / No_Copy

;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue

IF strlowCase(eventValue) EQ 'accept' THEN BEGIN
   ;; Get the kernel and store it in the pointer location.
   Widget_Control, info.tableID, Get_Value = theKernel
   * info.selected_kernel = theKernel
   * info.cancel = 0b ;set cancel to accept
ENDIF 

;; quit the application
Widget_Control, event.top, / Destroy

;; perform cleanup
Ptr_Free, info.kernel
IF info.destroy_groupleader THEN Widget_Control, groupleader, /Destroy

END
;  of get_kernel_done   *********************************************




PRO Get_kernel_Event, event
;; get the info structure
Widget_Control, event.top, Get_UValue = info, / No_Copy

;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue

widget_control, info.w_bin, get_value = binmask


CASE strlowCase(eventValue) OF

   'default': BEGIN
      ;; set the default kernel
      theKernel = * info.selected_kernel & kdim = (size(theKernel))[1]
      ;; update the kernel table, etc
      * info.kernel = theKernel
      widget_control, info.tableID, set_value = theKernel, alignment = 1
      widget_control, info.tableID, set_table_view = [0, 0], $
       table_xsize = kdim, table_ysize = kdim

      ksizeString = ['X', '3', '5', '7', '9', '11', '13', '27', '81', '243']
      widget_control, info.w_kernelsize_x, set_value = ksizestring
      widget_control, info.w_kernelsize_y, set_value = ksizestring
       
      widget_control, info.w_kernelsize_x, set_combobox_select = 1
      widget_control, info.w_kernelsize_y, set_combobox_select = 1
      widget_control, info.w_bin, set_value = info.bin_image
      widget_control, info.w_sqk, set_value = 1b
   END 


   'sqk': BEGIN
      widget_control, info.w_sqk, get_value = set_sq
      IF set_sq EQ 1b THEN BEGIN 
         ;; set the kernel to the default values using the smaller
         ;; dimension of x/y
         kdim = info.kdim_x < info.kdim_y
         info.kdim_x = kdim & info.kdim_y = kdim
         
         ;; setup the kernel      
         theKernel = replicate( - 1, kdim, kdim) 
         theKernel(kdim / 2, kdim / 2) = - (fix(total(theKernel)) + 1)

         ;; check for binary mask
         IF binmask THEN BEGIN
            q = where(theKernel NE 0, ct)
            IF ct GT 0 THEN theKernel(q) = theKernel(q) / theKernel(q) 
         ENDIF

         ;; update the kernel      
         * info.kernel = theKernel

         ;; update the kernel table
         widget_control, info.tableID, set_value = theKernel, alignment = 1
         ;; see beginning of table
         widget_control, info.tableID, set_table_view = [0, 0], $
          table_xsize = kdim, table_ysize = kdim
          
         ik = where(kdim EQ info.ksize)
         widget_control, info.w_kernelsize_x, set_combobox_select = ik(0)
         widget_control, info.w_kernelsize_y, set_combobox_select = ik(0)

         
      ENDIF
   END

   'bin': BEGIN
      IF binmask THEN BEGIN ; set to binary
         widget_control, info.tableID, get_value = theKernel
         q = where(theKernel NE 0, ct)
         IF ct GT 0 THEN BEGIN 
            theKernel(q) = theKernel(q) / theKernel(q) 
         ENDIF 
      ENDIF ELSE BEGIN ; set to normal
         theKernel = replicate( - 1, info.kdim_x, info.kdim_y) 
         theKernel(info.kdim_x / 2, info.kdim_y / 2) = $
          - (fix(total(theKernel)) + 1)
      ENDELSE
      * info.kernel = theKernel
      ;; update the kernel table
      widget_control, info.tableID, set_value = theKernel, alignment = 1
   END  


   'ksize_x': BEGIN
      ;; get new kernel dimension
      if event.index le 0 then begin ;; user-specified value
        if event.str eq 'X' then x=3 else x = fix(abs(event.str))
        ;; ensure decent range for new value of x
        x = 3 > x < 501
        if (x mod 2) eq 0 then x=x+1      
        kdim_x = x  & info.kdim_x = kdim_x
        info.ksize = [kdim_x,info.ksize[1:*]]
        ksizestring = strtrim(info.ksize,2)
        widget_control, info.w_kernelsize_x, set_value = ksizestring
        widget_control, info.w_kernelsize_x, set_combobox_select = 0
      endif else begin
        kdim_x = info.ksize(event.index) & info.kdim_x = kdim_x
      endelse
      
      ;; check for square kernel
      widget_control, info.w_sqk, get_value = set_sq
      IF set_sq EQ 1b THEN BEGIN
         info.kdim_y = kdim_x
         ik = where(kdim_x EQ info.ksize)
         widget_control, info.w_kernelsize_y, set_value = ksizestring
         widget_control, info.w_kernelsize_y, set_combobox_select = ik(0)
      ENDIF

      ;; setup the kernel      
      theKernel = replicate( - 1, kdim_x, info.kdim_y) 
      theKernel(kdim_x / 2, info.kdim_y / 2) = - (fix(total(theKernel)) + 1)

      ;; check for binary mask
      IF binmask THEN BEGIN
         q = where(theKernel NE 0, ct)
         IF ct GT 0 THEN theKernel(q) = theKernel(q) / theKernel(q) 
      ENDIF

      ;; get the old kernel dimension and add/delete columns/rows
      widget_control, info.tableID, get_value = oldKernel
      kdim_old_x = (size(oldKernel))[1] & kdim_old_y = (size(oldKernel))[2]

      new_dim_x = kdim_x - kdim_old_x
      IF new_dim_x GT 0 THEN BEGIN 
         widget_control, info.tableID, insert_columns = new_dim_x
      IF set_sq EQ 1b THEN $
       widget_control, info.tableID, insert_rows = new_dim_x
         widget_control, info.tableID, set_value = theKernel, alignment = 1
      ENDIF ELSE BEGIN
         ;; update the kernel table
         widget_control, info.tableID, set_value = theKernel, alignment = 1
         ;; see beginning of table
         widget_control, info.tableID, set_table_view = [0, 0], $
          table_xsize = kdim_x, table_ysize = info.kdim_y
      ENDELSE

      ;; update the kernel      
      * info.kernel = theKernel

   END 


   'ksize_y': BEGIN
     ;; get new kernel dimension
     if event.index le 0 then begin ;; user-specified value
       if event.str eq 'X' then x=3 else x = fix(abs(event.str))
       ;; ensure decent range for new value of x
       x = 3 > x < 501
       if (x mod 2) eq 0 then x=x+1
       kdim_y = x  & info.kdim_y = kdim_y
       info.ksize = [kdim_y,info.ksize[1:*]]
       ksizestring = strtrim(info.ksize,2)
       widget_control, info.w_kernelsize_y, set_value = ksizestring
       widget_control, info.w_kernelsize_y, set_combobox_select = 0
     endif else begin
       kdim_y = info.ksize(event.index) & info.kdim_y = kdim_y
     endelse

      ;; check for square kernel
      widget_control, info.w_sqk, get_value = set_sq
      IF set_sq EQ 1b THEN BEGIN
         info.kdim_x = kdim_y
         ik = where(kdim_y EQ info.ksize)
         widget_control, info.w_kernelsize_x, set_value = ksizestring
         widget_control, info.w_kernelsize_x, set_combobox_select = ik(0)
      ENDIF

      ;; setup the kernel      
      theKernel = replicate( - 1, info.kdim_x, kdim_y) 
      theKernel(info.kdim_x / 2, kdim_y / 2) = - (fix(total(theKernel)) + 1)

      ;; check for binary mask
      IF binmask THEN BEGIN
         q = where(theKernel NE 0, ct)
         IF ct GT 0 THEN theKernel(q) = theKernel(q) / theKernel(q) 
      ENDIF

      ;; get the old kernel dimension and add/delete columns/rows
      widget_control, info.tableID, get_value = oldKernel
      kdim_old_x = (size(oldKernel))[1] & kdim_old_y = (size(oldKernel))[2]

      new_dim_y = kdim_y - kdim_old_y
      IF new_dim_y GT 0 THEN BEGIN 
         IF set_sq EQ 1b THEN $
          widget_control, info.tableID, insert_columns = new_dim_y
         widget_control, info.tableID, insert_rows = new_dim_y
         widget_control, info.tableID, set_value = theKernel, alignment = 1
      ENDIF ELSE BEGIN
         ;; update the kernel table
         widget_control, info.tableID, set_value = theKernel, alignment = 1
         ;; see beginning of table
         widget_control, info.tableID, set_table_view = [0, 0], $
          table_xsize = info.kdim_x, table_ysize = kdim_y
      ENDELSE

      ;; update the kernel      
      * info.kernel = theKernel
   END 



   ;; edit the kernel
   'kernel': BEGIN
      ;; get the new settings and update
      widget_control, info.tableID, get_value = theKernel

      ;; check for binary mask
      IF binmask THEN BEGIN
         q = where(theKernel NE 0, ct)
         IF ct GT 0 THEN theKernel(q) = theKernel(q) / theKernel(q) 
      ENDIF

      ;; update the kernel      
      * info.kernel = theKernel
   END
ENDCASE

Widget_Control, event.top, set_UValue = info, / No_Copy
END
;  of get_kernel_event   *********************************************


;;**************************************************************************
PRO Get_kernel, selected_kernel = selected_kernel, Cancel = cancel, $
   Group_Leader=groupleader, binary = binary, square = square, $
   noedit = noedit, morph = morph, title = title

;; Return to caller if there is an error. Set the cancel
;; flag and destroy the group leader if it was created.
Catch, theError
IF theError NE 0 THEN BEGIN
   Catch, /Cancel
   ok = Dialog_Message(!Error_State.Msg)
   IF destroy_groupleader THEN Widget_Control, groupleader, /Destroy
   cancel = 1
ENDIF

;; Check parameters and keywords.
IF keyword_set(title) THEN title = title ELSE title = 'Define kernel'
label = ""
IF N_Elements(xsize) EQ 0 THEN xsize = 200
bin_image = keyword_set(binary)

;; Provide a group leader if not supplied with one. This
;; is required for modal operation of widgets. Set a flag
;; for destroying the group leader widget before returning.
IF N_Elements(groupleader) EQ 0 THEN BEGIN
   groupleader = Widget_Base(Map = 0)
   Widget_Control, groupleader, / Realize
   destroy_groupleader = 1
ENDIF ELSE destroy_groupleader = 0


;; Create modal base widget.
tlb = Widget_Base(Title = title, Column = 1,  / Base_Align_Center, / Modal, $
                 Group_Leader = groupleader)

;; Create the rest of the widgets.
;; bottom part widgets
lowerpart = widget_base(tlb, / row)
llpart = widget_base(lowerpart, / column, / frame)
llparttop = widget_base(llpart, / row)
;; kernel size
kernelsize_tit = widget_label(llparttop, value = 'Kernel size')
ksizeString = ['X', '3', '5', '7', '9', '11', '13', '27', '81', '243']
ksize = [0, 3, 5, 7, 9, 11, 13, 27, 81, 243]
IF keyword_set(morph) THEN BEGIN
  ksizeString = strtrim(indgen(11 * 2) + 1, 2)
  ksize = fix(ksizeString)
ENDIF
w_kernelsize_x = $
 Widget_combobox(llparttop, Value = ksizeString, $
                 UValue = 'ksize_x',/editable)
kernelsize_tit = widget_label(llparttop, value = ' columns   X')
w_kernelsize_y = $
 Widget_combobox(llparttop, Value = ksizeString, $
                 UValue = 'ksize_y',/editable)
kernelsize_tit = widget_label(llparttop, value = ' lines')

;; kernel label
IF keyword_set(noedit) THEN ttt='Fixed kernel parameters:' ELSE ttt='Editable kernel parameters:'
klabel = widget_label(llpart, value = ttt)
;; kernel settings, set initial kernel to the one from the input
kernel = * selected_kernel
s = size(kernel) & kdim_x = s(1) & kdim_y = s(2) 
ik = where(kdim_x EQ ksize)
;kernel = replicate( - 1, kdim_x, kdim_y) 
;kernel(kdim_x / 2, kdim_y / 2) = - (fix(total(kernel)) + 1)
IF keyword_set(noedit) THEN editable=0 else editable=1
tableID = $
 Widget_table(llpart, Editable=editable, / no_headers, alignment = 1, $
             value = kernel, uvalue = 'kernel', scr_xsize = 620, scr_ysize = 185)
             ;;xsize = kdim_x, ysize = kdim_y  xsize = 9, ysize = 9


lrpart = widget_base(lowerpart, / column, / frame)
;; set option for square kernel
dummy = widget_label(lrpart, value = 'Options')
w_sqk = $
 cw_bgroup(lrpart, 'Square kernel', / nonexclusive, uvalue = 'sqk')
w_bin = $
 cw_bgroup(lrpart, 'Binary kernel', / nonexclusive, uvalue = 'bin')
;; default, cancel and accept
button = $
 widget_button(lrpart, Value = 'Default kernel', uvalue = 'default')
cancelID = $
 Widget_Button(lrpart, Value = 'Cancel', uvalue = 'cancel', $
              event_pro = 'get_kernel_done')
acceptID = $
 Widget_Button(lrpart, Value = 'Accept', uvalue = 'accept', $
              event_pro = 'get_kernel_done')


;; Center the widgets on display.
get_kernel_CenterTLB, tlb
Widget_Control, tlb, /Realize

;; set initial defaults
widget_control, w_kernelsize_x, set_combobox_select = ik(0)
widget_control, w_kernelsize_y, set_combobox_select = ik(0)
widget_control, w_sqk, set_value = 1b ; set square kernel
widget_control, w_bin, set_value = bin_image ; binary kernel
IF keyword_set(binary) THEN widget_control, w_bin, sensitive = 0
IF keyword_set(square) THEN widget_control, w_sqk, sensitive = 0

;; center the tlb
Get_kernel_CenterTLB, tlb

;; Store the program information:
info = {destroy_groupleader:destroy_groupleader, $
        selected_kernel:selected_kernel, $ ;default input kernel
        w_bin:w_bin, $
        bin_image:bin_image, $       ; switch to set binary mask
        cancel:cancel, $
        w_sqk:w_sqk, $
        w_kernelsize_x:w_kernelsize_x, $
        w_kernelsize_y:w_kernelsize_y, $
        kdim_x:kdim_x, $        ; actual kernel x-dimension
        kdim_y:kdim_y, $  ; actual kernel y-dimension
        tableID:tableID, $
        cancelID:cancelID, $
        acceptID:acceptID, $
        kernel:Ptr_New(kernel), $ ; kernel values
        ksize:ksize $           ; possible kernel dimensions
       }  
Widget_Control, tlb, Set_UValue=info, /No_Copy

;; Blocking or modal widget, depending upon group leader.
XManager, 'get_kernel', tlb, Group_Leader = group

END ;-----------------------------------------------------

