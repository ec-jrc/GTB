; $Id: //depot/Release/ENVI50_IDL82/idl/idldir/lib/graphics/graphic.pro#6 $
;
; Copyright (c) 2010-2013, Exelis Visual Information Solutions, Inc. All
;       rights reserved. Unauthorized reproduction is prohibited.

;+
; :Description:
;   Generic function for creating an IDL graphic.
;
; :Author: sbolin
;-


;-------------------------------------------------------------------------
;+
; :Description:
;   Local utility function used to build up an XML DOM
;   with children.
;
; :Params:
;    oDoc          : XML Document Object
;    oParent       : Dom element to append to.
;    elementName   : Name of the XML Element to create
;    data          : data for the element
;
;-
pro Graphic_AppendChild, oDoc, oParent, elementName, data

  compile_opt idl2, hidden
  
  oElem = oDoc->CreateElement(elementName)
  oVoid = oParent->AppendChild(oElem)
  oText = oDoc->CreateTextNode(data)
  oVoid = oElem->AppendChild(oText)
end


;-------------------------------------------------------------------------
pro Graphic_GridData, zIn, xIn, yIn, zOut, xOut, yOut, SPHERE=sphere

  compile_opt idl2, hidden

  dimZ = SIZE(zIn, /DIM)
  dimX = SIZE(xIn, /DIM)
  dimY = SIZE(yIn, /DIM)
  ; If Z is 2D, and X,Y are 1D, then we have a "regular" grid with
  ; irregular spacing between the grid columns & rows.
  if (SIZE(zIn, /N_DIM) eq 2 && $
    SIZE(xIn, /N_DIM) eq 1 && dimZ[0] eq dimX[0] && $
    SIZE(yIn, /N_DIM) eq 1 && dimZ[0] eq dimY[0]) then begin
    ; In this case, rebin the X and Y up to the correct size,
    ; and call ourself with the new data.
    Graphic_GridData, zIn, REBIN(xIn, dimZ), REBIN(TRANSPOSE(yIn), dimZ), $
      zOut, xOut, yOut, SPHERE=sphere
    return
  endif

  nx = N_ELEMENTS(xIn)
  ny = N_ELEMENTS(yIn)
  nxOut = (100 > SQRT(nx) < 400) < nx
  nyOut = (100 > SQRT(ny) < 400) < ny
  xmin = MIN(xIn, MAX=xmax, /NAN)
  ymin = MIN(yIn, MAX=ymax, /NAN)
  dx = (xmax - xmin)/(nxOut - 1)
  dy = (ymax - ymin)/(nyOut - 1)
  
  ; Sanity check - if units are "degrees", make sure they are within the
  ; lon/lat range before doing spherical gridding.
  if KEYWORD_SET(sphere) then begin
    if (xmin lt -360.01 || xmax gt 540.01 || $
      ymin lt -90.01 || ymax gt 90.01) then begin
      sphere = 0b
    endif
  endif
  
  ; If our grid covers the "entire" sphere, make sure to include points
  ; at the east & west edges, so we don't have any gaps.
  if KEYWORD_SET(sphere) then begin
    if ((xmax - xmin + 2*dx) ge 360) then begin
      if ((xmin - dx) le -180) then xmin = xmin < (-180)
      if ((xmin - dx) le 0) then xmin = xmin < 0
      if ((xmax + dx) ge 360) then xmax = xmax > 360
      if ((xmax + dx) ge 180) then xmax = xmax > 180
      dx = (xmax - xmin)/(nxOut - 1)
    endif
  endif
  
  xout = DINDGEN(nxOut)*dx + xmin
  yout = DINDGEN(nyOut)*dy + ymin
  
  if (KEYWORD_SET(sphere)) then begin
    QHULL, xIn, yIn, tr, /DELAUNAY, SPHERE = s
    zout = GRIDDATA(xIn, yIn, zIn, /DEGREES, /SPHERE, TRIANGLES = tr, $
      /NATURAL_NEIGHBOR, /GRID, XOUT=xout, YOUT=yout)
  endif else begin
    QHULL, xIn, yIn, tr, /DELAUNAY
    zout = GRIDDATA(xIn, yIn, zIn, TRIANGLES = tr, $
      /NATURAL_NEIGHBOR, /GRID, XOUT=xout, YOUT=yout)
    zmin = MIN(zIn, MAX=zmax, /NAN)
    ; Check if the gridding produced some wacky values, compared to the input.
    ; If so, then try to fix them up using the slower LINEAR method.
    bad = WHERE(zout lt 0.9*zmin or zout gt 1.1*zmax, nbad)
    if (nbad gt 0) then begin
      if (FLOAT(nbad)/N_ELEMENTS(zout) gt 0.1) then $
        MESSAGE, 'Automatic gridding failed.'
      xy = ARRAY_INDICES(zout, bad)
      zout[bad] = GRIDDATA(xIn, yIn, zIn, TRIANGLES=tr, /LINEAR, $
        XOUT=xout[xy[0,*]], YOUT=yout[xy[1,*]])
    endif
  endelse
  
  zout = MIN(zIn, MAX=mx) > zout < mx
