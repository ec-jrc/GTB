PRO Get_roires_CenterTLB, tlb

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
; of get_roires_centerTLB   *********************************************

PRO Get_roires_done, event
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
;  of get_roires_done   *********************************************

PRO Get_roires_Event, event
;; get the info structure
Widget_Control, event.top, Get_UValue = info, / No_Copy
;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue
CASE strlowCase(eventValue) OF
   'targ_val': BEGIN ;; get selected target layer
       * info.seltarg = info.target(event.index)
   END 
ENDCASE
Widget_Control, event.top, set_UValue = info, / No_Copy
END
;  of get_roires_event   *********************************************


;;**************************************************************************
PRO Get_roires, title=title, target = target, seltarg = seltarg,$
              Cancel = cancel, Group_Leader = groupleader
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
w_l = widget_base(lowerpart, / row, / frame)
w_l2 = widget_base(w_l, / column)
IF strpos(title,'esistance') GT 0 THEN zz = '    Resistance    ' ELSE zz = '     New Value    '
button = widget_label(w_l2, value = zz, /sunken_frame, / align_center)
w_y = Widget_droplist(w_l2, Value = target, UVALUE = 'targ_val')
;; right side
button = widget_label(lowerpart, value = ' ') ;; some space
lrpart = widget_base(lowerpart, / column, / frame)
dummy = widget_label(lrpart, value = '    Options    ', /sunken_frame)
acceptID = $
 Widget_Button(lrpart, Value = 'Accept', uvalue = 'accept', $
              event_pro = 'get_roires_done')
cancelID = $
 Widget_Button(lrpart, Value = 'Cancel', uvalue = 'cancel', $
              event_pro = 'get_roires_done')

;; Realize and center the widgets on display.
get_roires_CenterTLB, tlb & Widget_Control, tlb, /Realize

;; Store the program information:
info = {target:target, $  ;; target options
        seltarg:seltarg, $
        cancel:cancel $
       }  
Widget_Control, tlb, Set_UValue=info, /No_Copy

;; Blocking or modal widget, depending upon group leader.
XManager, 'get_roires', tlb, Group_Leader = group

END ;-----------------------------------------------------

