PRO Get_xset_CenterTLB, tlb

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
; of get_xset_centerTLB   *********************************************

PRO Get_xset_done, event
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
;  of get_xset_done   *********************************************

PRO Get_xset_Event, event
;; get the info structure
Widget_Control, event.top, Get_UValue = info, / No_Copy
;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue
CASE strlowCase(eventValue) OF
   'pix_val': BEGIN ;; get the selected pixel value
       * info.selpix = info.upv(event.index) 
   END 
   'targ_val': BEGIN ;; get selected target layer
       * info.seltarg = info.target(event.index)
   END 
ENDCASE
Widget_Control, event.top, set_UValue = info, / No_Copy
END
;  of get_xset_event   *********************************************


;;**************************************************************************
PRO Get_xset, upv = upv, target = target, selpix = selpix, seltarg = seltarg,$
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

title = "Map pixel -> target"
;; Create modal base widget.
tlb = Widget_Base(Title = title, Column = 1,  / Base_Align_Center, / Modal, $
                 Group_Leader = groupleader)

;; Create the left side
lowerpart = widget_base(tlb, / row)
w_l = widget_base(lowerpart, / row, / frame)
w_l1 = widget_base(w_l, / column)
button = widget_label(w_l1, value = 'Pixel value', / align_center)
w_x = Widget_droplist(w_l1, Value = upv, UVALUE = 'pix_val')
w_l2 = widget_base(w_l, / column)
button = widget_label(w_l2, value = 'Target layer', / align_center)
w_y = Widget_droplist(w_l2, Value = target, UVALUE = 'targ_val')
;; right side
lrpart = widget_base(lowerpart, / column, / frame)
dummy = widget_label(lrpart, value = 'Options')
acceptID = $
 Widget_Button(lrpart, Value = 'Accept', uvalue = 'accept', $
              event_pro = 'get_xset_done')
cancelID = $
 Widget_Button(lrpart, Value = 'Cancel', uvalue = 'cancel', $
              event_pro = 'get_xset_done')

;; Realize and center the widgets on display.
get_xset_CenterTLB, tlb & Widget_Control, tlb, /Realize

;; Store the program information:
info = {upv:upv, $ ;; uniq pixel values
        target:target, $  ;; target options
        selpix:selpix, $
        seltarg:seltarg, $
        cancel:cancel $
       }  
Widget_Control, tlb, Set_UValue=info, /No_Copy

;; Blocking or modal widget, depending upon group leader.
XManager, 'get_xset', tlb, Group_Leader = group

END ;-----------------------------------------------------

