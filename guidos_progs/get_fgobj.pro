PRO get_fgobj_CenterTLB, tlb

;; This utility routine centers the TLB.
Device, Get_Screen_Size = screenSize
xCenter = screenSize(0) / 2
yCenter = screenSize(1) / 2

geom = Widget_info(tlb, / Geometry)
xHalfSize = geom.Scr_XSize / 2
yHalfSize = geom.Scr_YSize / 2

Widget_Control, tlb, XOffset = xCenter - xHalfSize, $
 YOffset = yCenter - yHalfSize

END 
; of get_fgobj_centerTLB   *********************************************


PRO get_fgobj_done, event
;; return the selected kernel size to the main program
Widget_Control, event.top, Get_UValue = info, / No_Copy

;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue
IF strlowCase(eventValue) EQ 'accept' THEN BEGIN
  * info.o_fgconn = info.ms8  ;; return the selected FGconn
  * info.o_pixres = info.ms0  ;; return the selected pixel resolution
  * info.o_big3pink = info.ms9  ;; return the selected big3pink option
  * info.o_cl1 = info.ms1  ;; return the selected class 1 area threshold
  * info.o_cl2 = info.ms2  ;; return the selected class 2 area threshold
  * info.o_cl3 = info.ms3  ;; return the selected class 3 area threshold
  * info.o_cl4 = info.ms4  ;; return the selected class 4 area threshold
  * info.o_cl5 = info.ms5  ;; return the selected class 5 area threshold
  * info.cancel = 0b   ;;set cancel to accept
  * info.o_saveopt = info.saveopt
ENDIF 

;; quit the application
Widget_Control, event.top, / Destroy

;; perform cleanup
IF info.destroy_groupleader THEN Widget_Control, groupleader, /Destroy

END
;  of get_fgobj_done   *********************************************



PRO get_fgobj_Event, event
;; get the info structure
Widget_Control, event.top, Get_UValue = info

;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue

