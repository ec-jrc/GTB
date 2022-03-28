PRO Get_locset_CenterTLB, tlb

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
; of get_locset_centerTLB   *********************************************

PRO Get_locset_done, event
;; return the selected kernel to the main program
Widget_Control, event.top, Get_UValue = info, / No_Copy
;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue
IF strlowCase(eventValue) EQ 'accept' THEN BEGIN
   ;; Get the result and store it in the pointer location.
   * info.cancel = 0b ;set cancel to accept
ENDIF 
;; quit the application
Widget_Control, event.top, / Destroy
END
;  of get_locset_done   *********************************************

PRO Get_locset_Event, event
;; get the info structure
Widget_Control, event.top, Get_UValue = info, / No_Copy
;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue
CASE strlowCase(eventValue) OF
   'pix_x': BEGIN ;; get the selected x value
      widget_control, info.w_x, get_value = newx
      newx = long(newx[0])
      ;; ensure decent range for new value of x
      newx = 1 > newx < info.xdim          
      xstring = strtrim(newx,2)
      widget_control, info.w_x, set_value = xstring
      * info.selx = xstring
   END 
   'pix_y': BEGIN ;; get the selected y value
      widget_control, info.w_y, get_value = newy
      newy = long(newy[0])
      ;; ensure decent range for new value of y
      newy = 1 > newy < info.ydim
      ystring = strtrim(newy,2)
      widget_control, info.w_y, set_value = ystring
      * info.sely = ystring
   END
   'targ_val': BEGIN ;; get selected resistance value
      widget_control, info.w_z, get_value = newz
      newz = long(newz[0])
      if info.type eq 'Recode' then begin
        ;; ensure decent range for new value of z
        newz = 0 > newz < 255
        zstring = strtrim(newz,2) 
      endif else begin
        ;; ensure decent range for new value of z
        newz = 0 > newz < 100
        IF newz EQ 1 THEN newz = 0 ; reset invalid value
        ;; assign the string
        IF newz EQ 0 THEN BEGIN
          zstring = '0-Blocking'
        ENDIF ELSE IF newz EQ 2 THEN BEGIN
          zstring = '2-Foreground'
        ENDIF ELSE BEGIN
          zstring = strtrim(newz,2) + '-Background'
        ENDELSE
      endelse
      widget_control, info.w_z, set_value = zstring
      * info.selval = zstring
   END 
ENDCASE
Widget_Control, event.top, set_UValue = info, / No_Copy
END
;  of get_locset_event   *********************************************


;;**************************************************************************
PRO Get_locset, title = title, xdim = xdim, ydim = ydim, selx = selx, sely = sely, $
  selval = selval, Cancel = cancel, Group_Leader = groupleader
;; Return to caller if there is an error. Set the cancel
;; flag and destroy the group leader if it was created.
Catch, theError
IF theError NE 0 THEN BEGIN
   Catch, /Cancel
   ok = Dialog_Message(!Error_State.Msg)
   IF destroy_groupleader THEN Widget_Control, groupleader, /Destroy
   cancel = 1
ENDIF

;; Create modal base widget.
tlb = Widget_Base(Title = title, Column = 1,  / Base_Align_Center, / Modal, $
                 Group_Leader = groupleader)

;; Create the left side
lowerpart = widget_base(tlb, / row)
w_l = widget_base(lowerpart, / column, / frame)
w = widget_label(w_l, value = 'Insert values & press the ENTER key *** in each field *** to validate your settings.', / align_center,/sunken_frame)
w_1x = widget_base(w_l, / row, / frame)
w_l1 = widget_base(w_1x, / column)
button = widget_label(w_l1, value = 'Pixel: X', / align_center)
w_x = Widget_text(w_l1, Value = ['x'], UVALUE = 'pix_x',/editable)
w_l2 = widget_base(w_1x, / column)
button = widget_label(w_l2, value = 'Pixel: Y', / align_center)
w_y = Widget_text(w_l2, Value = ['y'], UVALUE = 'pix_y',/editable)
w_l3 = widget_base(w_1x, / column)
type = strmid(title, 0, 6) 
if type eq 'Recode' then ttit = 'Recode value' else ttit = 'Resistance value'
button = widget_label(w_l3, value = ttit, / align_center)
w_z = Widget_text(w_l3, Value = ['R'], UVALUE = 'targ_val',/editable)

;; right side
lrpart = widget_base(lowerpart, / column, / frame)
dummy = widget_label(lrpart, value = 'Options')
acceptID = $
 Widget_Button(lrpart, Value = 'Accept', uvalue = 'accept', $
              event_pro = 'get_locset_done')
cancelID = $
 Widget_Button(lrpart, Value = 'Cancel', uvalue = 'cancel', $
              event_pro = 'get_locset_done')

;; Realize and center the widgets on display.
get_locset_CenterTLB, tlb & Widget_Control, tlb, /Realize

;; Store the stuff we need:
info = {xdim:xdim, $ 
        ydim:ydim, $  
        w_x:w_x, $
        w_y:w_y, $
        w_z:w_z, $
        selx:selx, $
        sely:sely, $
        selval:selval, $
        type:type, $
        cancel:cancel $
       }  
Widget_Control, tlb, Set_UValue=info, /No_Copy

;; Blocking or modal widget, depending upon group leader.
XManager, 'get_locset', tlb, Group_Leader = group

END ;-----------------------------------------------------

