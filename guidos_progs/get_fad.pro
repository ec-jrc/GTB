PRO get_fad_CenterTLB, tlb

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
; of get_fad_centerTLB   *********************************************


PRO get_fad_done, event
;; return the selected kernel size to the main program
Widget_Control, event.top, Get_UValue = info, / No_Copy

;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue
IF strlowCase(eventValue) EQ 'accept' THEN BEGIN
  * info.ftype = info.ms20
  * info.conn = info.ms21
  * info.cancel = 0b   ;;set cancel to accept
ENDIF 

;; quit the application
Widget_Control, event.top, / Destroy

;; perform cleanup
IF info.destroy_groupleader THEN Widget_Control, groupleader, /Destroy

END
;  of get_fad_done   *********************************************



PRO get_fad_Event, event
;; get the info structure
Widget_Control, event.top, Get_UValue = info, / No_Copy

;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue

CASE strlowCase(eventValue) OF
   'default': BEGIN
      ;; set initial defaults
      widget_control, info.w_fad20, set_combobox_select = 0 & info.ms20 = info.fad20arr[0]
      widget_control, info.w_fad21, set_combobox_select = 0 & info.ms21 = info.fad21arr[0]
   END 

   'fad20': BEGIN
      widget_control, event.id, get_value = val
      ftype = val(event.index)
      info.ms20 = ftype
   END 

   'fad21': BEGIN
     widget_control, event.id, get_value = val
     conn = val(event.index)
     info.ms21 = conn
   END

ENDCASE

Widget_Control, event.top, set_UValue = info, / No_Copy
END
;  of get_fad_event   *********************************************


;;**************************************************************************
PRO Get_fad, ftype = ftype, conn = conn, title = title, Cancel = cancel, Group_Leader = groupleader

 ;; Return to caller if there is an error. Set the cancel
 ;; flag and destroy the group leader if it was created.
 Catch, theError
IF theError NE 0 THEN BEGIN
   Catch, / Cancel
   ok = Dialog_Message(!Error_State.Msg)
   IF destroy_groupleader THEN Widget_Control, groupleader, /Destroy
   cancel = 1
ENDIF

;; Check parameters and keywords.
IF keyword_set(title) THEN title = title ELSE title = 'Select FAD type'
IF N_Elements(xsize) EQ 0 THEN xsize = 200

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
dummy = widget_label(llpart, value = 'Settings for FAD multi-scale analysis:')
llparttop = widget_base(llpart, / row)
;; fadtype
llparttop20 = widget_base(llparttop, / column, / frame)
button = widget_label(llparttop20, value = 'FAD Type', / sunken_frame, / align_center)
fad20arr = ['FAD 6-class', 'FAD-APP 2-class', 'FAD-APP 5-class']
w_fad20 = Widget_combobox(llparttop20, Value = fad20arr, UValue = 'fad20')

;; FG-conn
llparttop21 = widget_base(llparttop, / column, / frame)
button = widget_label(llparttop21, value = 'FG-conn', / sunken_frame, / align_center)
fad21arr = ['8', '4']
w_fad21 = Widget_combobox(llparttop21, Value = fad21arr, UValue = 'fad21')

lrpart = widget_base(lowerpart, / column, / frame)
dummy = widget_label(lrpart, value = 'Options')
;; default, cancel and accept
button = $
 widget_button(lrpart, Value = 'Default values', uvalue = 'default')
cancelID = $
 Widget_Button(lrpart, Value = 'Cancel', uvalue = 'cancel', $
              event_pro = 'get_fad_done')
acceptID = $
 Widget_Button(lrpart, Value = 'Accept', uvalue = 'accept', $
              event_pro = 'get_fad_done')


;; Center the widgets on display.
get_fad_CenterTLB, tlb
Widget_Control, tlb, /Realize

;; set initial defaults
widget_control, w_fad20, set_combobox_select = 0 & ms20 = fad20arr[0]
widget_control, w_fad21, set_combobox_select = 0 & ms21 = fad21arr[0]

;; center the tlb
get_fad_CenterTLB, tlb

;; Store the program information:
info = {destroy_groupleader:destroy_groupleader, $
        ftype:ftype, $
        conn:conn, $
        cancel:cancel, $
        w_fad20:w_fad20, $
        w_fad21:w_fad21, $
        fad20arr:fad20arr, $
        fad21arr:fad21arr, $
        ms20:ms20, $
        ms21:ms21, $
        cancelID:cancelID, $
        acceptID:acceptID}  
Widget_Control, tlb, Set_UValue=info, /No_Copy

;; Blocking or modal widget, depending upon group leader.
XManager, 'get_fad', tlb, Group_Leader = group

END ;-----------------------------------------------------