CASE strlowCase(eventValue) OF
    'setsave': BEGIN
      if event.value eq 0 then info.saveopt = 0 else info.saveopt = 1
    end
    
    'default': BEGIN
      info.fgconn_arr = ['8', '4']
      widget_control, info.w_fgconn, set_combobox_select = 0 & info.ms8 = info.fgconn_arr[0]
      info.pixres_arr = ['X', '1', '5', '10', '25', '30', '100', '500', '1000']
      widget_control, info.w_pixres, set_combobox_select = 4 & info.ms0 = info.pixres_arr[4] ;; pixres     
      info.big3pink_arr = ['0', '1']
      widget_control, info.w_big3pink, set_combobox_select = 0 & info.ms9 = info.big3pink_arr[0]
      
      info.classes = info.scaled_cl_arr
      widget_control, info.w_cl1, set_combobox_select = 1, set_value = info.classes & info.ms1 = info.classes[1]
      widget_control, info.w_cl2, set_combobox_select = 2, set_value = info.classes & info.ms2 = info.classes[2]
      widget_control, info.w_cl3, set_combobox_select = 3, set_value = info.classes & info.ms3 = info.classes[3]
      widget_control, info.w_cl4, set_combobox_select = 4, set_value = info.classes & info.ms4 = info.classes[4]
      widget_control, info.w_cl5, set_combobox_select = 5, set_value = info.classes & info.ms5 = info.classes[5]
      widget_control, info.w_cl6, set_combobox_select = 5, set_value = info.classes

      if info.oba_min gt info.ms1 then begin
        info.ms1 = strtrim(info.oba_min,2)
        widget_control, info.w_cl1, set_combobox_select = 0, set_value = [strtrim(info.ms1,2), info.classes[1:*]]
      endif
      if info.oba_min gt info.ms2 then begin
        info.ms2 = strtrim(info.oba_min,2)
        widget_control, info.w_cl2, set_combobox_select = 0, set_value = [strtrim(info.ms2,2), info.classes[1:*]]
      endif
      if info.oba_min gt info.ms3 then begin
        info.ms3 = strtrim(info.oba_min,2)
        widget_control, info.w_cl3, set_combobox_select = 0, set_value = [strtrim(info.ms3,2), info.classes[1:*]]
      endif
      if info.oba_min gt info.ms4 then begin
        info.ms4 = strtrim(info.oba_min,2)
        widget_control, info.w_cl4, set_combobox_select = 0, set_value = [strtrim(info.ms4,2), info.classes[1:*]]
      endif
      if info.oba_min gt info.ms5 then begin
        info.ms5 = strtrim(info.oba_min,2) 
        widget_control, info.w_cl5, set_combobox_select = 0, set_value = [strtrim(info.ms5,2), info.classes[1:*]]
        widget_control, info.w_cl6, set_combobox_select = 0, set_value = [strtrim(info.ms5,2), info.classes[1:*]]
      endif     
    END
    
    'save': BEGIN ;; save accouting table
      res = dialog_pickfile(Title = "Save accounting table to 'GTBacc.sav'", file = 'GTBacc.sav', get_path = path2file, $
        / overwrite_prompt, / write, default_extension = 'sav',  / fix_filter) 
      IF res EQ '' THEN GOTO, fin  ;; no name or 'cancel' selected
      ms8=info.ms8 & ms0=info.ms0 & ms9=info.ms9 & ms1=info.ms1 & ms2=info.ms2 & ms3=info.ms3 & ms4=info.ms4 & ms5=info.ms5
      save,filename=res,ms8,ms0,ms9,ms1,ms2,ms3,ms4,ms5
    END
    
   'restore': BEGIN ;; load existing accounting table
     res = dialog_pickfile(Title = "Restore accounting table 'GTBacc.sav'", get_path = path2file, $
       default_extension = 'sav', / must_exist, filter = 'GTBacc*.sav', /fix_filter, /read)
     IF res EQ '' THEN GOTO, fin  ;; no name or 'cancel' selected
     ;; restore the saved accounting table in res providing the variables ms8, ms0, ms9, ms1-5
     restore, res 
     info.ms8 = ms8 & widget_control, info.w_fgconn, set_combobox_select = ms8 eq '4'
     info.ms0 = ms0 & widget_control, info.w_pixres, set_value = [ms0,'1', '5', '10', '25', '30', '100', '500', '1000'] 
     info.ms9 = ms9 & widget_control, info.w_big3pink, set_combobox_select = ms9 eq '1'
     info.ms1 = ms1 & widget_control, info.w_cl1, set_combobox_select = 0, set_value = [strtrim(info.ms1,2), info.classes[1:*]]
     info.ms2 = ms2 & widget_control, info.w_cl2, set_combobox_select = 0, set_value = [strtrim(info.ms2,2), info.classes[1:*]]
     info.ms3 = ms3 & widget_control, info.w_cl3, set_combobox_select = 0, set_value = [strtrim(info.ms3,2), info.classes[1:*]]
     info.ms4 = ms4 & widget_control, info.w_cl4, set_combobox_select = 0, set_value = [strtrim(info.ms4,2), info.classes[1:*]]
     info.ms5 = ms5 & widget_control, info.w_cl5, set_combobox_select = 0, set_value = [strtrim(info.ms5,2), info.classes[1:*]]
     widget_control, info.w_cl6, set_combobox_select = 0, set_value = [strtrim(info.ms5,2), info.classes[1:*]]
   END

   'fgconn': BEGIN
     widget_control, event.id, get_value = val
     selval = val(event.index)
     info.ms8 = selval
     goto, skip ;; all is done already
   END
  
   'pixres': BEGIN
     if event.index le 0 then begin ;; user-specified value, correct for wrong input
       x = abs(event.str) & x = 0.1 > x < 1000.0
       ;; round to 2 digits
       x = round(x*100)/100.0 & x = strmid(x, 0, strpos(x,'.')+3)
       selval = strtrim(x,2)
       widget_control, info.w_pixres, set_value = [selval,'1', '5', '10', '25', '30', '100', '500', '1000'] ;; pixres_arr
     endif else begin ; user-selected value
       widget_control, event.id, get_value = val
       selval = val(event.index)
     endelse
     info.ms0 = selval     
   END 
      
   'big3pink': BEGIN
     widget_control, event.id, get_value = val
     selval = val(event.index)
     info.ms9 = selval
     goto, skip ;; all is done already
   END

   'cl1': BEGIN
     if event.index le 0 then begin ;; user-specified value, correct for wrong input
       x = ulong64(abs(event.str))
       ;; ensure decent range for new value of x
       x = 1 > x < 1000000000000
       selval = strtrim(x,2)
       widget_control, info.w_cl1, set_combobox_select = 0, set_value = [selval,info.classes[1:*]]
     endif else begin ; user-selected value
       widget_control, event.id, get_value = val
       selval = val(event.index)
     endelse
     info.ms1 = selval
     ;; amend the classes accordingly: cl1 was selected
     x = ulong64(selval)
     info.ms2 = info.ms2 > x
     info.ms3 = info.ms3 > ulong64(info.ms2)
     info.ms4 = info.ms4 > ulong64(info.ms3)
     info.ms5 = info.ms5 > ulong64(info.ms4)
     widget_control, info.w_cl2, set_combobox_select = 0, set_value = [strtrim(info.ms2,2), info.classes[1:*]]
     widget_control, info.w_cl3, set_combobox_select = 0, set_value = [strtrim(info.ms3,2), info.classes[1:*]]
     widget_control, info.w_cl4, set_combobox_select = 0, set_value = [strtrim(info.ms4,2), info.classes[1:*]]
     widget_control, info.w_cl5, set_combobox_select = 0, set_value = [strtrim(info.ms5,2), info.classes[1:*]]
     widget_control, info.w_cl6, set_combobox_select = 0, set_value = [strtrim(info.ms5,2), info.classes[1:*]]
   END

   'cl2': BEGIN
     if event.index le 0 then begin ;; user-specified value, correct for wrong input
       x = ulong64(abs(event.str))
       ;; ensure decent range for new value of x
       x = 1 > x < 1000000000000
       selval = strtrim(x,2)
       widget_control, info.w_cl2, set_combobox_select = 0, set_value = [selval,info.classes[1:*]]
     endif else begin ; user-selected value
       widget_control, event.id, get_value = val
       selval = val(event.index)
     endelse
     info.ms2 = selval
     ;; amend the classes accordingly: cl1 was selected
     x = ulong64(selval)
     ;;info.ms1 = ((info.ms1 < x) - 1) > 1  ;; must be smaller than X but at least 1
     info.ms1 = (info.ms1 < x) > 1  ;; must be smaller than X but at least 1
     info.ms3 = info.ms3 > ulong64(info.ms2)
     info.ms4 = info.ms4 > ulong64(info.ms3)
     info.ms5 = info.ms5 > ulong64(info.ms4)
     widget_control, info.w_cl1, set_combobox_select = 0, set_value = [strtrim(info.ms1,2), info.classes[1:*]]
     widget_control, info.w_cl3, set_combobox_select = 0, set_value = [strtrim(info.ms3,2), info.classes[1:*]]
     widget_control, info.w_cl4, set_combobox_select = 0, set_value = [strtrim(info.ms4,2), info.classes[1:*]]
     widget_control, info.w_cl5, set_combobox_select = 0, set_value = [strtrim(info.ms5,2), info.classes[1:*]]
     widget_control, info.w_cl6, set_combobox_select = 0, set_value = [strtrim(info.ms5,2), info.classes[1:*]]
   END

   'cl3': BEGIN
     if event.index le 0 then begin ;; user-specified value, correct for wrong input
       x = ulong64(abs(event.str))
       ;; ensure decent range for new value of x
       x = 1 > x < 1000000000000
       selval = strtrim(x,2)
       widget_control, info.w_cl3, set_combobox_select = 0, set_value = [selval, info.classes[1:*]]
     endif else begin ; user-selected value
       widget_control, event.id, get_value = val
       selval = val(event.index)
     endelse
     info.ms3 = selval
     ;; amend the classes accordingly: cl1 was selected
     x = ulong64(selval)
     info.ms2 = (info.ms2 < x) > 1 ;; must be smaller than X but at least 1
     info.ms1 = (info.ms1 < ulong64(info.ms2)) > 1 ;; must be smaller than ms2 but at least 1
     info.ms4 = info.ms4 > ulong64(info.ms3)
     info.ms5 = info.ms5 > ulong64(info.ms4)
     widget_control, info.w_cl1, set_combobox_select = 0, set_value = [strtrim(info.ms1,2), info.classes[1:*]]
     widget_control, info.w_cl2, set_combobox_select = 0, set_value = [strtrim(info.ms2,2), info.classes[1:*]]
     widget_control, info.w_cl4, set_combobox_select = 0, set_value = [strtrim(info.ms4,2), info.classes[1:*]]
     widget_control, info.w_cl5, set_combobox_select = 0, set_value = [strtrim(info.ms5,2), info.classes[1:*]]
     widget_control, info.w_cl6, set_combobox_select = 0, set_value = [strtrim(info.ms5,2), info.classes[1:*]]
   END

   'cl4': BEGIN
     if event.index le 0 then begin ;; user-specified value, correct for wrong input
       x = ulong64(abs(event.str))
       ;; ensure decent range for new value of x
       x = 1 > x < 1000000000000
       selval = strtrim(x,2)
       widget_control, info.w_cl4, set_combobox_select = 0, set_value = [selval, info.classes[1:*]]
     endif else begin ; user-selected value
       widget_control, event.id, get_value = val
       selval = val(event.index)
     endelse
     info.ms4 = selval
     ;; amend the classes accordingly: cl1 was selected
     x = ulong64(selval)
     info.ms3 = (info.ms3 < x) > 1
     info.ms2 = (info.ms2 < ulong64(info.ms3)) > 1 ;; must be smaller than X but at least 1
     info.ms1 = (info.ms1 < ulong64(info.ms2)) > 1 ;; must be smaller than ms2 but at least 1
     info.ms5 = info.ms5 > ulong64(info.ms4)
     widget_control, info.w_cl1, set_combobox_select = 0, set_value = [strtrim(info.ms1,2), info.classes[1:*]]
     widget_control, info.w_cl2, set_combobox_select = 0, set_value = [strtrim(info.ms2,2), info.classes[1:*]]
     widget_control, info.w_cl3, set_combobox_select = 0, set_value = [strtrim(info.ms3,2), info.classes[1:*]]
     widget_control, info.w_cl5, set_combobox_select = 0, set_value = [strtrim(info.ms5,2), info.classes[1:*]]
     widget_control, info.w_cl6, set_combobox_select = 0, set_value = [strtrim(info.ms5,2), info.classes[1:*]]
   END

   'cl5': BEGIN
     if event.index le 0 then begin ;; user-specified value, correct for wrong input
       x = ulong64(abs(event.str))
       ;; ensure decent range for new value of x
       x = 1 > x < 1000000000000
       selval = strtrim(x,2)
       widget_control, info.w_cl5, set_combobox_select = 0, set_value = [selval, info.classes[1:*]]
     endif else begin ; user-selected value
       widget_control, event.id, get_value = val
       selval = val(event.index)
     endelse
     info.ms5 = selval
     ;; amend the classes accordingly: cl1 was selected
     x = ulong64(selval)
     info.ms4 = (info.ms4 < x) > 1
     info.ms3 = (info.ms3 < ulong64(info.ms4)) > 1
     info.ms2 = (info.ms2 < ulong64(info.ms3)) > 1 ;; must be smaller than X but at least 1
     info.ms1 = (info.ms1 < ulong64(info.ms2)) > 1 ;; must be smaller than ms2 but at least 1
     widget_control, info.w_cl1, set_combobox_select = 0, set_value = [strtrim(info.ms1,2), info.classes[1:*]]
     widget_control, info.w_cl2, set_combobox_select = 0, set_value = [strtrim(info.ms2,2), info.classes[1:*]]
     widget_control, info.w_cl3, set_combobox_select = 0, set_value = [strtrim(info.ms3,2), info.classes[1:*]]
     widget_control, info.w_cl4, set_combobox_select = 0, set_value = [strtrim(info.ms4,2), info.classes[1:*]]
     widget_control, info.w_cl6, set_combobox_select = 0, set_value = [selval,info.classes[1:*]]
   END
