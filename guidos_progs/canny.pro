;; Canny edge detection filter
;; required routines/functions

;;=============================================================================
FUNCTION Gauss, x, std
;; Gauss function
return, exp( - x^2 / (2 * std^2)) / (std * sqrt(2) * !pi)
End 
;;=============================================================================

;;=============================================================================
FUNCTION Dgauss, x, std
;; first order derivative of Gauss function
return, - x * gauss(x, std) / std^2
END
;;=============================================================================

;;=============================================================================
FUNCTION D2dgauss, n1, sigma1, n2, sigma2, theta
;; 2D-edge detector (first order derivative of 2D Gaussian function)
;; with size n2*n1. theta is the edge detector angle counterclockwise
;; sigm1/2 std of Gauss
arg = theta/!radeg & h = fltarr(n2, n1) 
r = [[cos(arg), - sin(arg)], [sin(arg), cos(arg)]]

FOR i = 0, n1 - 1 DO BEGIN
   FOR j = 0, n2 - 1 DO BEGIN
      u = r ## transpose([j - (n1 + 1) / 2, i - (n2 + 1) / 2])
      h(i, j) = gauss(u(0), sigma1) ## dgauss(u(1), sigma2)
   ENDFOR
ENDFOR
h = h / sqrt(total((abs(h))^2))
return, h
END
;;=============================================================================



PRO Canny, im, nx1, nx2, ny1, ny2, sigmax1, sigmax2, sigmay1, sigmay2, gr, df
;;=============================================================================
;; edge detection filter returning, rescaled in a bytarr:
;; - g: the gradient 
;; - d: the difference x-y direction
;;
;; input:
;; - im: the image 
;; - nx1,ny1,sigmax1,sigmay1: Gauss distribution over subarray 1
;; - nx2,ny2,sigmax2,sigmay2: Gauss distribution over subarray 2
;; - theta1, theta2 fixed? meaning ?
;;
;; example settings:
;; nx1 = 10 & nx2 = 10 &  sigmax1 = 1 & sigmax2 = 1 & theta1 = !pi / 2
;; ny1 = 10 & ny2 = 10 &  sigmay1 = 1 & sigmay2 = 1 & theta2 = 0
;;
theta1 = !pi / 2.0 & theta2 = 0.0

;; x-axis direction edge detection (derivative)
fx = d2dgauss(nx1, sigmax1, nx2, sigmax2, theta1)
ix = convol(FLOAT(im), fx, total(fx), / EDGE_TRUNCATE)

;; y-axis direction edge detection (derivative)
fy = d2dgauss(ny1, sigmay1, ny2, sigmay2, theta2)
iy = convol(FLOAT(im), fy, total(fy), / EDGE_TRUNCATE)

;; Norm of gradient (combining x- and y- derivative)
gr = bytscl(sqrt(ix^2 + iy^2))

;; difference
df = bytscl(ix - iy)

END 
