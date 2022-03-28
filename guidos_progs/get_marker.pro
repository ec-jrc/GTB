PRO Get_marker_CenterTLB, tlb

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
; of get_marker_centerTLB   *********************************************



PRO Get_marker_done, event
;; return the selected marker to the main program

Widget_Control, event.top, Get_UValue = info, / No_Copy

;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue

IF strlowCase(eventValue) EQ 'accept' THEN BEGIN
   ;; Get the marker and store it in the pointer location.
    widget_control, info.w_polpi, get_value = set_polpi
   * info.opolpi = set_polpi
   * info.cancel = 0b ;set cancel to accept
ENDIF 

;; quit the application
Widget_Control, event.top, / Destroy

;; perform cleanup
IF info.destroy_groupleader THEN Widget_Control, groupleader, /Destroy

END
;  of get_marker_done   *********************************************




PRO Get_marker_Event, event
;;; get the info structure
Widget_Control, event.top, Get_UValue = info, / No_Copy
;
;;; What kind of processing do you need?
;Widget_Control, event.id, Get_Uvalue = eventValue
;
;;widget_control, info.w_bin, get_value = binmask
;
;
;CASE strlowCase(eventValue) OF
;
;   'polpi': BEGIN
;      widget_control, info.w_polpi, get_value = set_polpi
;      IF set_polpi EQ 1b THEN BEGIN 
;         ;; set the marker to the default values using the smaller
;         ;; dimension of x/y
;         kdim = info.kdim_x < info.kdim_y
;         info.kdim_x = kdim & info.kdim_y = kdim
;         
;         ;; setup the marker      
;         themarker = replicate( - 1, kdim, kdim) 
;         themarker(kdim / 2, kdim / 2) = - (fix(total(themarker)) + 1)
;
;         ;; check for binary mask
;         IF binmask THEN BEGIN
;            q = where(themarker NE 0, ct)
;            IF ct GT 0 THEN themarker(q) = themarker(q) / themarker(q) 
;         ENDIF
;
;         ;; update the marker      
;         * info.marker = themarker
;
;         widget_control, info.w_markersize_x, set_combobox_select = ik(0)
;         widget_control, info.w_markersize_y, set_combobox_select = ik(0)
;
;         
;      ENDIF
;   END
;
;ENDCASE
;
Widget_Control, event.top, set_UValue = info, / No_Copy
END
;  of get_marker_event   *********************************************


;;**************************************************************************
PRO Get_marker,Cancel = cancel, opolpi = opolpi, Group_Leader=groupleader, wtitle = wtitle, costr = costr, proxlcp = proxlcp

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
IF keyword_set(wtitle) THEN wtitle = wtitle ELSE wtitle = 'Define marker object'

;; Provide a group leader if not supplied with one. This
;; is required for modal operation of widgets. Set a flag
;; for destroying the group leader widget before returning.
IF N_Elements(groupleader) EQ 0 THEN BEGIN
   groupleader = Widget_Base(Map = 0)
   Widget_Control, groupleader, / Realize
   destroy_groupleader = 1
ENDIF ELSE destroy_groupleader = 0


;; Create modal base widget.
tlb = Widget_Base(Title = strmid(wtitle,0,18), Column = 1,  / Base_Align_Center, / Modal, $
                  Group_Leader = groupleader)

;; Create the rest of the widgets.
;; bottom part widgets
lowerpart = widget_base(tlb, / row)
llpart = widget_base(lowerpart, / column, / frame)

;; position label
klabel = widget_label(llpart, value = strmid(wtitle,20))
klabel = widget_label(llpart, value = 'Location marked at x/y:')
klabel = widget_label(llpart, value = costr)
klabel = widget_label(llpart, value = '')
klabel = widget_label(llpart, value = 'D' + strmid(wtitle,strpos(wtitle,'define')+1) + ' as:',/align_left)
klabel = widget_label(llpart, value = '- Polygon surrounding this location',/align_left)
if proxlcp eq 0 then klabel = widget_label(llpart, value = '- Pixel point at this location',/align_left)
q = strmid(wtitle,strlen(wtitle)-1)
if q eq 'B' then klabel = widget_label(llpart, value = "or press 'Cancel' to skip setup of object B",/align_left)

lrpart = widget_base(lowerpart, / column, / frame)
;; set option for square marker
if proxlcp eq 0 then values = ['Polygon', 'Pixel'] else values = ['Polygon']
;w_polpi = cw_bgroup(lrpart, values, /COLUMN, /EXCLUSIVE, LABEL_TOP='Select:', /FRAME, uvalue = 'polpi')
w_polpi = cw_bgroup(lrpart, values, /COLUMN, /EXCLUSIVE, /FRAME)
;; cancel and accept
cancelID = Widget_Button(lrpart, Value = 'Cancel', uvalue = 'cancel', $
              event_pro = 'get_marker_done')
acceptID = Widget_Button(lrpart, Value = 'Accept', uvalue = 'accept', $
              event_pro = 'get_marker_done')


;; Center the widgets on display.
get_marker_CenterTLB, tlb
Widget_Control, tlb, /Realize

;; set initial defaults
widget_control, w_polpi, set_value = *opolpi ;polygon switch

;; center the tlb
Get_marker_CenterTLB, tlb

;; Store the program information:
info = {destroy_groupleader:destroy_groupleader, $
        w_polpi:w_polpi, $
        opolpi:opolpi, $
        cancel:cancel, $
        cancelID:cancelID, $
        acceptID:acceptID $
       }  
Widget_Control, tlb, Set_UValue=info, /No_Copy

;; Blocking or modal widget, depending upon group leader.
XManager, 'get_marker', tlb, Group_Leader = group

END ;-----------------------------------------------------