ENDCASE

;; apply new settings


;; set info field for hectares and acres
hec = ((info.ms0)^2) / 10000.0
acr = hec * 2.47105
;; the 5 classes in hectares and acres
widget_control, info.w_hec1, set_value = '->  ' + strtrim(hec * info.ms1,2) + ' ha'
widget_control, info.w_hec2, set_value = '->  ' + strtrim(hec * info.ms2,2) + ' ha'
widget_control, info.w_hec3, set_value = '->  ' + strtrim(hec * info.ms3,2) + ' ha'
widget_control, info.w_hec4, set_value = '->  ' + strtrim(hec * info.ms4,2) + ' ha'
widget_control, info.w_hec5, set_value = '->  ' + strtrim(hec * info.ms5,2) + ' ha'
widget_control, info.w_hec6, set_value = strtrim(hec * info.ms5,2) + ' ha ->'
widget_control, info.w_acr1, set_value = '->  ' + strtrim(acr * info.ms1,2) + ' ac'
widget_control, info.w_acr2, set_value = '->  ' + strtrim(acr * info.ms2,2) + ' ac'
widget_control, info.w_acr3, set_value = '->  ' + strtrim(acr * info.ms3,2) + ' ac'
widget_control, info.w_acr4, set_value = '->  ' + strtrim(acr * info.ms4,2) + ' ac'
widget_control, info.w_acr5, set_value = '->  ' + strtrim(acr * info.ms5,2) + ' ac'
widget_control, info.w_acr6, set_value = strtrim(acr * info.ms5,2) + ' ac ->' 
skip:
Widget_Control, event.top, set_UValue = info, / No_Copy
fin:
END
;  of get_fgobj_event   *********************************************


