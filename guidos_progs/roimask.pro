FUNCTION ROIMask, $
   image, $                 ; A 2D image.
   title=title, $           ; title
   Indices=indices, $       ; Outputs the indices inside the ROI
   mask                     ; the output image

;; Draw ROI's on image. (Use the freehand PENCIL tool, for example.)
XROI, image, Regions_Out=rois, title=title, /Block

;; Create an image mask from the ROIs you just created.
dim = Size(image, /Dimensions)
mask = BytArr(dim[0], dim[1])

;; Cycle through the ROIs.
FOR j=0, N_Elements(rois)-1 DO BEGIN
   thisROI = rois[j]
   IF Obj_Valid(thisROI) THEN BEGIN
      thisROI -> GetProperty, Data=polygon
      indices = PolyFillV(polygon[0,*], polygon[1,*], dim[0], dim[1])
      IF indices[0] NE -1 THEN mask[indices] = 1b
      Obj_Destroy, thisROI
   ENDIF
ENDFOR

;; return the mask 
RETURN, mask
END