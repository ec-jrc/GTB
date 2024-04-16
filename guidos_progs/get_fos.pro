PRO get_fos_CenterTLB, tlb

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
; of get_fos_centerTLB   *********************************************


PRO get_fos_done, event
;; return the selected kernel size to the main program
Widget_Control, event.top, Get_UValue = info, / No_Copy

;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue
IF strlowCase(eventValue) EQ 'accept' THEN BEGIN
  * info.graythresh = info.ms19
  * info.fmethod = info.ms20a
  * info.frep = info.ms20
  * info.conn = info.ms21
  * info.pres = info.ms1
  * info.wdim = info.ms2
  * info.cancel = 0b   ;;set cancel to accept
;  ptr_free, info.fos20arr
ENDIF 

;; quit the application
Widget_Control, event.top, / Destroy

;; perform cleanup
IF info.destroy_groupleader THEN Widget_Control, groupleader, /Destroy

END
;  of get_fos_done   *********************************************



PRO get_fos_Event, event
;; get the info structure
Widget_Control, event.top, Get_UValue = info, / No_Copy

;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue

CASE strlowCase(eventValue) OF
   'default': BEGIN
      ;; set initial defaults
      if info.gray eq 1b then widget_control, info.w_fos19, set_combobox_select = 3 & info.ms19 = info.fos19arr[3] ; 30%
      widget_control, info.w_fos20a, set_combobox_select = 0 & info.ms20a = info.methodarr[0]
      widget_control, info.w_fos20, set_combobox_select = 0 & info.ms20 = info.reportarr[0]
      widget_control, info.w_fos21, set_combobox_select = 0 & info.ms21 = info.fos21arr[0]
      widget_control, info.w_fos1, set_combobox_select = 4 & info.ms1 = info.fos1arr[4]
      widget_control, info.w_fos2, set_combobox_select = 3 & info.ms2 = info.fos2arr[3]
      pixres = float(info.ms1) & kdim = float(info.ms2)
      hec = ((pixres * kdim)^2) / 10000.0
      acr = hec * 2.47105
      widget_control, info.w_fos3, set_value = strtrim(hec,2)  
      widget_control, info.w_fos4, set_value = strtrim(acr,2)
   END 

   'fos19': BEGIN
     if event.index le 0 then begin ;; user-specified value, correct for wrong input
       x = uint(abs(event.str)) & if x eq 0 then x = 1 & x = x < 100
       newsize = strtrim(x,2)
       widget_control, info.w_fos19, set_value = [newsize,'10','20','30','40']
     endif else begin ; user-selected value
       widget_control, event.id, get_value = val
       newsize = val(event.index)
     endelse
     info.ms19 = newsize       
   END 

   'method': BEGIN
     widget_control, event.id, get_value = val
     ftype = val(event.index)
     info.ms20a = ftype
   END

   'reporting': BEGIN
     widget_control, event.id, get_value = val
     ftype = val(event.index)
     info.ms20 = ftype
   END

   'fos21': BEGIN
     widget_control, event.id, get_value = val
     conn = val(event.index)
     info.ms21 = conn
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
;  of get_fos_event   *********************************************


;;**************************************************************************
PRO Get_fos, inpgray = inpgray, graythresh = graythresh, fmethod = fmethod, frep = frep, conn = conn, $
  pres = pres, wdim = wdim, gdal = gdal, title = title, Cancel = cancel, Group_Leader = groupleader

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
gray = *inpgray EQ 1b
IF gray eq 0b THEN tt = 'Input: BINARY' ELSE tt = 'Input: GRAYSCALE    '
dummy = widget_label(llpart, value = tt +  '           Settings for Fixed Observation Scale (FOS):',/align_left)
llparttop = widget_base(llpart, / row)
;; FOS params
;; gray threshold
fos19arr = ['X', '10', '20', '30', '40']
w_fos19 = 0 & ms19 = 0
IF gray THEN BEGIN
  llparttop19 = widget_base(llparttop, / column, / frame)
  button = widget_label(llparttop19, value = 'Gray Threshold', / sunken_frame, / align_center)
  w_fos19 = Widget_combobox(llparttop19, Value = fos19arr, / editable, UValue = 'fos19', sensitive = gray)
ENDIF

