PRO get_CIparams_CenterTLB, tlb

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
; of get_CIparams_centerTLB   *********************************************


PRO get_CIparams_done, event
;; return the selected options to the main program
Widget_Control, event.top, Get_UValue = info, / No_Copy

;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue
IF strlowCase(eventValue) EQ 'accept' THEN BEGIN
   * info.cancel = 0b   ;;set cancel to accept
ENDIF 

;; quit the application
Widget_Control, event.top, / Destroy

;; perform cleanup
IF info.destroy_groupleader THEN Widget_Control, groupleader, /Destroy

END
;  of get_CIparams_done   *********************************************



PRO get_CIparams_Event, event
;; get the info structure
Widget_Control, event.top, Get_UValue = info, / No_Copy

;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue

CASE strlowCase(eventValue) OF
   'default': BEGIN
      ;; set initial defaults
      widget_control, info.w_CI1, set_combobox_select = 0 & * info.CI1 = '8'
      widget_control, info.w_CI2, set_combobox_select = 0 & * info.CI2 = 'Edge2Edge'
   END 
   'conef1': BEGIN
      * info.CI1 = info.CI1arr(event.index) 
   END  
   'conef2': BEGIN
       * info.CI2 = info.CI2arr(event.index) 
   END 

ENDCASE

Widget_Control, event.top, set_UValue = info, / No_Copy
END
;  of get_CIparams_event   *********************************************


;;**************************************************************************
PRO Get_CIparams, $
 title = title, Cancel = cancel, Group_Leader = groupleader, CI1 = CI1, CI2 = CI2

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
IF keyword_set(title) THEN title = title ELSE $
 title = 'Select Conefor Inputs parameters'
label = ""
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
dummy = $
 widget_label(llpart, value = 'Conefor Inputs Settings:')
llparttop = widget_base(llpart, / row)
;; CI params
llparttop1 = widget_base(llparttop, / column, / frame)
button = $
  widget_label(llparttop1, value = 'FGconn', / sunken_frame, / align_center)
CI1arr = ['8', '4'] ;; fgconn
w_CI1 = $
 Widget_combobox(llparttop1, Value = CI1arr, $
                 UValue = 'conef1')

llparttop2 = widget_base(llparttop, / column, / frame)
button = $
  widget_label(llparttop2, value = 'Distance', / sunken_frame, / align_center)
CI2arr = ['Edge2Edge', 'Centroid2Centroid'] ;; distance
w_CI2 = $
 Widget_combobox(llparttop2, Value = CI2arr, $
                 UValue = 'conef2')

lrpart = widget_base(lowerpart, / column, / frame)
dummy = widget_label(lrpart, value = 'Options')
;; default, cancel and accept
button = $
 widget_button(lrpart, Value = 'Default values', uvalue = 'default')
cancelID = $
 Widget_Button(lrpart, Value = 'Cancel', uvalue = 'cancel', $
              event_pro = 'get_CIparams_done')
acceptID = $
 Widget_Button(lrpart, Value = 'Accept', uvalue = 'accept', $
              event_pro = 'get_CIparams_done')


;; Center the widgets on display.
get_CIparams_CenterTLB, tlb
Widget_Control, tlb, /Realize

;; set initial defaults
widget_control, w_CI1, set_combobox_select = 0
widget_control, w_CI2, set_combobox_select = 0

;; center the tlb
get_CIparams_CenterTLB, tlb

;; Store the program information:
info = {destroy_groupleader:destroy_groupleader, $
        cancel:cancel, $
        w_CI1:w_CI1, $
        w_CI2:w_CI2, $
        CI1arr:CI1arr, $
        CI2arr:CI2arr, $
        CI1:CI1, $
        CI2:CI2, $
        cancelID:cancelID, $
        acceptID:acceptID}  
Widget_Control, tlb, Set_UValue=info, /No_Copy

;; Blocking or modal widget, depending upon group leader.
XManager, 'get_CIparams', tlb, Group_Leader = group

END ;-----------------------------------------------------