;;**************************************************************************
PRO Get_fgobj, o_saveopt = o_saveopt, o_fgconn = o_fgconn, o_pixres = o_pixres, o_big3pink = o_big3pink, scaled_cl_arr = scaled_cl_arr, oba_min = oba_min, $   
  o_cl1 = o_cl1, o_cl2 = o_cl2, o_cl3 = o_cl3, o_cl4 = o_cl4, o_cl5 = o_cl5, $  
  gdal = gdal, title = title, Cancel = cancel, saveopt = saveopt, Group_Leader = groupleader

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
;; temporay, this stuff is set in main
;cancel = ptr_new(1b)
;o_fgconn = ptr_new(1b) ;; fgconn
;o_pixres = ptr_new(25) ;; pixel resolution
;o_big3pink = ptr_new(0b) ;; big3pink
;o_cl1 = ptr_new(5)
;o_cl2 = ptr_new(5)
;o_cl3 = ptr_new(5)
;o_cl4 = ptr_new(5)
;o_cl5 = ptr_new(5)
;cl_arr = ['X', '10', '50', '100', '1000', '5000']
;scaled_cl_arr = ['X', '560', '5600', '112000', '280000', '504000']
;oba_min = 150000
;;=====================================


IF keyword_set(title) THEN title = title ELSE title = 'Accounting'
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
;; Define arrays
;; FG connectivity
fgconn_arr = ['8', '4']
;; pixel resolution
pixres_arr = ['X', '1', '5', '10', '25', '30', '100', '500', '1000']
;; big3pink
big3pink_arr = ['0', '1']
;; area class arrary
classes = scaled_cl_arr
;; Create the rest of the widgets.
;; bottom part widgets
lowerpart = widget_base(tlb, / row)
llpart = widget_base(lowerpart, / column, / frame)
dummy = widget_label(llpart, value = 'Please define the Base Settings and the Area Thresholds in pixels for up to 6 Area Classes.', /align_left)
row0 = widget_base(llpart, / row, /frame)
button = widget_label(row0, value = 'Base Settings:  ', / sunken_frame, / align_center)