end


;-------------------------------------------------------------------------
; Determine if Z, X, Y all lie on a regular or irregular grid.
; Contour, Image, and Surface will all go through this path.
;
; GRID_UNITS: Input: 0 or '', 1 or 'm' or 'meters', 2 or 'deg' or 'degrees'
; SPHERE: Output: 0 if non-spherical grid, 1 if on a sphere
;
function Graphic_IsIrregular, zIn, xIn, yIn, $
  GRID_UNITS=gridUnits, SPHERE=sphere
  
  compile_opt idl2, hidden
  
  if (N_PARAMS() ne 3) then $
    MESSAGE, 'Incorrect number of arguments.', /NONAME
  
  zdim0 = (SIZE(zIn, /DIM))[0]
  xdim0 = (SIZE(xIn, /DIM))[0]
  ydim0 = (SIZE(yIn, /DIM))[0]

  ndimz = SIZE(zIn, /N_DIMENSIONS) - (zdim0 eq 1)
  ndimx = SIZE(xIn, /N_DIMENSIONS) - (xdim0 eq 1)
  ndimy = SIZE(yIn, /N_DIMENSIONS) - (ydim0 eq 1)

  ; Ignore true-color images.
  if (ndimz eq 3) then return, 0

  if (ndimz eq 0 || ndimz gt 2) then $
    MESSAGE, 'Input must be a vector or 2D array.', /NONAME
    
  nz = N_ELEMENTS(zIn)
  nx = N_ELEMENTS(xIn)
  ny = N_ELEMENTS(yIn)
  
  ; If Z is a vector, then X and Y must have the same # of elements as Z.
  if (ndimz eq 1 && (nx ne nz || ny ne nz)) then $
    MESSAGE, 'Arguments have invalid dimensions.', /NONAME
    
  sphere = 0b
  
  ; Do we have longitude/latitude values, in degrees?  
  if (ISA(gridUnits, "STRING")) then begin
    sphere = STRCMP(gridUnits, 'DEG', 3, /FOLD)
  endif else if ISA(gridUnits, /NUMBER) then begin
    sphere = gridUnits eq 2
  endif
  
  ; Do we have an irregular grid?
  
  ; If we have all vectors, then assume we have an irregular grid.
  if (ndimz eq 1 && ndimx eq 1 && ndimy eq 1) then return, 1b
  
  ; See if X and Y are uniformly increasing (or decreasing)
  ; in both dimensions.
  eps = 1e-2
  diff = ABS(xIn[1:*,*] - xIn[0:-2,*])
  mx = MAX(diff, MIN=mn)
  if ((mx - mn)/mx gt eps) then return, 1b

  if (ndimx eq 2) then begin
    diff = ABS(xIn[*,1:*] - xIn[*,0:-2])
    mx = MAX(diff, MIN=mn)
    if ((mx - mn)/mx gt eps) then return, 1b
  endif

  diff = ABS(yIn[1:*,*] - yIn[0:-2,*])
  mx = MAX(diff, MIN=mn)
  if ((mx - mn)/mx gt eps) then return, 1b

  if (ndimy eq 2) then begin
    diff = ABS(yIn[*,1:*] - yIn[*,0:-2])
    mx = MAX(diff, MIN=mn)
    if ((mx - mn)/mx gt eps) then return, 1b
  endif

  return, 0b
end


