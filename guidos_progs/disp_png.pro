 PRO DISP_PNG, filepng ;, truecolor = truecolor, R, G, B
;PRO DISP_PNG, img, filepng ;, truecolor = truecolor, R, G, B
;; test on display settings (truecolor or not)
;; on 24-bit displays make sure device decomposition
;; - is ON for output to file
;; - is OFF for output to screen
;;
truecolor = fix((!D.Flags AND 256) NE 0)

;; settings for output of screen dump
;; get screen dump, 2-D on 8-bit display, 3-D on 24-bit displays
;wshow, !D.Window &
tvlct, r, g, b, / get
IF truecolor THEN BEGIN
   device, get_decomposed = theDecomposedState
   device, decomposed = 1
   img = tvrd(0, 0, !D.X_size, !D.Y_size, true = truecolor)
   img2D = color_quan(img, 1, r, g, b, / map_all)
   write_png, filepng + '.png', img2D, r, g, b
   ;;reset settings to get propper output on screen
   device, decomposed = theDecomposedState
ENDIF ELSE BEGIN
   tvlct, r, g, b, / get
   img = tvrd(0, 0, !D.X_size, !D.Y_size)
   write_png, filepng + '.png', img, r, g, b
ENDELSE

END ;; of 'Disp_png'
