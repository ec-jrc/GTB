PRO fmdistg, fname, fgd, bgd, imdist
;; build distance maps for FG and BG
;;
im = read_tiff(fname, channel = 0, geotiff = geotiff) 
bg = (im EQ 1b) & fg = (im EQ 2b)
cloud = im EQ 3b OR im EQ 0b & sz = size(im, / dim)
adf=0.0 & adb=0.0
;; distance map for foreground and adf
fg = morph_distance(fg, neighbor = 3)
;; same for background and adb
bg =  morph_distance(bg, neighbor = 3)
;; combine the foreground and backg distance maps and apply thresholds
imdist = (fg GT 0.0) * ((fg LE fgd) * 211b + (fg GT fgd) * 208b) + $
         (bg GT 0.0) * ((bg LE bgd) * 195b + (bg GT bgd) * 192b) 

xx = fgd > bgd ;; largest distance used
;; mask dilated clouds
IF total(cloud) GT 0.5 THEN BEGIN 
   k = replicate(1, xx, xx) & q = where(dilate(cloud, k) GT 0,ct) 
   if ct gt 0 then imdist(q) = 129b
ENDIF
;; mask frame
imdist(0:xx, * ) = 129b & imdist(sz(0) - xx - 1: * , * ) = 129b
imdist( *, 0:xx) = 129b & imdist( *, sz(1) - xx - 1: *) = 129b


END