;; FOS method
llparttop20a = widget_base(llparttop, / column, / frame)
button = widget_label(llparttop20a, value = 'Method', / sunken_frame, / align_center)
methodarr_binary = ['FAD', 'FED', 'FAC']
methodarr_gray = ['FAD', 'FED']
IF gray EQ 0 THEN methodarr = methodarr_binary ELSE methodarr = methodarr_gray
w_fos20a = Widget_combobox(llparttop20a, Value = methodarr, UValue = 'method')

;; FOS reporting style
llparttop20 = widget_base(llparttop, / column, / frame)
button = widget_label(llparttop20, value = 'Reporting', / sunken_frame, / align_center)
reportarr = ['5class', '6class', 'APP_2class', 'APP_5class']
w_fos20 = Widget_combobox(llparttop20, Value = reportarr, UValue = 'reporting')

;; FG-conn
llparttop21 = widget_base(llparttop, / column, / frame)
button = widget_label(llparttop21, value = 'FG-conn', / sunken_frame, / align_center)
fos21arr = ['8', '4']
w_fos21 = Widget_combobox(llparttop21, Value = fos21arr, UValue = 'fos21')

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
button = widget_label(llparttop3, value = 'Observation scale [hectares, acres]', / sunken_frame, / align_center)
llparttop31 = widget_base(llparttop3, / row)
fos3arr = 'hectares' ;; area
w_fos3 = Widget_text(llparttop31, xsize=20, Value = fos3arr, /frame)
fos4arr = 'acres'
w_fos4 = Widget_text(llparttop31, xsize=20, Value = fos4arr, /frame)

w_gdal = widget_text(llpart, value = gdalstr)

lrpart = widget_base(lowerpart, / column, / frame)
dummy = widget_label(lrpart, value = 'Options')
;; default, cancel and accept
button = $
 widget_button(lrpart, Value = 'Default values', uvalue = 'default')
cancelID = $
 Widget_Button(lrpart, Value = 'Cancel', uvalue = 'cancel', $
              event_pro = 'get_fos_done')
acceptID = $
 Widget_Button(lrpart, Value = 'Accept', uvalue = 'accept', $
              event_pro = 'get_fos_done')


;; Center the widgets on display.
get_fos_CenterTLB, tlb
Widget_Control, tlb, /Realize

;; set initial defaults
IF gray THEN BEGIN
  widget_control, w_fos19, set_combobox_select = 3 & ms19 = fos19arr[3]
ENDIF
widget_control, w_fos20a, set_combobox_select = 0 & ms20a = methodarr[0]
widget_control, w_fos20, set_combobox_select = 0 & ms20 = reportarr[0]
widget_control, w_fos21, set_combobox_select = 0 & ms21 = fos21arr[0]
widget_control, w_fos1, set_combobox_select = 4 & ms1 = fos1arr[4]
widget_control, w_fos2, set_combobox_select = 3 & ms2 = fos2arr[3]
pixres = float(ms1) & kdim = float(ms2)
hec = ((pixres * kdim)^2) / 10000.0
acr = hec * 2.47105
widget_control, w_fos3, set_value = strtrim(hec,2)
widget_control, w_fos4, set_value = strtrim(acr,2)

;; center the tlb
get_fos_CenterTLB, tlb

;; Store the program information:
info = {destroy_groupleader:destroy_groupleader, $
        inpgray:inpgray, $
        graythresh:graythresh, $
        gray:gray, $
        fmethod:fmethod, $
        frep:frep, $
        conn:conn, $
        pres:pres, $
        wdim:wdim, $
        cancel:cancel, $
        w_fos19:w_fos19, $
        w_fos20a:w_fos20a, $
        w_fos20:w_fos20, $
        w_fos21:w_fos21, $
        w_fos1:w_fos1, $
        w_fos2:w_fos2, $
        w_fos3:w_fos3, $
        w_fos4:w_fos4, $
        fos19arr:fos19arr, $
        methodarr:methodarr, $
        reportarr:reportarr, $
        fos21arr:fos21arr, $
        fos1arr:fos1arr, $
        fos2arr:fos2arr, $
        fos3arr:fos3arr, $
        fos4arr:fos4arr, $
        ms1:ms1, $
        ms2:ms2, $
        ms19:ms19, $
        ms20a:ms20a, $
        ms20:ms20, $
        ms21:ms21, $
        cancelID:cancelID, $
        acceptID:acceptID}  
Widget_Control, tlb, Set_UValue=info, /No_Copy

;; Blocking or modal widget, depending upon group leader.
XManager, 'get_fos', tlb, Group_Leader = group

END ;-----------------------------------------------------