;-------------------------------------------------------------------------
;+
; :Description:
;    Create an idl graphic, return a handle to the graphic object.
;
; :Params:
;    toolName   : (Contour,Image,Map,Plot,Surface,Vector,Volume)
;    arg1       : optional generic argument
;    arg2       : optional generic argument
;
; :Keywords:
;    _REF_EXTRA
;
;-
pro Graphic, graphicName, arg1, arg2, arg3, arg4, $
  BUFFER=buffer, $
  CLEAN=clean, $  ; undocumented - erase the System save file
  CURRENT=currentIn, $
  DEBUG=debug, $
  DEVICE=device, $
  DIMENSIONS=dimensions, $
  ERROR_CLASS=errorClass, $
  FONT_SIZE=font_sizeIn,  $
  GRAPHIC=graphic, $
  LAYOUT=layoutIn, $
  LOCATION=location, $
  MARGIN=marginIn, $
  OVERPLOT=overplotIn, $
  POSITION=position, $
  RENDERER=renderer, $
  TEST=test, $
  TITLE=titleIn, $
  UNDOCUMENTED=undocumented, $  ; allow undocumented properties
  VERIFY_KEYWORDS=verifyKeywords, $  ; verify keywords and return
  WINDOW_TITLE=winTitle, $
  WIDGETS=widgets, $  ; undocumented - use IDL widgets for the window
  XTITLE=xtitlein, $
  YTITLE=ytitlein, $
  _REF_EXTRA=ex
  
  compile_opt idl2, hidden

