PRO get_lm_CenterTLB, tlb

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
; of get_lm_centerTLB   *********************************************


PRO get_lm_done, event
;; return the selected kernel size to the main program
Widget_Control, event.top, Get_UValue = info, / No_Copy

;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue
IF strlowCase(eventValue) EQ 'accept' THEN BEGIN
  * info.pres = info.ms1
  * info.wdim = info.ms2
  * info.cancel = 0b   ;;set cancel to accept
ENDIF 

;; quit the application
Widget_Control, event.top, / Destroy

;; perform cleanup
IF info.destroy_groupleader THEN Widget_Control, groupleader, /Destroy

END
;  of get_lm_done   *********************************************



PRO get_lm_Event, event
;; get the info structure
Widget_Control, event.top, Get_UValue = info, / No_Copy

;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue

CASE strlowCase(eventValue) OF
   'default': BEGIN
      ;; set initial defaults
      widget_control, info.w_fos1, set_combobox_select = 4 & info.ms1 = info.fos1arr[4]
      widget_control, info.w_fos2, set_combobox_select = 7 & info.ms2 = info.fos2arr[7]
      pixres = float(info.ms1) & kdim = float(info.ms2)
      hec = ((pixres * kdim)^2) / 10000.0
      acr = hec * 2.47105
      widget_control, info.w_fos3, set_value = strtrim(hec,2)  
      widget_control, info.w_fos4, set_value = strtrim(acr,2)
   END 

   'fos1': BEGIN ;; pixel resolution
     if event.index le 0 then begin ;; user-specified value, correct for wrong input
       x = abs(event.str) & x = 0.1 > x < 1000.0
       ;; round to 2 digits
       x = round(x*100)/100.0 & x = strmid(x, 0, strpos(x,'.')+3)
       pixres = strtrim(x,2)
       widget_control, info.w_fos1, set_value = [pixres,'1', '5', '10', '25', '30', '100', '500', '1000'] 
     endif else begin ; user-selected value
       widget_control, event.id, get_value = val
       pixres = val(event.index)
     endelse
     ;;; * info.ms1 = info.fos1arr(event.index)
     info.ms1 = pixres

     ;; set info field for hectares and acres
     kdim = float(info.ms2)
     hec = ((pixres * kdim)^2) / 10000.0
     acr = hec * 2.47105
     widget_control, info.w_fos3, set_value = strtrim(hec,2)
     widget_control, info.w_fos4, set_value = strtrim(acr,2)    
   END 
   
   'fos2': BEGIN ;; kernel size
     if event.index le 0 then begin ;; user-specified value, correct for wrong input
       x = uint(abs(event.str)) 
       ;; ensure decent range for new value of x
       x = 3 > x < 501
       if (x mod 2) eq 0 then x=x+1
       kdim = strtrim(x,2)
       widget_control, info.w_fos2, set_value = [kdim,'3', '5', '7', '9', '11', '13', '27', '81', '243']
     endif else begin ; user-selected value
       widget_control, event.id, get_value = val
       kdim = val(event.index)
     endelse
     info.ms2 = kdim
     ;; set info field for hectares and acres
     pixres = float(info.ms1)
     hec = ((pixres * kdim)^2) / 10000.0
     acr = hec * 2.47105
     widget_control, info.w_fos3, set_value = strtrim(hec,2)
     widget_control, info.w_fos4, set_value = strtrim(acr,2)
   END 
ENDCASE

Widget_Control, event.top, set_UValue = info, / No_Copy
END
;  of get_lm_event   *********************************************


;;**************************************************************************
PRO get_lm, pres = pres, wdim = wdim, gdal = gdal, title = title, Cancel = cancel, Group_Leader = groupleader

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
IF keyword_set(title) THEN title = title ELSE title = 'Select observation scale'
IF keyword_set(gdal) THEN gdalstr = gdal ELSE gdalstr = 'gdalinfo: no details on the pixel resolution.'

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
dummy = widget_label(llpart, value = 'Define settings for Observation Scale:',/align_left)
llparttop = widget_base(llpart, / row)

;; pixres
llparttop1 = widget_base(llparttop, / column, / frame)
button = widget_label(llparttop1, value = 'PixelRes [m]', / sunken_frame, / align_center)
fos1arr = ['X', '1', '5', '10', '25', '30', '100', '500', '1000'] ;; pixel resolution
w_fos1 = Widget_combobox(llparttop1, Value = fos1arr, / editable, UValue = 'fos1')

;; winsize
llparttop2 = widget_base(llparttop, / column, / frame)
button = widget_label(llparttop2, value = 'WinSize', / sunken_frame, / align_center)
fos2arr = ['X', '3', '5', '7', '9', '11', '13', '27', '81', '243']
w_fos2 = Widget_combobox(llparttop2, Value = fos2arr, / editable, UValue = 'fos2')

llparttop3 = widget_base(llparttop, / column, / frame)
button = widget_label(llparttop3, value = 'Observation Scale [hectares, acres]', / sunken_frame, / align_center)
llparttop31 = widget_base(llparttop3, / row)
fos3arr = 'hectares' ;; area
w_fos3 = Widget_text(llparttop31, xsize=20, Value = fos3arr, /frame)
fos4arr = 'acres'
w_fos4 = Widget_text(llparttop31, xsize=20, Value = fos4arr, /frame)

w_gdal = widget_label(llpart, value = gdalstr,/align_left)


lrpart = widget_base(lowerpart, / column, / frame)
dummy = widget_label(lrpart, value = 'Options')
;; default, cancel and accept
button = $
 widget_button(lrpart, Value = 'Default values', uvalue = 'default')
cancelID = $
 Widget_Button(lrpart, Value = 'Cancel', uvalue = 'cancel', $
              event_pro = 'get_lm_done')
acceptID = $
 Widget_Button(lrpart, Value = 'Accept', uvalue = 'accept', $
              event_pro = 'get_lm_done')


;; Center the widgets on display.
get_lm_CenterTLB, tlb
Widget_Control, tlb, /Realize

;; set initial defaults
widget_control, w_fos1, set_combobox_select = 4 & ms1 = fos1arr[4]
widget_control, w_fos2, set_combobox_select = 7 & ms2 = fos2arr[7]
pixres = float(ms1) & kdim = float(ms2)
hec = ((pixres * kdim)^2) / 10000.0
acr = hec * 2.47105
widget_control, w_fos3, set_value = strtrim(hec,2)
widget_control, w_fos4, set_value = strtrim(acr,2)

;; center the tlb
get_lm_CenterTLB, tlb

;; Store the program information:
info = {destroy_groupleader:destroy_groupleader, $
        pres:pres, $
        wdim:wdim, $
        cancel:cancel, $
        w_fos1:w_fos1, $
        w_fos2:w_fos2, $
        w_fos3:w_fos3, $
        w_fos4:w_fos4, $
        fos1arr:fos1arr, $
        fos2arr:fos2arr, $
        fos3arr:fos3arr, $
        fos4arr:fos4arr, $
        ms1:ms1, $
        ms2:ms2, $
        cancelID:cancelID, $
        acceptID:acceptID}  
Widget_Control, tlb, Set_UValue=info, /No_Copy

;; Blocking or modal widget, depending upon group leader.
XManager, 'get_lm', tlb, Group_Leader = group

END ;-----------------------------------------------------

