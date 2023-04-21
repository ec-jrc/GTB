PRO get_gsc_CenterTLB, tlb

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
; of get_gsc_centerTLB   *********************************************


PRO get_gsc_done, event
;; return the selected kernel size to the main program
Widget_Control, event.top, Get_UValue = info, / No_Copy

;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue
IF strlowCase(eventValue) EQ 'accept' THEN BEGIN
  * info.sc_m = info.ms_m
  * info.sc_f = info.ms_f
  * info.sc_g = info.ms_g
  * info.sc_p = info.ms_p
  * info.sc_w = info.ms_w
  * info.sc_a = info.ms_a
  * info.sc_b = info.ms_b
  * info.sc_x = info.ms_x
  * info.sc_y = info.ms_y
  * info.sc_k = info.ms_k
  * info.sc_n = info.ms_n
  * info.cancel = 0b   ;;set cancel to accept
ENDIF 

;; quit the application
Widget_Control, event.top, / Destroy

;; perform cleanup
IF info.destroy_groupleader THEN Widget_Control, groupleader, /Destroy

END
;  of get_gsc_done   *********************************************



PRO get_gsc_Event, event
;; get the info structure
Widget_Control, event.top, Get_UValue = info, / No_Copy

;; What kind of processing do you need?
Widget_Control, event.id, Get_Uvalue = eventValue

