PRO Get_xrecode_CenterTLB, tlb

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
; of get_xrecode_centerTLB   *********************************************

PRO Get_xrecode_done, event
;; return the selected kernel to the main program
Widget_Control, event.top, Get_UValue = info
;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue
IF strlowCase(eventValue) EQ 'accept' THEN BEGIN
   ;; Get the result and store it in the pointer location.
   *info.cancel = 0b ;set cancel to accept
   ;; read out the final matrix
   widget_control, info.w_x, get_value=upv
   *info.seltab = upv
ENDIF 
;; quit the application
Widget_Control, event.top, / Destroy
END
;  of get_xrecode_done   *********************************************

PRO Get_xrecode_Event, event
;; get the info structure
Widget_Control, event.top, Get_UValue = info
;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue
CASE strlowCase(eventValue) OF
   'pix_val': BEGIN ;; get the selected pixel value
       ;; don't do anything here
   END 
   'restore': BEGIN ;; load existing recode table
     res = dialog_pickfile(Title = "Restore recode table 'GTBrecode_*.sav'", get_path = path2file, $
       default_extension = 'sav', / must_exist, filter = 'GTBrecode_*.sav', /fix_filter, /read)
     IF res EQ '' THEN GOTO, fin  ;; no name or 'cancel' selected
     ;; test to see if a .sav file was selected
     suffix = strmid(res,3,/reverse)
     IF suffix NE '.sav' THEN BEGIN
       msg = 'Wrong file selected. Please select a ' +string(10b) + $
         'GTB-generated recode table (GTBrecode_*.sav)'
       q = dialog_message(msg,/error)
       goto, fin
     ENDIF

     ;; restore the saved recode table in res providing the variable "upv"
     restore, res 
     ;; verify that upv exist in the sav-file
     if size(upv,/type) eq 0 then begin ;; upv does not exist
       msg = 'Invalid recode table detected. Please select a' + string(10b) + $
         'GTB-generated recode table (GTBrecode_*.sav)'
       q = dialog_message(msg,/error)
       goto, fin
     endif 
          
     ;; the current recode table is "upv_curr"
     upv_curr=info.upv
     
     ;; for all currently existing upvs transfer their restored values if available.
     rcol_curr=reform(upv_curr[1,*]) ;; the old class upv values of the currently displayed image
     rcol=reform(upv[1,*]) ;; the old class upv values of the restored image
     idx=0 & nr_curr=n_elements(rcol_curr)
     for i=0, nr_curr-1 do begin
       oldval=rcol_curr(i)
       q=where(rcol eq oldval,ct)
       if ct gt 0 then begin
         upv_curr[0,i]=upv[0,q]
         idx=idx+1
       endif
     endfor
     ;; the restored upv is applicable
     info.upv = upv_curr
     widget_control, info.w_x, set_value=upv_curr
     *info.seltab = upv_curr
     msg = 'Restored recodetable:' +string(10b) + res
     q = dialog_message(msg,/information)
   
     if idx ne nr_curr then begin
      msg='Class values in current and restored table were different.' + string(10b) + $
         'Only applicable class values were inserted.' + string(10b) + $
         'Please verify the new recode table.'
       res = dialog_message(msg, / information)
     endif
   END
   'save': BEGIN ;; save recode table
     res = dialog_pickfile(Title = "Save recode table to 'GTBrecode_*.sav'", file = 'GTBrecode_myrecode.sav', get_path = path2file, $
       / overwrite_prompt, / write, default_extension = 'sav', filter = ['GTBrecode_*.sav'], / fix_filter)
     IF res EQ '' THEN GOTO, fin  ;; no name or 'cancel' selected
     ;; verify the correct naming scheme
     fbn = file_basename(res) & prefix = strmid(fbn,0,10)
     IF prefix NE 'GTBrecode_' THEN res = file_dirname(res) + path_sep() + 'GTBrecode_' + fbn     
     widget_control, info.w_x, get_value=upv & *info.seltab = upv
     save, upv, filename = res
     msg = 'The recodetable was saved as:' +string(10b) + res
     q = dialog_message(msg,/information)
   END
   'set2': BEGIN ;; set all new values to the selected value
     widget_control, event.id, get_value = val
     newval = val(event.index)
     upv_curr = info.upv
     upv_curr[0,*] = newval   
     info.upv = upv_curr
     widget_control, info.w_x, set_value=upv_curr
     *info.seltab = upv_curr
   END