@graphic_error

  toolName = 'Graphic'
  
  ; Ensure that all class definitions are available.
  Graphic__define
  
  nparams = N_PARAMS() > 1
  
  ; If we have a graphic, then create a tool of that type,
  ; otherwise just use iPlot to create a generic empty tool.
  procName = ISA(graphicName) ? 'i'+graphicName : 'iPlot'
  title = ISA(winTitle,'STRING') ? winTitle : $
    (ISA(graphicName) ? graphicName : 'Graphic')
    
  ; Check for illegal properties.
  if (ISA(ex) && ~KEYWORD_SET(undocumented)) then begin
    if (~ISA(errorClass)) then begin
      errorClass = 'Graphic'
      if (ISA(graphicName)) then begin
        if (MAX(OBJ_CLASS(graphicName, /SUPERCLASS) eq 'GRAPHIC')) then $
          errorClass = graphicName
      endif
    endif
    oClass = OBJ_NEW(errorClass)
    oClass->VerifyProperty, ex
    OBJ_DESTROY, oClass
  endif
  
  if KEYWORD_SET(verifyKeywords) then return
  
  ; Get the system object, making sure we pass in keywords.
  void = _IDLitSys_GetSystem(CLEAN=clean)
  
  ; Check for overplot situation
  if (ISA(currentIn, 'GRAPHIC')) then begin
    w = (currentIn[0]).window
    w.SetCurrent
    oTool = (currentIn[0]).GetTool()
    toolID = oTool->GetFullIdentifier()
    current = toolID
    overplot = KEYWORD_SET(overplotIn) && (toolID ne '') ? overplotIn : 0b
  endif else begin
    toolID = iGetCurrent()
    current = KEYWORD_SET(currentIn) && (toolID ne '')
    overplot = KEYWORD_SET(overplotIn) && (toolID ne '') ? overplotIn : 0b
  endelse
  
  createCanvas = ~current && ~KEYWORD_SET(overplot) && $
    ~KEYWORD_SET(buffer) && ~KEYWORD_SET(widgets)
    
  if (ISA(dimensions)) then begin
    if (N_ELEMENTS(dimensions) ne 2) then $
      MESSAGE, 'DIMENSIONS must have 2 elements.'
    dimStr = STRING(dimensions, FORMAT='(I0,",",I0)')
  endif else begin
    if (!version.OS_FAMILY eq 'Windows') then begin
      prefWidth = PREF_GET('IDL_GR_WIN_WIDTH')
      prefHeight = PREF_GET('IDL_GR_WIN_HEIGHT')
    endif else begin
      prefWidth = PREF_GET('IDL_GR_X_WIDTH')
      prefHeight = PREF_GET('IDL_GR_X_HEIGHT')
    endelse
    dimStr = STRING([prefWidth, prefHeight], FORMAT='(I0,",",I0)')
  endelse
  
  if (ISA(location)) then begin
    if (N_ELEMENTS(location) ne 2) then $
      MESSAGE, 'LOCATION must have 2 elements.'
    dimStr += STRING(location, FORMAT='(",",I0,",",I0)')
  endif else begin
    dimStr += ',-1,-1'
  endelse
  
  ; Now, request the workbench to create a canvas for us to
  ; render into.  If a workbench is not available to create
  ; the canvas, just use normal IDL widgets for the iTool container.
  ;
  wbCanvasID = createCanvas ? IDLNotify('wb_create_canvas',dimStr,'') : 0
  
  if (wbCanvasID ne 0 && wbCanvasID ne 1) then begin
    ; The workbench has created a graphics canvas.
    
    ; Create a uiAdaptor object that will be the communication
    ; mechanism between the workbench and the iTool.
    ; For Example, it gets all native GUI events
    ;  (OnEnter, OnExit, OnWheel, OnMouseMotion, OnMouseDown,
    ;   OnMouseUp, OnExpose, etc..)
    uiAdaptor = OBJ_NEW('GraphicsWin', toolName, EXTERNAL_WINDOW=wbCanvasID, $
      LAYOUT_INDEX=1, $
      RENDERER=renderer, $
      /ZOOM_ON_RESIZE)
      
    ; Attach our uiAdaptor to the wbGraphic Canvas
    uiAdaptorStr = obj_valid(uiAdaptor, /get_heap_id)
    oTool = uiAdaptor->GetTool()
    idTool = oTool->GetFullIdentifier()
    payload = STRTRIM(uiAdaptorStr,2) + '::' + idTool
    void = IDLNotify('attachUIAdaptorToCanvas',wbCanvasID, payload)
    
    ; Pass the window title to the workbench.
    void = IDLNotify('IDLitThumbnail', title + '::' + idTool, '')
    
    ; CT, Feb 2011: For SWT canvas we do not want double click to bring
    ; up the property sheet, so remove the operation. We will still keep
    ; the PropertySheet UI service, just in case someone wants to
    ; programmatically bring it up.
    oTool->UnRegisterOperation, 'Edit/Properties'
    
    ; Make sure we plot into our newly-created tool
    current = idTool
  endif
  
  ; Add layout if either layout or margin is specified
  if ((N_ELEMENTS(marginIn) ne 0) || (N_ELEMENTS(layoutIn) eq 3)) then $
    layout = N_ELEMENTS(layoutIn) eq 3 ? layoutIn : [1,1,1]
    
  ; Render the graphic
  case nparams of
    1: call_procedure, procName, TEST=test, $
      _EXTRA=ex, /NO_SAVEPROMPT, BUFFER=buffer, /AUTO_DELETE, /NO_TRANSACT, $
      /NO_MENUBAR, WINDOW_TITLE=title, LAYOUT=layout, MARGIN=marginIn, $
      RENDERER=renderer, $
      DEVICE=device, POSITION=position, $
      TOOLNAME='Graphic', USER_INTERFACE='Graphic', $
      DIMENSIONS=dimensions, LOCATION=location, $
      ID_VISUALIZATION=idVisualization, $
      CURRENT=current, OVERPLOT=overplot, $
      FONT_SIZE=font_sizeIn, TITLE=titleIn, XTITLE=xtitleIn, YTITLE=ytitleIn, $
      /NO_SELECT
    2: call_procedure, procName, arg1, $
      _EXTRA=ex, /NO_SAVEPROMPT, BUFFER=buffer, /AUTO_DELETE, /NO_TRANSACT, $
      /NO_MENUBAR, WINDOW_TITLE=title, LAYOUT=layout, MARGIN=marginIn, $
      RENDERER=renderer, $
      DEVICE=device, POSITION=position, $
      TOOLNAME='Graphic', USER_INTERFACE='Graphic', $
      DIMENSIONS=dimensions, LOCATION=location, $
      ID_VISUALIZATION=idVisualization, $
      CURRENT=current, OVERPLOT=overplot, $
      FONT_SIZE=font_sizeIn, TITLE=titleIn, XTITLE=xtitleIn, YTITLE=ytitleIn, $
      /NO_SELECT
    3: call_procedure, procName, arg1, arg2, $
      _EXTRA=ex, /NO_SAVEPROMPT, BUFFER=buffer, /AUTO_DELETE, /NO_TRANSACT, $
      /NO_MENUBAR, WINDOW_TITLE=title, LAYOUT=layout, MARGIN=marginIn, $
      RENDERER=renderer, $
      DEVICE=device, POSITION=position, $
      TOOLNAME='Graphic', USER_INTERFACE='Graphic', $
      DIMENSIONS=dimensions, LOCATION=location, $
      ID_VISUALIZATION=idVisualization, $
      CURRENT=current, OVERPLOT=overplot, $
      FONT_SIZE=font_sizeIn, TITLE=titleIn, XTITLE=xtitleIn, YTITLE=ytitleIn, $
      /NO_SELECT
    4: call_procedure, procName, arg1, arg2, arg3, $
      _EXTRA=ex, /NO_SAVEPROMPT, BUFFER=buffer, /AUTO_DELETE, /NO_TRANSACT, $
      /NO_MENUBAR, WINDOW_TITLE=title, LAYOUT=layout, MARGIN=marginIn, $
      RENDERER=renderer, $
      DEVICE=device, POSITION=position, $
      TOOLNAME='Graphic', USER_INTERFACE='Graphic', $
      DIMENSIONS=dimensions, LOCATION=location, $
      ID_VISUALIZATION=idVisualization, $
      CURRENT=current, OVERPLOT=overplot, $
      FONT_SIZE=font_sizeIn, TITLE=titleIn, XTITLE=xtitleIn, YTITLE=ytitleIn, $
      /NO_SELECT
    5: call_procedure, procName, arg1, arg2, arg3, arg4, $
      _EXTRA=ex, /NO_SAVEPROMPT, BUFFER=buffer, /AUTO_DELETE, /NO_TRANSACT, $
      /NO_MENUBAR, WINDOW_TITLE=title, LAYOUT=layout, MARGIN=marginIn, $
      RENDERER=renderer, $
      DEVICE=device, POSITION=position, $
      TOOLNAME='Graphic', USER_INTERFACE='Graphic', $
      DIMENSIONS=dimensions, LOCATION=location, $
      ID_VISUALIZATION=idVisualization, $
      CURRENT=current, OVERPLOT=overplot, $
      FONT_SIZE=font_sizeIn, TITLE=titleIn, XTITLE=xtitleIn, YTITLE=ytitleIn, $
      /NO_SELECT
  endcase
  
  ; Return the created graphic object to the caller
  graphic = OBJ_NEW()
  void = iGetCurrent(TOOL=oTool)
  
  if (OBJ_VALID(oTool)) then begin
  
    ; Change the default File->SaveAs filename from "Untitled.png" to the
    ; actual graphic name, like "plot.png", "surface.png", etc.
    if (ISA(graphicName)) then $
      oTool->SetProperty, TOOL_FILENAME=STRLOWCASE(graphicName)
      
    oWin = oTool->GetCurrentWindow()
    
    if ISA(idVisualization) && strlen(idVisualization) gt 0 then begin
      ; For most graphics we are able to retrieve the identifier and we
      ; we were able to prevent the graphic from being selected at creation
      oVisualization = oTool->GetByIdentifier(idVisualization)
    endif else begin
      ; For maps we are NOT able to retrieve the identifier and we were
      ; NOT able to prevent the graphic from being selected at creation
      ; (due to the way it is created for itools)
      oVisualization = oTool->GetSelectedItems(COUNT=nSel)
    endelse
    
    oSel = oTool->GetSelectedItems(COUNT=nSel)
    for i=0,nSel-1 do begin
      oSel[i]->Select, 0, /NO_NOTIFY
      if (i eq (nSel-1)) then oWin->ClearSelections
    endfor
    
    if ISA(oVisualization) then graphic = oVisualization[0]
    if (OBJ_VALID(graphic)) then begin
      ; If the graphic is already the correct class then return it.
      if (~ISA(graphic, graphicName)) then begin
        ; Otherwise, if a class exists, use it.
        ; Otherwise, just wrap it in the generic Graphic class.
        catch, ierr1
        if (ierr1 ne 0) then begin
          graphic = OBJ_NEW('Graphic', oVisualization[0])
        endif else begin
          graphic = Graphic_GetGraphic(oVisualization[0])
        endelse
      endif
    endif else begin
      ; Return the window if no graphics were created.
      graphic = oWin
    endelse
    
    ; Reset the Undo/Redo buffer if this is the first graphic.
    if (wbCanvasID ne 0) then begin
      oBuffer = oTool->_GetCommandBuffer()
      oBuffer->ResetBuffer
      oTool->_SetDirty, 0b
    endif
  endif
  
end