CASE strlowCase(eventValue) OF
   'gsc_guide': BEGIN
     doc = 'GRAYSPATCON_Guide.pdf'
     IF info.my_os EQ 'apple' THEN BEGIN
       spawn, 'open ' + info.dir_guidossub + 'spatcon/' + doc
     ENDIF
     widget_control, / hourglass
     IF info.my_os EQ 'windows' THEN BEGIN
       pushd, info.dir_guidossub + 'spatcon'
       spawn, 'start ' + doc, / nowait, /hide
       popd
     ENDIF ELSE BEGIN ;; Linux
       cmd = info.pdf_exe + ' "' + info.dir_guidossub + 'spatcon/' + doc + '"'
       spawn, cmd + ' &'
     ENDELSE
   END

   'default': BEGIN
      ;; set initial defaults
      widget_control, info.w_gsc_m, set_combobox_select = 0 & info.ms_m = info.gsc_m_arr[0]
      widget_control, info.w_gsc_f, set_combobox_select = 1 & info.ms_f = info.gsc_f_arr[1]
      widget_control, info.w_gsc_g, set_combobox_select = 0 & info.ms_g = info.gsc_g_arr[0]
      widget_control, info.w_gsc_p, set_combobox_select = 0 & info.ms_p = info.gsc_p_arr[0]
      widget_control, info.w_gsc_w, set_combobox_select = 7, sensitive = 1 & info.ms_w = info.gsc_w_arr[7]
      widget_control, info.w_gsc_a, set_combobox_select = 1, sensitive = 1 & info.ms_a = info.gsc_a_arr[1]
      widget_control, info.w_gsc_b, set_combobox_select = 0 & info.ms_b = info.gsc_b_arr[0]
      widget_control, info.w_gsc_x, set_combobox_select = 4 & info.ms_x = info.gsc_x_arr[4]
      widget_control, info.w_gsc_y, set_combobox_select = 9 & info.ms_y = info.gsc_y_arr[9]
      widget_control, info.w_gsc_k, set_combobox_select = 4 & info.ms_k = info.gsc_k_arr[4]
      widget_control, info.w_gsc_n, set_combobox_select = 1, sensitive = 1 & info.ms_n = info.gsc_n_arr[1]
   END 

   'gsc_m': BEGIN
      widget_control, event.id, get_value = val
      sc_m = val(event.index)
      info.ms_m = sc_m
      ;; set to float for these metrics
      q = (strsplit(sc_m,':',/extract))[0] & q = fix(q)
      IF q EQ 44 OR q EQ 45 OR q EQ 50 THEN BEGIN ; activate float and deactivate bytestretch?
        sc_f = '2' & info.ms_f = sc_f
        widget_control, info.w_gsc_f, set_combobox_select = 1
        widget_control, info.w_gsc_b, sensitive = 0
        widget_control, info.w_gsc_n, set_combobox_select = 1, sensitive = 1 & info.ms_n = info.gsc_n_arr[1]
      ENDIF
      condition = q gt 20 and q lt 25 ; activate w_x for metric 21, 22, 23, 24
      if condition then widget_control, info.w_gsc_x, sensitive = 1 else widget_control, info.w_gsc_x, sensitive = 0
      condition = q eq 23 or q eq 24 ; activate w_y for metric 23, 24
      if condition then widget_control, info.w_gsc_y, sensitive = 1 else widget_control, info.w_gsc_y, sensitive = 0
      condition = q eq 49 ; activate w_k for metric 49
      if condition then widget_control, info.w_gsc_k, sensitive = 1 else widget_control, info.w_gsc_k, sensitive = 0      
      
      ;; test for byteStretch compatibility if Byte output is set
      IF info.ms_f EQ '1:Byte' THEN BEGIN
        bsel = (strsplit(info.ms_b,':',/extract))[0] & bsel = fix(bsel)
        msel = (strsplit(info.ms_m,':',/extract))[0] & msel = fix(msel)
        ;; we have 3 conditions to deal with
        ;; 1) bounded metrics: 2, 3, 6, 8, 10-15, 20-24, 31-38, 40, 42, 49
        marr1 = strtrim([2,3,6,8,10,11,12,13,14,15,20,21,22,23,24,31,32,33,34,35,36,37,38,40,42,49],2)
        ;; 3) no-stretch metrics: 1, 9, 16, 17, 18, 19, 25, 27
        marr3 = strtrim([1,9,16,17,18,19,25,27],2)
        ;; 2) unbounded metrics: those not in arr1 or arr3: 4, 5, 7, 26, 28-30, 39, 41, 43-48, 50, 51
        marr2 = strtrim([4,5,7,26,28,29,30,39,41,43,44,45,46,47,48,50,51],2)
        ;; find out which condition we have
        q = where(marr1 eq msel, ct1)
        q = where(marr2 eq msel, ct2)
        q = where(marr3 eq msel, ct3)
        IF ct1 THEN BEGIN ; selected metric can only have bsel option 1 or 2
          if bsel gt 2 then begin ; invalid selection
            sc_b = '1:[0.0, 1.0]->[0b, 100b]'
            msg = 'GSC ' + strtrim(msel,2) + ' requires ByteStretch option 1 or 2, please set B'
            res = dialog_message(msg, / information)
          endif
          info.ms_b = sc_b
        ENDIF ELSE IF ct2 THEN BEGIN ; selected metric can only have bsel option 3,4,5
          if bsel lt 3 OR bsel EQ 6 then begin ; invalid selection
            sc_b = '3:[Min, Max]->[0b, 254b]'
            msg = 'GSC ' + strtrim(msel,2)  + ' requires ByteStretch option 3, 4, or 5, please set B'
            res = dialog_message(msg, / information)
          endif
          info.ms_b = sc_b
        ENDIF ELSE IF ct3 THEN BEGIN ; enforce B 6 for no-stretch metrics: 1, 9, 16, 17, 18, 19, 25, 27
          if bsel lt 6 then begin ; invalid selection
            sc_b = '6:float -> byte'
            msg = 'GSC ' + strtrim(msel,2)  + ': enforcing no ByteStretch option 6'
            res = dialog_message(msg, / information)
          endif
          info.ms_b = sc_b
        ENDIF
        bsel = (strsplit(sc_b,':',/extract))[0] & bsel = fix(bsel)
        widget_control, info.w_gsc_b, set_combobox_select = bsel-1           
      ENDIF               
   END 

   'gsc_f': BEGIN
     widget_control, event.id, get_value = val
     sc_f = val(event.index)
     ;; set to float for these metrics
     q = (strsplit(info.ms_m,':',/extract))[0] & q = fix(q)
     IF q EQ 44 OR q EQ 45 OR q EQ 50 THEN BEGIN ; enforce float for metric 44, 45, 50
      sc_f = '2:Float'
      msg = 'GSC ' + strtrim(q,2)  + ': enforcing F:Precision to 2'
      res = dialog_message(msg, / information)
      widget_control, info.w_gsc_f, set_combobox_select = 1
     ENDIF 
     info.ms_f = sc_f
     widget_control, info.w_gsc_b, sensitive = info.ms_f eq '1:Byte' ; activate bytestretch if byte output 
     
     ;; test for byteStretch compatibility if Byte output is set
     IF info.ms_f EQ '1:Byte' THEN BEGIN
       bsel = (strsplit(info.ms_b,':',/extract))[0] & bsel = fix(bsel)
       msel = (strsplit(info.ms_m,':',/extract))[0] & msel = fix(msel)
       ;; we have 3 conditions to deal with
       ;; 1) bounded metrics: 2, 3, 6, 8, 10-15, 20-24, 31-38, 40, 42, 49
       marr1 = strtrim([2,3,6,8,10,11,12,13,14,15,20,21,22,23,24,31,32,33,34,35,36,37,38,40,42,49],2)
       ;; 3) no-stretch metrics: 1, 9, 16, 17, 18, 19, 25, 27
       marr3 = strtrim([1,9,16,17,18,19,25,27],2)
       ;; 2) unbounded metrics: those not in arr1 or arr3: 4, 5, 7, 26, 28-30, 39, 41, 43-48, 50, 51
       marr2 = strtrim([4,5,7,26,28,29,30,39,41,43,44,45,46,47,48,50,51],2)
       ;; find out which condition we have
       q = where(marr1 eq msel, ct1)
       q = where(marr2 eq msel, ct2)
       q = where(marr3 eq msel, ct3)
       sc_b = info.ms_b ;; get currently selected option
       IF ct1 THEN BEGIN ; selected metric can only have bsel option 1 or 2
         if bsel gt 2 then begin ; invalid selection
           sc_b = '1:[0.0, 1.0]->[0b, 100b]'
           msg = 'GSC ' + strtrim(msel,2) + ' requires ByteStretch option 1 or 2, please set B'
           res = dialog_message(msg, / information)
         endif
         info.ms_b = sc_b
       ENDIF ELSE IF ct2 THEN BEGIN ; selected metric can only have bsel option 3,4,5
         if bsel lt 3 OR bsel EQ 6 then begin ; invalid selection
           sc_b = '3:[Min, Max]->[0b, 254b]'
           msg = 'GSC ' + strtrim(msel,2)  + ' requires ByteStretch option 3, 4, or 5, please set B'
           res = dialog_message(msg, / information)
         endif
         info.ms_b = sc_b
       ENDIF ELSE IF ct3 THEN BEGIN ; enforce B 6 for no-stretch metrics: 1, 9, 16, 17, 18, 19, 25, 27
         if bsel lt 6 then begin ; invalid selection
           sc_b = '6:float -> byte'
           msg = 'GSC ' + strtrim(msel,2)  + ': enforcing no ByteStretch option 6'
           res = dialog_message(msg, / information)
         endif
         info.ms_b = sc_b
       ENDIF
       bsel = (strsplit(sc_b,':',/extract))[0] & bsel = fix(bsel)
       widget_control, info.w_gsc_b, set_combobox_select = bsel-1
       widget_control, info.w_gsc_n, set_combobox_select = 0, sensitive = 0 & info.ms_n = info.gsc_n_arr[0]
     ENDIF ELSE BEGIN ;; float
       widget_control, info.w_gsc_n, set_combobox_select = 1, sensitive = 1 & info.ms_n = info.gsc_n_arr[1]
     ENDELSE     
   END

   'gsc_g': BEGIN
     widget_control, event.id, get_value = val
     sc_g = val(event.index)
     info.ms_g = sc_g
     ;; activate w_w and w_p if G=0
     q = (strsplit(info.ms_g,':',/extract))[0] & q = fix(q)
     widget_control, info.w_gsc_w, sensitive = 1 - q
     widget_control, info.w_gsc_a, sensitive = 1 - q
   END
   
   'gsc_p': BEGIN
     widget_control, event.id, get_value = val
     sc_p = val(event.index)
     info.ms_p = sc_p
   END
   
   'gsc_w': BEGIN
     if event.index le 0 then begin ;; user-specified value, correct for wrong input
       x = uint(abs(event.str)) 
       ;; ensure decent range for new value of x
       x = 3 > x < info.wmax
       if (x mod 2) eq 0 then x=x+1
       kdim = strtrim(x,2)
       widget_control, info.w_gsc_w, set_value = [kdim,'3', '5', '7', '9', '11', '13', '27', '81', '243']
     endif else begin ; user-selected value
       widget_control, event.id, get_value = val
       kdim = val(event.index)
     endelse
     info.ms_w = kdim
   END 

   'gsc_a': BEGIN
     widget_control, event.id, get_value = val
     sc_a = val(event.index)
     info.ms_a = sc_a
   END

   'gsc_b': BEGIN
     widget_control, event.id, get_value = val
     sc_b = val(event.index)
     bsel = (strsplit(sc_b,':',/extract))[0] & bsel = fix(bsel)
     msel = (strsplit(info.ms_m,':',/extract))[0] & msel = fix(msel)
     ;; we have 3 conditions to deal with
     ;; 1) bounded metrics: 2, 3, 6, 8, 10-15, 20-24, 31-38, 40, 42, 49
     marr1 = strtrim([2,3,6,8,10,11,12,13,14,15,20,21,22,23,24,31,32,33,34,35,36,37,38,40,42,49],2)
     ;; 3) no-stretch metrics: 1, 9, 16, 17, 18, 19, 25, 27
     marr3 = strtrim([1,9,16,17,18,19,25,27],2)
     ;; 2) unbounded metrics: those not in arr1 or arr3: 4, 5, 7, 26, 28-30, 39, 41, 43-48, 50, 51
     marr2 = strtrim([4,5,7,26,28,29,30,39,41,43,44,45,46,47,48,50,51],2)
     ;; find out which condition we have
     q = where(marr1 eq msel, ct1)
     q = where(marr2 eq msel, ct2)
     q = where(marr3 eq msel, ct3)
     IF ct1 THEN BEGIN ; selected metric can only have bsel option 1 or 2
       if bsel gt 2 then begin ; invalid selection
         sc_b = '1:[0.0, 1.0]->[0b, 100b]'
         msg = 'GSC ' + strtrim(msel,2) + ' requires ByteStretch option 1 or 2, please set B'
         res = dialog_message(msg, / information)
       endif
       info.ms_b = sc_b     
     ENDIF ELSE IF ct2 THEN BEGIN ; selected metric can only have bsel option 3,4,5
       if bsel lt 3 OR bsel EQ 6 then begin ; invalid selection
         sc_b = '3:[Min, Max]->[0b, 254b]'
         msg = 'GSC ' + strtrim(msel,2)  + ' requires ByteStretch option 3, 4, or 5, please set B'
         res = dialog_message(msg, / information)
       endif 
       info.ms_b = sc_b      
     ENDIF ELSE IF ct3 THEN BEGIN ; enforce B 6 for no-stretch metrics: 1, 9, 16, 17, 18, 19, 25, 27
       if bsel lt 6 then begin ; invalid selection
         sc_b = '6:float -> byte'
         msg = 'GSC ' + strtrim(msel,2)  + ': enforcing no ByteStretch option 6'
         res = dialog_message(msg, / information)
       endif
       info.ms_b = sc_b
     ENDIF
     bsel = (strsplit(sc_b,':',/extract))[0] & bsel = fix(bsel)
     widget_control, info.w_gsc_b, set_combobox_select = bsel-1     
   END

   'gsc_x': BEGIN
     widget_control, event.id, get_value = val
     sc_x = val(event.index)
     info.ms_x = sc_x
   END

   'gsc_y': BEGIN
     widget_control, event.id, get_value = val
     sc_y = val(event.index)
     info.ms_y = sc_y
   END

   'gsc_k': BEGIN
     widget_control, event.id, get_value = val
     sc_k = val(event.index)
     info.ms_k = sc_k
   END  

   'gsc_n': BEGIN
     widget_control, event.id, get_value = val
     sc_n = val(event.index)
     info.ms_n = sc_n
   END
   