rowtl1 = widget_base(row0, /row, /frame)
button = widget_label(rowtl1, value = 'Foreground Connectivity:', / sunken_frame, / align_center)
w_fgconn = Widget_combobox(rowtl1, Value = fgconn_arr, UValue = 'fgconn')

rowtl2 = widget_base(row0, /row, /frame)
button = widget_label(rowtl2, value = 'Pixel Resolution [m]', / sunken_frame, / align_center)
w_pixres = Widget_combobox(rowtl2, Value = pixres_arr, / editable,/dynamic_resize, UValue = 'pixres')

rowtl3 = widget_base(row0, /row, /frame)
button = widget_label(rowtl3, value = 'Show Big3 in pink color', / sunken_frame, / align_center)
w_big3pink = Widget_combobox(rowtl3, Value = big3pink_arr, UValue = 'big3pink')


row1 = widget_base(llpart, / row, /frame)
;; column 0: pixres [m]
col_0 = widget_base(row1, / column, / frame)
dummy = widget_label(col_0, value = 'Classes:', / sunken_frame)
dummy = widget_label(col_0, value = '', ysize=5)
dummy = widget_label(col_0, value = 'Pixels:')
dummy = widget_label(col_0, value = '', ysize=9)
dummy = widget_label(col_0, value = 'Hectares:')
dummy = widget_label(col_0, value = '', ysize=6)
dummy = widget_label(col_0, value = 'Acres:')