ENDCASE
Widget_Control, event.top, set_UValue = info 
fin:
END
;  of get_xrecode_event   *********************************************


;;**************************************************************************
PRO Get_xrecode, upv = upv, batch = batch, tit = tit, Cancel = cancel, seltab=seltab, Group_Leader = groupleader
;; Return to caller if there is an error. Set the cancel
;; flag and destroy the group leader if it was created.
Catch, theError
IF theError NE 0 THEN BEGIN
   Catch, /Cancel
   ok = Dialog_Message(!Error_State.Msg)
   IF destroy_groupleader THEN Widget_Control, groupleader, /Destroy
   cancel = 1
ENDIF

title = tit
;; Create modal base widget.
tlb = Widget_Base(Title = title, Column = 1,  / Base_Align_Center, / Modal, Group_Leader = groupleader)

;; Create the left side
lowerpart = widget_base(tlb, / row)
w_l = widget_base(lowerpart, / row, / frame)
w_l1 = widget_base(w_l, / column)

if batch eq 1 then begin
  ;; in batch mode upv must have 256 entries, define base table
  q0 = bytarr(256) & q1 = bindgen(256)
  upv = (transpose([[q0],[q1]]))
endif
edt=upv*0b & edt[0,*]=1b  ;; allow editing of first column only
qs = size(upv,/dim) & q = n_elements(qs)
if q eq 1 then nclass=1 else nclass=qs[1]
if nclass gt 20 then begin
  w_x = Widget_table(w_l1, Value = upv, UVALUE = 'pix_val', editable=edt, $
    /no_ROW_headers, column_labels=['New', 'Old'], scr_ysize=350)
endif else begin
  w_x = Widget_table(w_l1, Value = upv, UVALUE = 'pix_val', editable=edt, $
    /no_ROW_headers, column_labels=['New', 'Old'])
endelse

;; and right side
lrpart = widget_base(lowerpart, / column, / frame)
dummy = widget_label(lrpart, value = 'Options')
acceptID = $
 Widget_Button(lrpart, Value = 'Accept', uvalue = 'accept', $
              event_pro = 'get_xrecode_done')
cancelID = $
 Widget_Button(lrpart, Value = 'Cancel', uvalue = 'cancel', $
              event_pro = 'get_xrecode_done')

loadID = Widget_Button(lrpart, Value = 'Restore', uvalue = 'restore')
saveID = Widget_Button(lrpart, Value = 'Save', uvalue = 'save')

setall = widget_base(lrpart, / column, / frame)
button = widget_label(setall, value = 'Set all to:', / sunken_frame, / align_center)
set2arr = strtrim(indgen(256),2)
w_set2 = Widget_combobox(setall, Value = set2arr, UValue = 'set2')


;; Realize and center the widgets on display.
get_xrecode_CenterTLB, tlb & Widget_Control, tlb, /Realize

;; Store the program information:
info = {upv:upv, $ ;; uniq pixel values
        seltab:seltab, $
        w_x:w_x, $
        batch:batch, $
        w_set2:w_set2, $
        set2arr:set2arr, $
        cancel:cancel $
       }  
Widget_Control, tlb, Set_UValue=info

;; Blocking or modal widget, depending upon group leader.
XManager, 'get_xrecode', tlb, Group_Leader = group

END ;-----------------------------------------------------