ENDCASE
Widget_Control, event.top, set_UValue = info, / No_Copy
END
;  of get_gsc_event   *********************************************


;;**************************************************************************
PRO get_gsc, sc_m=sc_m, sc_f=sc_f, sc_g=sc_g, sc_p=sc_p, sc_w=sc_w, sc_a=sc_a, sc_b=sc_b, $
  sc_x=sc_x, sc_y=sc_y, sc_k=sc_k, sc_n=sc_n, title = title, dir_guidossub=dir_guidossub, pdf_exe=pdf_exe, my_os=my_os, $
  imgminsize=imgminsize, Cancel = cancel, Group_Leader = groupleader

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
IF keyword_set(title) THEN title = title ELSE title = 'GraySpatCon'

;; Provide a group leader if not supplied with one. This
;; is required for modal operation of widgets. Set a flag
;; for destroying the group leader widget before returning.
IF N_Elements(groupleader) EQ 0 THEN BEGIN
   groupleader = Widget_Base(Map = 0)
   Widget_Control, groupleader, / Realize
   destroy_groupleader = 1
ENDIF ELSE destroy_groupleader = 0

;; Create modal base widget.
tlb = Widget_Base(Title = title, Column = 1,  / Base_Align_Center, / Modal, Group_Leader = groupleader)

