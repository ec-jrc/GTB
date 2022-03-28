FUNCTION Laplace, im
;; Laplace edge detection filter
;; return byte-scaled image
kx = [[ - 1, - 1, - 1], [ - 1, 8, - 1], [ - 1, - 1, - 1]]
lp = CONVOL(FLOAT(im), kx, total(kx), / EDGE_TRUNCATE)
return, bytscl(lp)
END 