;; column 1: class1
col_1 = widget_base(row1, / column, / frame)
button = widget_label(col_1, value = 'Class 1:', / sunken_frame, / align_center)
col_1r = widget_base(col_1,/row)
button = widget_label(col_1r, value = '1 -> ')
w_cl1 = Widget_combobox(col_1r, Value = classes, / editable, UValue = 'cl1',/dynamic_resize)
button = widget_label(col_1r, value = ' pixels')
w_hec1 = Widget_text(col_1, Value = '10')
w_acr1 = Widget_text(col_1, Value = '10')

;; column 2: class2
col_2 = widget_base(row1, / column, / frame)
button = widget_label(col_2, value = 'Class 2:', / sunken_frame, / align_center)
col_2r = widget_base(col_2,/row)
button = widget_label(col_2r, value = '-> ')
w_cl2 = Widget_combobox(col_2r, Value = classes, / editable, UValue = 'cl2',/dynamic_resize)
button = widget_label(col_2r, value = ' pixels')
w_hec2 = Widget_text(col_2, Value = '10')
w_acr2 = Widget_text(col_2, Value = '10')

;; column 3: class3
col_3 = widget_base(row1, / column, / frame)
button = widget_label(col_3, value = 'Class 3:', / sunken_frame, / align_center)
col_3r = widget_base(col_3,/row)
button = widget_label(col_3r, value = '-> ')
w_cl3 = Widget_combobox(col_3r, Value = classes, / editable, UValue = 'cl3',/dynamic_resize)
button = widget_label(col_3r, value = ' pixels')
w_hec3 = Widget_text(col_3, Value = '10')
w_acr3 = Widget_text(col_3, Value = '10')

;; column 4: class4
col_4 = widget_base(row1, / column, / frame)
button = widget_label(col_4, value = 'Class 4:', / sunken_frame, / align_center)
col_4r = widget_base(col_4,/row)
button = widget_label(col_4r, value = '-> ')
w_cl4 = Widget_combobox(col_4r, Value = classes, / editable, UValue = 'cl4',/dynamic_resize)
button = widget_label(col_4r, value = ' pixels')
w_hec4 = Widget_text(col_4, Value = '10')
w_acr4 = Widget_text(col_4, Value = '10')

;; column 5: class5
col_5 = widget_base(row1, / column, / frame)
button = widget_label(col_5, value = 'Class 5:', / sunken_frame, / align_center)
col_5r = widget_base(col_5,/row)
button = widget_label(col_5r, value = '-> ')
w_cl5 = Widget_combobox(col_5r, Value = classes, / editable, UValue = 'cl5',/dynamic_resize)
button = widget_label(col_5r, value = ' pixels')
w_hec5 = Widget_text(col_5, Value = '10')
w_acr5 = Widget_text(col_5, Value = '10')