;; Create the rest of the widgets.
;; bottom part widgets
lowerpart = widget_base(tlb, / row)
llpart = widget_base(lowerpart, / column, / frame)

llpartA = widget_base(llpart, /frame, / row) ;; first row
dummy = widget_label(llpartA, value = 'Mandatory GSC Settings:')
;; M-Metric
llpart10 = widget_base(llpartA, / column, / frame)
button = widget_label(llpart10, value = 'M:Metric', / sunken_frame, / align_center)
gsc_m_arr = ['1:Mean', '2:EvennessOrderedAdj', '3:EvennessUnorderedAdj', '4:EntropyOrderedAdj', '5:EntropyUnorderedAdj', '6:DiagonalContagion', $
  '7:ShannonDiversity', '8:ShannonEvenness', '9:Median', '10:GSDiversity', '11:GSEvenness', '12:EquitabilityOrderedAdj', $
  '13:EquitabilityUnorderedAdj', '14:DiversityOrderedAdj', '15:DiversityUnorderedAdj', '16:Majority', '17:LandscapeMosaic19', '18:LandscapeMosaic103', $
  '19:NumberGrayLevels', '20:MaxAreaDensity', '21:FocalAreaDensity', '22:FocalAdjT1', '23:FocalAdjT1andT2', '24:FocalAdjT1givenT2', $
  '25:StandardDeviation', '26:CoefficientDeviation', '27:Range', '28:Dissimilarity', '29:Contrast', '30:UniformityOrderedAdj', $
  '31:UniformityUnorderedAdj', '32:Homogeneity', '33:InverseDifference', '34:SimilarityRMax', '35:SimilarityRGlobal', '36:SimilarityRWindows', $
  '37:DominanceOrderedAdj', '38:DominanceUnorderedAdj', '39:DifferenceEntropy', '40:DifferenceEvenness', '41:SumEntropy', '42:SumEvenness', $
  '43:AutoCorrelation', '44:Correlation', '45:ClusterShade', '46:ClusterProminence', '47:RootMeanSquare', '48:AverageAbsDeviation', $
  '49:kContagion', '50:Skewness', '51:Kurtosis']
