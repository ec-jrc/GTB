PRO get_MSPAparams_CenterTLB, tlb

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
; of get_MSPAparams_centerTLB   *********************************************


PRO get_MSPAparams_done, event
;; return the selected SE sizes to the main program
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
;  of get_MSPAparams_done   *********************************************



PRO get_MSPAparams_Event, event
;; get the info structure
Widget_Control, event.top, Get_UValue = info, / No_Copy

;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue

CASE strlowCase(eventValue) OF
   'default': BEGIN
      ;; set initial defaults
      widget_control, info.w_mspa1, set_combobox_select = 0 & * info.ms1 = '8'
      widget_control, info.w_mspa2, set_combobox_select = 1 & * info.ms2 = '1'
      widget_control, info.w_mspa3, set_combobox_select = 0 & * info.ms3 = 'on'
      widget_control, info.w_mspa4, set_combobox_select = 0 & * info.ms4 = 'on'
      widget_control, info.w_mspa5, set_combobox_select = 1 & * info.ms5 = 'no'
   END 
   'mspa1': BEGIN
       ;;* info.ms1 = event.index
      * info.ms1 = info.mspa1arr(event.index) 
   END 
   'mspa2': BEGIN
     if event.index le 0 then begin ;; user-specified value, correct for wrong input
       x = uint(abs(event.str)) & if x eq 0 then x = 1 & x = x < 100
       newsize = strtrim(x,2)
       widget_control, info.w_mspa2, set_value = [newsize,'1','2','3','4','5','6','7','8','9','10']
     endif else begin ; user-selected value
       widget_control, event.id, get_value = val
       newsize = val(event.index)
     endelse
     * info.ms2 = newsize
   END 
   'mspa3': BEGIN
       * info.ms3 = info.mspa3arr(event.index) 
   END 
   'mspa4': BEGIN
       * info.ms4 = info.mspa4arr(event.index) 
   END 
   'mspa5': BEGIN
       * info.ms5 = info.mspa5arr(event.index) 
   END 
ENDCASE

Widget_Control, event.top, set_UValue = info, / No_Copy
END
;  of get_MSPAparams_event   *********************************************


;;**************************************************************************
PRO Get_MSPAparams, $
 title = title, Cancel = cancel, msstats = msstats, Group_Leader = groupleader, $
 ms1 = ms1, ms2 = ms2, ms3 = ms3, ms4 = ms4, ms5 = ms5

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
 title = 'Select MSPA parameters'
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
 widget_label(llpart, value = 'Settings for MSPA parameters and statistics:')
llparttop = widget_base(llpart, / row)
;; mspa params
llparttop1 = widget_base(llparttop, / column, / frame)
button = $
  widget_label(llparttop1, value = 'FGconn', / sunken_frame, / align_center)
mspa1arr = ['8', '4'] ;; fgconn
w_mspa1 = $
 Widget_combobox(llparttop1, Value = mspa1arr, $
                 UValue = 'mspa1')

llparttop2 = widget_base(llparttop, / column, / frame)

sens = title NE 'MSPA Tiling'
button = $
  widget_label(llparttop2, value = 'EdgeWidth', / sunken_frame, / align_center)
mspa2arr = ['X', '1', '2', '3', '4', '5', '6', '7', '8', '9', '10']
w_mspa2 = $  ;; eew
 Widget_combobox(llparttop2, Value = mspa2arr, / editable, $
                 UValue = 'mspa2', sensitive = sens)

llparttop3 = widget_base(llparttop, / column, / frame)
button = $
  widget_label(llparttop3, value = 'Transition', / sunken_frame, / align_center)
mspa3arr = ['on', 'off'] ;; transition
w_mspa3 = $
 Widget_combobox(llparttop3, Value = mspa3arr, $
                 UValue = 'mspa3')

llparttop4 = widget_base(llparttop, / column, / frame)
button = $
  widget_label(llparttop4, value = 'Intext', / sunken_frame, / align_center)
mspa4arr = ['on', 'off'] ;; intext
w_mspa4 = $
 Widget_combobox(llparttop4, Value = mspa4arr, $
                 UValue = 'mspa4')

llparttop5 = widget_base(llparttop, / column, / frame)
button = $
  widget_label(llparttop5, value = 'Statistics', / sunken_frame, / align_center)
mspa5arr = ['yes', 'no'] ;; statistics
w_mspa5 = $
 Widget_combobox(llparttop5, Value = mspa5arr, $
                 UValue = 'mspa5')

lrpart = widget_base(lowerpart, / column, / frame)
dummy = widget_label(lrpart, value = 'Options')
;; default, cancel and accept
button = $
 widget_button(lrpart, Value = 'Default values', uvalue = 'default')
cancelID = $
 Widget_Button(lrpart, Value = 'Cancel', uvalue = 'cancel', $
              event_pro = 'get_MSPAparams_done')
acceptID = $
 Widget_Button(lrpart, Value = 'Accept', uvalue = 'accept', $
              event_pro = 'get_MSPAparams_done')


;; Center the widgets on display.
get_MSPAparams_CenterTLB, tlb
Widget_Control, tlb, /Realize

;; set initial defaults
widget_control, w_mspa1, set_combobox_select = 0
widget_control, w_mspa2, set_combobox_select = 1
widget_control, w_mspa3, set_combobox_select = 0
widget_control, w_mspa4, set_combobox_select = 0
widget_control, w_mspa5, set_combobox_select = 1, sensitive = keyword_set(msstats)


;; center the tlb
get_MSPAparams_CenterTLB, tlb

;; Store the program information:
info = {destroy_groupleader:destroy_groupleader, $
        cancel:cancel, $
        w_mspa1:w_mspa1, $
        w_mspa2:w_mspa2, $
        w_mspa3:w_mspa3, $
        w_mspa4:w_mspa4, $
        w_mspa5:w_mspa5, $
        mspa1arr:mspa1arr, $
        mspa3arr:mspa3arr, $
        mspa4arr:mspa4arr, $
        mspa5arr:mspa5arr, $
        ms1:ms1, $
        ms2:ms2, $
        ms3:ms3, $
        ms4:ms4, $
        ms5:ms5, $
        cancelID:cancelID, $
        acceptID:acceptID}  
Widget_Control, tlb, Set_UValue=info, /No_Copy

;; Blocking or modal widget, depending upon group leader.
XManager, 'get_MSPAparams', tlb, Group_Leader = group

END ;-----------------------------------------------------