;; column 6: class6
col_6 = widget_base(row1, / column, / frame)
button = widget_label(col_6, value = 'Class 6:', / sunken_frame, / align_center)
col_6r = widget_base(col_6,/row)
w_cl6 = Widget_combobox(col_6r, Value = classes, sensitive=0,/dynamic_resize) 
button = widget_label(col_6r, value = ' pixels')
button = widget_label(col_6r, value = '-> ')
w_hec6 = Widget_text(col_6, Value = '10')
w_acr6 = Widget_text(col_6, Value = '10')

;; bottom gdal-panel
w_gdal = widget_text(llpart, value = gdalstr)

;; right panel with Options
lrpart = widget_base(lowerpart, / column, / frame)
lrpart1 = widget_base(lrpart,/column,/frame)
button = widget_label(lrpart1, value = 'Accounting Settings:', / sunken_frame, / align_center)
lrpart11 = widget_base(lrpart1,/row)
button = widget_button(lrpart11, Value = 'Reset', uvalue = 'default')
saveID = Widget_Button(lrpart11, Value = 'Save', uvalue = 'save')
loadID = Widget_Button(lrpart11, Value = 'Restore', uvalue = 'restore')
lrpart2 = widget_base(lrpart1,/row)
cancelID = Widget_Button(lrpart2, Value = 'Cancel', uvalue = 'cancel', event_pro = 'get_fgobj_done')
button = widget_label(lrpart2, value = '            ') ;; some space
acceptID = Widget_Button(lrpart2, Value = 'Accept', uvalue = 'accept', event_pro = 'get_fgobj_done')

IF saveopt eq '1' THEN BEGIN
  lrpart3 = widget_base(lrpart,/column,/frame)
  button = widget_label(lrpart3, value = 'File Save Options:', / sunken_frame, / align_center)
  bb = widget_base(lrpart3,/row)
  val = ['Stats + Viewport only','Stats + View,ID,Area'] & saveopt = 0
  button = CW_BGROUP(bb, val, /column, /EXCLUSIVE, set_value=saveopt, uvalue = 'setsave')
ENDIF

;; Center the widgets on display.
get_fgobj_CenterTLB, tlb
Widget_Control, tlb, /Realize


;; set initial defaults
;;==========================================
;; 8-connectivity: ms8
widget_control, w_fgconn, set_combobox_select = 0 & ms8 = fgconn_arr[0] ;; fgcon

;; pixel resolution: ms0
widget_control, w_pixres, set_combobox_select = 4 & ms0 = pixres_arr[4] ;; pixres

;; big3pink: ms9
widget_control, w_big3pink, set_combobox_select = 0 & ms9 = big3pink_arr[0] ;; big3pink

;; the 5 classes in pixels
widget_control, w_cl1, set_combobox_select = 1 & ms1 = classes[1]
widget_control, w_cl2, set_combobox_select = 2 & ms2 = classes[2]
widget_control, w_cl3, set_combobox_select = 3 & ms3 = classes[3]
widget_control, w_cl4, set_combobox_select = 4 & ms4 = classes[4]
widget_control, w_cl5, set_combobox_select = 5 & ms5 = classes[5]
widget_control, w_cl6, set_combobox_select = 5 & ms6 = ms5 

if oba_min gt ms1 then begin
  ms1 = strtrim(oba_min,2)
  widget_control, w_cl1, set_combobox_select = 0, set_value = [strtrim(ms1,2), info.classes[1:*]]
endif
if oba_min gt ms2 then begin
  ms2 = strtrim(oba_min,2)
  widget_control, w_cl2, set_combobox_select = 0, set_value = [strtrim(ms2,2), info.classes[1:*]]
endif
if oba_min gt ms3 then begin
  ms3 = strtrim(oba_min,2)
  widget_control, w_cl3, set_combobox_select = 0, set_value = [strtrim(ms3,2), info.classes[1:*]]
endif
if oba_min gt ms4 then begin
  ms4 = strtrim(oba_min,2)
  widget_control, w_cl4, set_combobox_select = 0, set_value = [strtrim(ms4,2), info.classes[1:*]]
endif
if oba_min gt ms5 then begin
  ms5 = strtrim(oba_min,2) & ms6 = ms5
  widget_control, w_cl5, set_combobox_select = 0, set_value = [strtrim(ms5,2), info.classes[1:*]]
  widget_control, w_cl6, set_combobox_select = 0, set_value = [strtrim(ms6,2), info.classes[1:*]]