w_gsc_m = Widget_combobox(llpart10, Value = gsc_m_arr, UValue = 'gsc_m')

;; F-Precision
llpart11 = widget_base(llpartA, / column, / frame)
button = widget_label(llpart11, value = 'F:Precision', / sunken_frame, / align_center)
gsc_f_arr = ['1:Byte', '2:Float']
w_gsc_f = Widget_combobox(llpart11, Value = gsc_f_arr, UValue = 'gsc_f')

;; G-Analysis Type
llpart12 = widget_base(llpartA, / column, / frame)
button = widget_label(llpart12, value = 'G:Analysis Type', / sunken_frame, / align_center)
gsc_g_arr = ['0:MovingWindow', '1:Global']
w_gsc_g = Widget_combobox(llpart12, Value = gsc_g_arr, UValue = 'gsc_g')

;; P-Analysis Type
llpart13 = widget_base(llpartA, / column, / frame)
button = widget_label(llpart13, value = 'P:Pixel 0', / sunken_frame, / align_center)
gsc_p_arr = ['0:Include', '1:Exclude']
w_gsc_p = Widget_combobox(llpart13, Value = gsc_p_arr, UValue = 'gsc_p')

;;;;;;;;;;;;;;;;;; second panel
llpartB = widget_base(llpart, / row, /frame) ;; second row
;; block 1
block1 = widget_base(llpartB, /frame, /column)
block2 = widget_base(llpartB, /frame, /column)
block3 = widget_base(llpartB, /frame, /column)

