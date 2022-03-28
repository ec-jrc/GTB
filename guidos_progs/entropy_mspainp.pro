pro entropy_mspainp, arrayin, mw, entropy, entarr
;; requires a MSPA-inut image
;; returns normalised entropy
;;
;; assign different values to FG/BG/M to obtain unique values when building the differences
;; this way we can find only these combinations:
;; 0-7 249 (-7)
;; 0-3 253 (-3)
;; 0-0  0
;; 3-7 252 (-4)
;; 3-3  0
;; 3-0 +3
;; 7-7  0
;; 7-3 +4
;; 7-0 +7
;; so the possible range of differences is [ 0, 3, 4, 7, 249, 252, 253 ]
image = (arrayin eq 2b)*7b + (arrayin eq 1b)*3b
q = size(image,/dim) & sim_x = q[0] & sim_y = q[1]
Ent_max = 1.5 & al2 = alog(2)
normfac=100.0/al2/Ent_max

;; calculate entropy on full image  
  ;; extend the image to avoid border effects
  sext_x = sim_x + 2 & sext_y = sim_y + 2
  ext = bytarr(sext_x, sext_y)
  ext[1:sim_x, 1:sim_y] = image
  ext[0, 1:sim_y] = image[0,*] ;left
  ext[1+sim_x:*, 1:sim_y] = image[sim_x-1:*,*] ;right
  ext[1:sim_x, 0] = image[*, 0] ;top
  ext[1:sim_x, 1+sim_y:*] = image[*, sim_y-1:*] ;bottom
  if mw eq 0 then image = 0
  
  ;; count occurences of each possible pixel to pixel difference value.  Store these values in prob
  ;;
  ;; shift(ext, x, y):  if we calculate X - shifted X
  ;; (i.e. if ext is shifted by -1 pixel in x-direction then we look at the difference to the pixel to the right
  ;;  which implies that the last column will be calculated via a flip-over of the ext, not good)
  ;; X: -  last column
  ;; X: +  first column
  ;; Y: -  last line
  ;; Y: +  first line
  ;;
  ;; invalidate all border lines of extended array: they are calculated by flip overs
  ;; therefore not to be considered. Set them to 255b, this value is never
  ;; encountered within the image area but we need it to ensure that the number
  ;; of elements in the histogram are always 256; this ensures that we can sum
  ;; the histogram ranges because they will then all have the same number of elements

  d = ext - shift(ext, - 1, 0) ; diff to the right neighbor
  d[0,*] = 255b & d[*,0] = 255b & d[sext_x - 1, * ] = 255b & d[*, sext_y-1] = 255b
  prob = histogram(d)
  
  d = ext - shift(ext, + 1, 0) ; diff to the left neighbor
  d[0,*] = 255b & d[*,0] = 255b & d[sext_x - 1, * ] = 255b & d[*, sext_y-1] = 255b
  prob = prob + histogram(d)
  
  d = ext - shift(ext, 0, - 1) ; diff to the neighbor below
  d[0,*] = 255b & d[*,0] = 255b & d[sext_x - 1, * ] = 255b & d[*, sext_y-1] = 255b
  prob = prob + histogram(d)

  d = ext - shift(ext, 0, 1) ; diff to the neighbor above
  d[0,*] = 255b & d[*,0] = 255b & d[sext_x - 1, * ] = 255b & d[*, sext_y-1] = 255b
  prob = prob + histogram(d)

  d = ext - shift(ext, - 1, - 1); difference to right below
  d[0,*] = 255b & d[*,0] = 255b & d[sext_x - 1, * ] = 255b & d[*, sext_y-1] = 255b
  prob = prob + histogram(d)

  d = ext - shift(ext, 1, 1); difference to left above
  d[0,*] = 255b & d[*,0] = 255b & d[sext_x - 1, * ] = 255b & d[*, sext_y-1] = 255b
  prob = prob + histogram(d)

  d = ext - shift(ext, - 1, 1); difference to right above
  d[0,*] = 255b & d[*,0] = 255b & d[sext_x - 1, * ] = 255b & d[*, sext_y-1] = 255b
  prob = prob + histogram(d)

  d = ext - shift(ext, 1, - 1); difference to left below
  d[0,*] = 255b & d[*,0] = 255b & d[sext_x - 1, * ] = 255b & d[*, sext_y-1] = 255b
  prob = prob + histogram(d)
 
  ;; reset the one for the boundaries so it will not be considered
  prob(255)=0

  ;; Convert values in prob to probabilities and compute entropy
;  k=Where(prob GT 0) & prob = prob/total(prob)
;  entropy = -total(prob[k]*alog(prob[k]))/alog(2)
  prob=prob(Where(prob GT 0)) & prob = prob/total(prob)
  entropy = [ -total(prob*alog(prob))] * normfac < 100.0

if mw eq 0 then goto,fin


;; moving window

;; extend the image to avoid border effects
;; we use a 50 x 50 window, enlarge a bit more and then 
;; only calculate for the actual image area, this way we 
;; don't need to invalidate the borders  
dx = 26 & dy = dx
ext = bytarr(sim_x + 2*dx, sim_y + 2*dy) 
sz=size(ext,/dim) & sext_x = sz[0] & sext_y = sz[1]
ext[dx:dx + sim_x - 1, dy:dy + sim_y - 1] = image
ext[0:dx-1,     dy:dy+sim_y-1] = reverse(image[0:dx-1,*]) ;left
ext[dx+sim_x:*, dy:dy+sim_y-1] = reverse(image[sim_x-dx:*,*]) ;right
ext[dx:dx + sim_x - 1, 0:dy-1] = reverse(image[*, 0:dy-1],2) ;top
ext[dx:dx + sim_x - 1, dy+sim_y:*] = reverse(image[*, sim_y-dy:*],2) ;bottom
entarr = temporary(image) * 0b 

stack = bytarr(8, sext_x, sext_y)
stack(0,*,*) = ext - shift(ext, - 1, 0) 
stack(1,*,*)  = ext - shift(ext, + 1, 0)
stack(2,*,*)  = ext - shift(ext, 0, - 1) 
stack(3,*,*)  = ext - shift(ext, 0, 1)
stack(4,*,*)  = ext - shift(ext, - 1, - 1)
stack(5,*,*)  = ext - shift(ext, 1, 1)
stack(6,*,*)  = ext - shift(ext, - 1, 1) 
stack(7,*,*)  = ext - shift(ext, 1, - 1) 

for idy = dy, dy + sim_y - 1 do begin
  for idx = dx,dx + sim_x -1 do begin
    tt = stack[*, idx-dx+1:idx+dx-2, idy-dy+1:idy+dy-2]
;    prob=histogram(tt) & k=Where(prob GT 0) & prob = prob/total(prob)
;    z = -total(prob[k]*alog(prob[k])) ;/alog(2)
    prob=histogram(tt) & prob=prob(Where(prob GT 0)) & prob = prob/total(prob)
    z = -total(prob*alog(prob))
    entarr[idx-dx, idy-dy] = byte(round(z*normfac)<100.0)    
  endfor
endfor

fin:

end