endif

pixres = float(ms0) ;; pixres in [m]
hec = ((pixres)^2) / 10000.0
acr = hec * 2.47105

;; the 5 classes in hectare
widget_control, w_hec1, set_value = '->  ' + strtrim(hec * ms1,2) + ' ha'
widget_control, w_hec2, set_value = '->  ' + strtrim(hec * ms2,2) + ' ha'
widget_control, w_hec3, set_value = '->  ' + strtrim(hec * ms3,2) + ' ha'
widget_control, w_hec4, set_value = '->  ' + strtrim(hec * ms4,2) + ' ha'
widget_control, w_hec5, set_value = '->  ' + strtrim(hec * ms5,2) + ' ha'
widget_control, w_hec6, set_value = strtrim(hec * ms5,2) + ' ha ->' 

;; the 5 classes in acres
widget_control, w_acr1, set_value = '->  ' + strtrim(acr * ms1,2) + ' ac'
widget_control, w_acr2, set_value = '->  ' + strtrim(acr * ms2,2) + ' ac'
widget_control, w_acr3, set_value = '->  ' + strtrim(acr * ms3,2) + ' ac'
widget_control, w_acr4, set_value = '->  ' + strtrim(acr * ms4,2) + ' ac'
widget_control, w_acr5, set_value = '->  ' + strtrim(acr * ms5,2) + ' ac'
widget_control, w_acr6, set_value = strtrim(acr * ms5,2) + ' ac ->' 

;; center the tlb
get_fgobj_CenterTLB, tlb

;; Store the program information:
info = {destroy_groupleader:destroy_groupleader, $
        o_fgconn:o_fgconn, $     ;; pointer to the final fgconn
        o_pixres:o_pixres, $     ;; pointer to the final pixres
        o_big3pink:o_big3pink, $ ;; pointer to the final big3pink
        o_cl1:o_cl1, $           ;; pointer to the final cl1
        o_cl2:o_cl2, $           ;; pointer to the final cl2
        o_cl3:o_cl3, $           ;; pointer to the final cl3
        o_cl4:o_cl4, $           ;; pointer to the final cl4
        o_cl5:o_cl5, $           ;; pointer to the final cl5
        o_saveopt:o_saveopt, $   ;; pointer to the save option, only when in batch mode       
        cancel:cancel, $
        w_fgconn:w_fgconn, $
        w_pixres:w_pixres, $
        w_big3pink:w_big3pink, $
        oba_min:oba_min, $
        w_cl1:w_cl1, $
        w_cl2:w_cl2, $
        w_cl3:w_cl3, $
        w_cl4:w_cl4, $
        w_cl5:w_cl5, $
        w_cl6:w_cl6, $
        w_hec1:w_hec1, $
        w_hec2:w_hec2, $
        w_hec3:w_hec3, $
        w_hec4:w_hec4, $
        w_hec5:w_hec5, $
        w_hec6:w_hec6, $
        w_acr1:w_acr1, $
        w_acr2:w_acr2, $
        w_acr3:w_acr3, $
        w_acr4:w_acr4, $
        w_acr5:w_acr5, $
        w_acr6:w_acr6, $
        fgconn_arr:fgconn_arr, $
        pixres_arr:pixres_arr, $
        big3pink_arr:big3pink_arr, $
        classes:classes, $
        scaled_cl_arr:scaled_cl_arr, $
        ms8:ms8, $ ;; fgconn selector
        ms0:ms0, $ ;; pixres selector
        ms9:ms9, $ ;; big3pink selector
        ms1:ms1, $ ;; class1 selector
        ms2:ms2, $ ;; class2 selector
        ms3:ms3, $ ;; class3 selector
        ms4:ms4, $ ;; class4 selector
        ms5:ms5, $ ;; class5 selector
        cancelID:cancelID, $
        acceptID:acceptID, $
        saveopt:saveopt}  
Widget_Control, tlb, Set_UValue=info, /No_Copy

;; Blocking or modal widget, depending upon group leader.
XManager, 'get_fgobj', tlb, Group_Leader = group

END ;-----------------------------------------------------