;llpartB0 = widget_base(llpart, / column, /frame)

dummy = widget_label(block1, value = 'If G=0 (moving window analysis):')
llpartB = widget_base(block1, / row)
llpart20 = widget_base(llpartb, / column)

;; W-Winsize
llpart20 = widget_base(llpartB, / column, / frame)
button = widget_label(llpart20, value = 'W:WindowSize', / sunken_frame, / align_center)
gsc_w_arr = ['X', '3', '5', '7', '9', '11', '13', '27', '81', '243']
w_gsc_w = Widget_combobox(llpart20, Value = gsc_w_arr, / editable, UValue = 'gsc_w')

;; A - Mask Missing
llpart21 = widget_base(llpartB, / column, / frame)
button = widget_label(llpart21, value = 'A:MaskMissing', / sunken_frame, / align_center)
gsc_a_arr = ['0:No', '1:Yes']
w_gsc_a = Widget_combobox(llpart21, Value = gsc_a_arr, UValue = 'gsc_a')

;;;;;;;;;;;;;;;;;;; third panel
dummy = widget_label(block2, value = 'If F=1 (Byte Output):')
llpartC = widget_base(block2, / row)
llpart30 = widget_base(llpartC, / column, / frame)
button = widget_label(llpart30, value = 'B:ByteStretch', / sunken_frame, / align_center)
gsc_b_arr = ['1:[0.0, 1.0]->[0b, 100b]', '2:[0.0, 1.0]->[0b, 254b]', '3:[Min, Max]->[0b, 254b]', '4:[0.0, Max]->[0b, 254b]', $
  '5:[0.0,Max]->[0b, 100b]', '6:float -> byte']
w_gsc_b = Widget_combobox(llpart30, Value = gsc_b_arr, UValue = 'gsc_b', sensitive = 0)
;
;;;;;;;;;;;;;;;;;;; forth panel
dummy = widget_label(block3, value = 'Target Settings:')
llpartD = widget_base(block3, / row)
llpart40 = widget_base(llpartD, / column, / frame)
button = widget_label(llpart40, value = 'X:Code1', / sunken_frame, / align_center)
gsc_x_arr = strtrim(indgen(100)+1,2)
w_gsc_x = Widget_combobox(llpart40, Value = gsc_x_arr, UValue = 'gsc_x', sensitive = 0)

llpart41 = widget_base(llpartD, / column, / frame)
button = widget_label(llpart41, value = 'Y:Code2', / sunken_frame, / align_center)
gsc_y_arr = strtrim(indgen(100)+1,2)
w_gsc_y = Widget_combobox(llpart41, Value = gsc_y_arr, UValue = 'gsc_y', sensitive = 0)

llpart42 = widget_base(llpartD, / column, / frame)
button = widget_label(llpart42, value = 'K:Difference', / sunken_frame, / align_center)
gsc_k_arr = strtrim(indgen(100)+1,2)
w_gsc_k = Widget_combobox(llpart42, Value = gsc_k_arr, UValue = 'gsc_k', sensitive = 0)

;;;;;;;;;;;;;;; right panel ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
lrpart = widget_base(lowerpart, / column, / frame)
button = widget_button(lrpart, Value = 'GSC Guide', uvalue = 'gsc_guide')
dummy = widget_label(lrpart, value = '  Options  ', / sunken_frame, / align_center)
lrpart1 = widget_base(lrpart,/row)
gsc_n_arr = ['No', 'Yes']
dummy = widget_label(lrpart1, value = 'Missing->NaN:')
w_gsc_n = Widget_combobox(lrpart1, Value = gsc_n_arr, UValue = 'gsc_n')
;; default, cancel and accept
button = widget_button(lrpart, Value = 'Default values', uvalue = 'default')
cancelID = Widget_Button(lrpart, Value = 'Cancel', uvalue = 'cancel', event_pro = 'get_gsc_done')
acceptID = Widget_Button(lrpart, Value = 'Accept', uvalue = 'accept', event_pro = 'get_gsc_done')


;; Center the widgets on display.
get_gsc_CenterTLB, tlb
Widget_Control, tlb, /Realize

;; set initial defaults
widget_control, w_gsc_m, set_combobox_select = 0  & ms_m = gsc_m_arr[0] ; first metric
widget_control, w_gsc_f, set_combobox_select = 1  & ms_f = gsc_f_arr[1] ; float precision
widget_control, w_gsc_g, set_combobox_select = 0  & ms_g = gsc_g_arr[0] ; moving window
widget_control, w_gsc_p, set_combobox_select = 0  & ms_p = gsc_p_arr[0] ; include pixel with value 0
widget_control, w_gsc_w, set_combobox_select = 7  & ms_w = gsc_w_arr[7] ; window of 27x27
widget_control, w_gsc_a, set_combobox_select = 1  & ms_a = gsc_a_arr[1] ; mask missing pixels
widget_control, w_gsc_b, set_combobox_select = 0  & ms_b = gsc_b_arr[0] ; byte stretch into [0b, 100b]
widget_control, w_gsc_x, set_combobox_select = 4  & ms_x = gsc_x_arr[4] ; Target code 1
widget_control, w_gsc_y, set_combobox_select = 9  & ms_y = gsc_y_arr[9] ; Target code 2
widget_control, w_gsc_k, set_combobox_select = 4  & ms_k = gsc_k_arr[4] ; target difference
widget_control, w_gsc_n, set_combobox_select = 1  & ms_n = gsc_n_arr[1] ; maskNaN 


IF imgminsize EQ 0 THEN BEGIN ; batch mode
  wmax = 32001
ENDIF ELSE BEGIN
  q = imgminsize mod 2
  if q eq 1 then wmax = imgminsize else wmax = imgminsize - 1
ENDELSE

;; center the tlb
get_gsc_CenterTLB, tlb

;; Store the program information:
info = {destroy_groupleader:destroy_groupleader, $
        sc_m:sc_m, $ ;; pointer to the final setting
        sc_f:sc_f, $
        sc_g:sc_g, $
        sc_p:sc_p, $
        sc_w:sc_w, $
        sc_a:sc_a, $
        sc_b:sc_b, $
        sc_x:sc_x, $
        sc_y:sc_y, $
        sc_k:sc_k, $
        sc_n:sc_n, $
        cancel:cancel, $
        w_gsc_m:w_gsc_m, $
        w_gsc_f:w_gsc_f, $
        w_gsc_g:w_gsc_g, $
        w_gsc_p:w_gsc_p, $
        w_gsc_w:w_gsc_w, $
        w_gsc_a:w_gsc_a, $
        w_gsc_b:w_gsc_b, $
        w_gsc_x:w_gsc_x, $
        w_gsc_y:w_gsc_y, $
        w_gsc_k:w_gsc_k, $
        w_gsc_n:w_gsc_n, $
        gsc_m_arr:gsc_m_arr, $
        gsc_f_arr:gsc_f_arr, $
        gsc_g_arr:gsc_g_arr, $
        gsc_p_arr:gsc_p_arr, $
        gsc_w_arr:gsc_w_arr, $
        gsc_a_arr:gsc_a_arr, $
        gsc_b_arr:gsc_b_arr, $
        gsc_x_arr:gsc_x_arr, $
        gsc_y_arr:gsc_y_arr, $
        gsc_k_arr:gsc_k_arr, $
        gsc_n_arr:gsc_n_arr, $
        ms_m:ms_m, $ ;; metric selector
        ms_f:ms_f, $
        ms_g:ms_g, $
        ms_p:ms_p, $
        ms_w:ms_w, $
        ms_a:ms_a, $
        ms_b:ms_b, $
        ms_x:ms_x, $
        ms_y:ms_y, $
        ms_k:ms_k, $
        ms_n:ms_n, $
        dir_guidossub:dir_guidossub, $
        pdf_exe:pdf_exe, $
        my_os:my_os, $
        wmax:wmax, $
        cancelID:cancelID, $
        acceptID:acceptID}  
Widget_Control, tlb, Set_UValue=info, /No_Copy

;; Blocking or modal widget, depending upon group leader.
XManager, 'get_gsc', tlb, Group_Leader = group

END ;-----------------------------------------------------

