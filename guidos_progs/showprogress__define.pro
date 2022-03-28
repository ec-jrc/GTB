;+
; NAME:
;       SHOWPROGRESS__DEFINE
;
; PURPOSE:
;
;       An object for creating a progress bar.
;
; AUTHOR:
;
;       FANNING SOFTWARE CONSULTING
;       David Fanning, Ph.D.
;       1645 Sheely Drive
;       Fort Collins, CO 80526 USA
;       Phone: 970-221-0438
;       E-mail: davidf@dfanning.com
;       Coyote's Guide to IDL Programming: http://www.dfanning.com
;
; CATEGORY:

;       Utilities
;
; CALLING SEQUENCE:
;
;       progressBar = Obj_New("SHOWPROGRESS")
;
; INPUTS:
;
;       parent: A widget identifier of the widget that will be the
;           group leader for this program. It is *required* for modal
;           operation. If missing, you are on your own. :-(
;
; KEYWORDS:
;
;      CANCELBUTTON: Set this keyword if a Cancel button is desired.
;      DELAY: The total time the widget should be on the display in AutoUpDate
;           mode. The keyword applies only to AutoUpDate mode. Default is 5 seconds.
;      STEPS: The number of steps to take in AutoUpDate mode. The keyword applies only
;           to AutoUpDate mode.
;      MESSAGE: The text of the label above the progress bar. Default is "Operation
;           in Progress...".
;      TITLE: ; The text of the top-level base title bar. Default is ""
;      COLOR: The color to draw the progress bar.
;      XSIZE: The XSize of the progress bar in Device coordinates. Default is 150.
;      YSIZE: The YSize of the progress bar in Device coordinates. Default is 10.
;      AUTOUPDATE: Set this keyword to be in AutoUpDate mode.

;
; PROCEDURE:
;       There are two modes. In AutoUpDate mode, a delay and number of steps is
;       required. The modal widget stays on the display until the total time
;       exceeds the DELAY or the requested number of steps is taken. A TIMER
;       widget is used to generate update events. Nothing can be going on
;       concurrently in AutoUpDate mode. To enter AutoUpDate mode, type this:
;
;          progressBar = Obj_New("SHOWPROGRESS", /AutoUpDate, Delay=2, Steps=10)
;          progressBar->Start
;          Obj_Destroy, progressBar
;
;       The program will update and destroy itself automatically. (The object
;       itself is not destroyed. You must do this explicitly, as in the example
;       above.)
;
;       In normal mode, the user is responsible for starting, updating, and
;       destroying the progress indicator. The sequence of commands might look
;       like this:
;
;          progressBar = Obj_New("SHOWPROGRESS")
;          progressBar->Start
;          FOR j=0,9 DO BEGIN
;             Wait, 0.5  ; Would probably be doing something ELSE here!
;             progressBar->Update, (j+1)*10
;          ENDFOR
;          progressBar->Destroy
;          Obj_Destroy, progressBar
;
;       Normal mode gives you the opportunity to update the Progress Bar
;       in a loop while something else is going on. See the example program
;       at the end of this file.
;
;       Note that the object itself is not destroyed when calling the DESTROY
;       method. You must explicitly destroy the object, as in the example above.
;
; METHODS:
;
;       CHECKCANCEL: This function method returns a 1 if the user has clicked
;           the CANCEL button. Otherwise, it returns a 0.
;
;          cancelled = progressBar->CheckCancel()
;          IF cancelled THEN progressBar->Destroy
;
;       DESTROY: Destroys the ShowProgress widgets. It does NOT destroy the object.
;
;          progressBar->Destroy
;
;       GETPROPERTY: Gets the properties that can be set in the INIT method, including
;          the parent widget ID.
;
;          progressBar->GetProperty, Steps=currentNSteps, Delay=currentDelay
;
;       SETCOLOR: Changes the color of the progress bar.
;
;           progressBar->SetColor, !P.Color
;
;       SETLABEL: Changes the text on the widget label.
;
;           progressBar->SetLabel, 'This text instead'
;
;       SETPROPERTY: Allows the user to set the INIT parameter via keywords.
;
;          progressBar->SetProperty, Color=244, XSize=200, Message='Please Wait...'
;
;       START: Puts the ShowProgress bar on the display. In AutoUpDate mode, the
;          widget starts to automatically update.
;
;          progressBar->Start
;
;       UPDATE: Updates the progress bar. Requires on argument, a number between 0
;          and 100 that indicates the percent of progress bar that should be filled
;          with a color.
;
;          progressBar->Update, 50
;
; EXAMPLE:
;
;       See the example program at the bottom of this file.
;
; RESTRICTIONS:
;
;       In contradiction to the IDL documentation, making the parent widget
;          insensitive in normal mode does NOT prevent the parent widgets from
;          receiving events on my Windows NT 4.0, SP 4 system running IDL 5.2,
;          IDL 5.2.1, or IDL 5.3 (beta).
;
;       Note that if you specify a CANCEL button the Show Progress program CANNOT
;       run as a MODAL widget program. Thus, user *may* be able to generate events
;       in the calling program while this program is running.
;
; MODIFICATION HISTORY:
;       Written by:  David Fanning, 26 July 1999.
;       Added code so that the current graphics window doesn't change. 1 September 1999. DWF.
;       Added yet more code for the same purpose. 3 September 1999. DWF.
;       Added a CANCEL button and made other minor modifications. 12 Oct 1999. DWF.
;-


FUNCTION ShowProgress::CheckCancel

; This method checks for a CANCEL Button event. It returns 1
; if an event has occurred and 0 otherwise.

RETURN, self.cancel
END; -----------------------------------------------------------------------------



PRO ShowProgress::SetLabel, newlabel

; This method allows the widget label to be changed while
; the program is on the display.

IF N_Elements(newlabel) EQ 0 THEN newlabel = ""
Widget_Control, self.labelID, Set_Value=newlabel
END; -----------------------------------------------------------------------------



PRO ShowProgress::SetCancel, value

; This method sets the Cancel flag.

IF N_Elements(value) EQ 0 THEN value = 1
self.cancel = value
END; -----------------------------------------------------------------------------



PRO ShowProgress::SetColor, color

; This method sets the Cancel flag.

IF N_Elements(color) EQ 0 THEN color = !P.Color
self.color = color
END; -----------------------------------------------------------------------------



PRO ShowProgress::Destroy

; This method takes the widget off the display.

Widget_Control, self.tlb, Destroy=1
END; -----------------------------------------------------------------------------



PRO ShowProgress::UpDate, percent

; This method updates the display. It should be called with
; manual operation. PERCENT should be a value between 0 and 100.

Catch, theError
IF theError NE 0 THEN BEGIN
    Catch, /Cancel

      ; Catch a WSET error silently.

    IF !Error_State.Code EQ -386 THEN RETURN
    Message, !Error_State.Msg, /Informational
    RETURN
ENDIF

percent = 0 > percent < 100

   ; Update the progress box.

thisWindow = !D.Window
WSet, self.wid
x1 = 0
y1 = 0
x2 = Fix(self.xsize  * (percent/100.0))
y2 = self.ysize
Polyfill, [x1, x1, x2, x2, x1], [y1, y2, y2, y1, y1], /Device, Color=self.color
IF thisWindow GE 0 AND thisWindow NE self.wid THEN WSet, thisWindow

   ; Check for a CANCEL button event.

IF Widget_Info(self.cancelID, /Valid_ID) THEN BEGIN
   event = Widget_Event(self.cancelID, /NoWait)
   name = Tag_Names(event, /Structure_Name)
   IF name EQ 'WIDGET_BUTTON' THEN self.cancel = 1
ENDIF

END; -----------------------------------------------------------------------------



PRO ShowProgress::Timer_Events, event

; This method processes the widget TIMER events.

Catch, theError
IF theError NE 0 THEN BEGIN
    Catch, /Cancel

      ; Catch a WSET error silently.

    IF !Error_State.Code EQ -386 THEN RETURN
    Message, !Error_State.Msg, /Informational
    RETURN
ENDIF

   ; Do it the specified number of times, then quit.

self.count = self.count + 1
IF self.count GE self.nsteps THEN BEGIN
   thisWindow = !D.Window
   selfWindow = self.wid
   WSet, self.wid
   Polyfill, [0, 0, self.xsize, self.xsize, 0], $
             [0, self.ysize, self.ysize, 0, 0], /Normal, Color=self.color
   Widget_Control, self.tlb, Destroy=1
   IF thisWindow GE 0 AND thisWindow NE selfWindow THEN WSet, thisWindow
   RETURN
ENDIF

   ; If time gets away from you, then quit.

theTime = Systime(1) - self.startTime
IF theTime GT self.delay THEN BEGIN
   thisWindow = !D.Window
   WSet, self.wid
   Polyfill, [0, 0, self.xsize, self.xsize, 0], $
             [0, self.ysize, self.ysize, 0, 0], /Normal, Color=self.color
   Widget_Control, self.tlb, Destroy=1
   IF thisWindow GE 0 AND thisWindow NE selfWindow THEN WSet, thisWindow
   RETURN
ENDIF

   ; Update the progress box.

thisSize = (self.xsize / self.nsteps) * self.count
thisSize = Float(thisSize) / self.xsize * 100
self->Update, thisSize

   ; Set the next timer event if the CANCEL button is not set.

IF self.cancel EQ 1 THEN BEGIN
   self->Destroy
ENDIF ELSE BEGIN
   Widget_Control, self.tlb, Timer=self.step
ENDELSE
END; -----------------------------------------------------------------------------



PRO ShowProgress_Event, event

; This is the event handler for the program. It simply calls
; the event handling method.

Widget_Control, event.top, Get_UValue=self
thisEvent = Tag_Names(event, /Structure_Name)
IF thisEvent EQ 'WIDGET_BUTTON' THEN self->SetCancel, 1 ELSE $
   self->Timer_Events, event
END; -----------------------------------------------------------------------------


PRO ShowProgress::Start

; This is the method that puts the timer on the display and gets
; things going. The initial timer event is set here.

   ; Find the window index number of any open display window.

thisWindow = !D.Window

   ; Realize the widget.

Widget_Control, self.tlb, /Realize

   ; Set an initial start time.

self.startTime = Systime(1)

   ; Get the window index number of the draw widget.

Widget_Control, self.drawID, Get_Value=wid
self.wid = wid

   ; Back to the open display window.

IF thisWindow GE 0 THEN WSet, thisWindow

IF self.autoupdate THEN BEGIN

      ; Set the first timer event.

   Widget_Control, self.tlb, Timer=self.step

      ; Register with XMANAGER so you can receive events.

   XManager, 'showprogress', self.tlb, Cleanup='ShowProgress_CleanUp'

ENDIF ELSE BEGIN

   IF Widget_Info(self.parent, /Valid_ID) THEN $
      Widget_Control, self.parent, Sensitive=0
   self->Update, 0
   Widget_Control, self.tlb, Kill_Notify='ShowProgress_CleanUp'

ENDELSE

END; -----------------------------------------------------------------------------



PRO ShowProgress_Cleanup, tlb

; This is the cleanup method for the widget. The idea
; here is to reinitialize the widget after the old
; widget has been destroyed. This is necessary, because
; it is not possible to MAP and UNMAP a modal widget.

Widget_Control, tlb, Get_UValue=self
self->ReInitialize

END; -----------------------------------------------------------------------------



PRO ShowProgress::ReInitialize

; This method just reinitializes the ShowProgress widget after the
; previous one has been destroyed. This is called from the widget's
; CLEANUP routine.

IF NOT Float(self.autoupdate) THEN BEGIN
   IF Widget_Info(self.parent, /Valid_ID) THEN $
      Widget_Control, self.parent, Sensitive=1, /Clear_Events
ENDIF

   ; Create the widgets.

IF self.parent EQ -1 THEN BEGIN
   self.tlb = Widget_Base(Title=self.title, Column=1, Base_Align_Center=1)
ENDIF ELSE BEGIN
   IF self.cancelButton THEN modal = 0 ELSE modal = 1
   self.tlb = Widget_Base(Group_Leader=self.parent, Modal=modal, Title=self.title, $
      Column=1, Base_Align_Center=1, Floating=1)
ENDELSE
self.labelID = Widget_Label(self.tlb, Value=self.message, /Dynamic_Resize)
self.drawID = Widget_Draw(self.tlb, XSize=self.xsize, YSize=self.ysize)
IF self.cancel THEN BEGIN
   self.cancelID = Widget_Button(self.tlb, Value='Cancel')
ENDIF ELSE self.cancelID = -1L

Widget_Control, self.tlb, Set_UValue=self

   ; Center the top-level base.

Device, Get_Screen_Size=screenSize
xCenter = screenSize(0) / 2
yCenter = screenSize(1) / 2

geom = Widget_Info(self.tlb, /Geometry)
xHalfSize = geom.Scr_XSize / 2
yHalfSize = geom.Scr_YSize / 2

Widget_Control, self.tlb, XOffset = xCenter-xHalfSize, $
   YOffset = yCenter-yHalfSize

   ; Reset the counter.

self.count = 0
END; -----------------------------------------------------------------------------



PRO ShowProgress::Cleanup

; This CLEANUP method is not usually required, since other widget
; programs are destroying the widget, but is here for completeness.

IF Widget_Info(self.tlb, /Valid_ID) THEN Widget_Control, self.tlb, /Destroy
END; -----------------------------------------------------------------------------



PRO ShowProgress::GetProperty, Parent=parent, Delay=delay, Steps=nsteps, $
   Message=message, Title=title, Color=color, XSize=xsize, $
   YSize=ysize, AutoUpdate=autoupdate

   ; This method allows you to get all the properties available in the INIT method.

parent = self.parent
delay = self.delay
nsteps = self.nsteps
message = self.message
title = self.title
color = self.color
xsize = self.xsize
ysize = self.ysize
autoupdate = self.autoupdate
END; -----------------------------------------------------------------------------



PRO ShowProgress::SetProperty, Parent=parent, Delay=delay, Steps=nsteps, $
   Message=message, Title=title, Color=color, XSize=xsize, $
   YSize=ysize, AutoUpdate=autoupdate

   ; This method allows you to set all the properties available in the INIT method.

IF N_Elements(parent) NE 0 THEN BEGIN
    self.parent = parent
   Widget_Control, self.tlb, /Destroy
ENDIF
IF N_Elements(delay) NE 0 THEN self.delay = delay
IF N_Elements(nsteps) NE 0 THEN BEGIN
   self.nsteps = nsteps
   self.step = (Float(self.delay) / self.nsteps)
ENDIF
IF N_Elements(message) NE 0 THEN self.message = message
IF N_Elements(title) NE 0 THEN self.title = title
IF N_Elements(color) NE 0 THEN self.color = color
IF N_Elements(xsize) NE 0 THEN self.xsize = xsize
IF N_Elements(ysize) NE 0 THEN self.ysize = ysize
IF N_Elements(autoupdate) NE 0 THEN self.autoupdate = autoupdate
Widget_Control, self.tlb, /Destroy
END; -----------------------------------------------------------------------------



FUNCTION ShowProgress::Init, $
   parent, $             ; The widget ID of the group leader.
   CancelButton=cancelButton, $ ; This keyword is set if a cancel button is required.
   Delay=delay, $        ; The total time the widget should be on the display in AutoUpDate mode.
   Steps=nsteps, $       ; The number of steps to take in AutoUpDate mode.
   Message=message, $    ; The text of the label above the progress bar.
   Title=title, $        ; The text of the top-level base title bar.
   Color=color, $        ; The color to draw the progress bar.
   XSize=xsize, $        ; The XSize of the progress bar in Device coordinates.
   YSize=ysize, $        ; The YSize of the progress bar in Device coordinates.
   AutoUpdate=autoupdate ; Set this keyword to be in AutoUpDate mode.

   ; A group leader widget (i.e., a parent parameter) is REQUIRED for MODAL operation.

   ; Check keywords.

IF N_Elements(delay) EQ 0 THEN delay = 5
IF N_Elements(nsteps) EQ 0 THEN nsteps = 10
   theStep = (Float(delay) / nsteps)
IF N_Elements(message) EQ 0 THEN message = "Operation in progress..."
IF N_Elements(title) EQ 0 THEN title = ""
IF N_Elements(color) EQ 0 THEN color = !P.Color
IF N_Elements(xsize) EQ 0 THEN xsize = 150
IF N_Elements(ysize) EQ 0 THEN ysize = 10
self.autoupdate = Keyword_Set(autoupdate)

   ; Update self structure.

self.delay = delay
self.step = theStep
self.nsteps = nsteps
self.message = message
self.title = title
self.color = color
self.xsize = xsize
self.ysize = ysize
self.count = 0
self.cancel = 0
self.cancelButton = Keyword_Set(cancelButton)

   ; Create the widgets.

IF N_Elements(parent) EQ 0 THEN BEGIN
   self.tlb = Widget_Base(Title=self.title, Column=1, Base_Align_Center=1)
   self.parent = -1L
ENDIF ELSE BEGIN
   IF self.cancelButton THEN modal = 0 ELSE modal = 1
   self.tlb = Widget_Base(Group_Leader=parent, Modal=modal, Title=self.title, $
      Column=1, Base_Align_Center=1, Floating=1)
   self.parent = parent
ENDELSE
self.labelID = Widget_Label(self.tlb, Value=self.message, /Dynamic_Resize)
self.drawID = Widget_Draw(self.tlb, XSize=self.xsize, YSize=self.ysize)
Widget_Control, self.tlb, Set_UValue=self

IF self.cancelButton THEN BEGIN
   self.cancelID = Widget_Button(self.tlb, Value='Cancel')
ENDIF ELSE self.cancelID = -1L

   ; Center the top-level base.

Device, Get_Screen_Size=screenSize
xCenter = screenSize(0) / 2
yCenter = screenSize(1) / 2

geom = Widget_Info(self.tlb, /Geometry)
xHalfSize = geom.Scr_XSize / 2
yHalfSize = geom.Scr_YSize / 2

Widget_Control, self.tlb, XOffset = xCenter-xHalfSize, $
   YOffset = yCenter-yHalfSize

RETURN, 1
END; -----------------------------------------------------------------------------


PRO ShowProgress__Define

; The SHOWPROGRESS class definition.

   struct = {SHOWPROGRESS, $    ; The SHOWPROGRESS object class.
             tlb:0L, $          ; The identifier of the top-level base.
             labelID:0L, $      ; The identifier of the label widget.
             drawID:0L, $       ; The identifier of the draw widget.
             parent:0L, $       ; The identifier of the group leader widget.
             cancelID:0L, $     ; The identifier of the CANCEL button.
             wid:0L, $          ; The window index number of the draw widget.
             xsize:0L, $        ; The XSize of the progress bar.
             ysize:0L, $        ; The YSize of the progress bar.
             color:0L, $        ; The color of the progress bar.
             autoupdate:0L, $   ; A flag for indicating if the bar should update itself.
             cancel:0L, $       ; A flag to indicate the CANCEL button was clicked.
             cancelButton:0L, $ ; A flag to indicate a CANCEL button should be added.
             message:'', $      ; The message to be written over the progress bar.
             title:'', $        ; The title of the top-level base widget.
             count:0L, $        ; The number of times the progress bar has been updated.
             startTime:0D, $    ; The time when the widget is started.
             delay:0L, $        ; The total time the widget is on the display.
             nsteps:0L, $       ; The number of steps you want to take.
             step:0.0 $         ; The time delay between steps.
             }

END; -----------------------------------------------------------------------------



PRO Example_Event, event

; Respond to program button events.

Widget_Control, event.id, Get_Value=buttonValue, Get_UValue=timer

CASE buttonValue OF

   'Automatic Mode':timer->start

   'Manual Mode': BEGIN ; Updating of Show Progress widget occurs in loop.

      timer->start
      count = 0
      FOR j=0, 1000 DO BEGIN
          if j mod 100 EQ 0 THEN BEGIN
            cancelled = timer->CheckCancel()
            IF cancelled THEN BEGIN
               ok = Dialog_Message('User cancelled operation.')
               timer->Destroy
               RETURN
            ENDIF
            timer->Update, (count * 10.0)
            count = count + 1
          endif
          Wait, 0.01 ; This is where you would do something useful.
      ENDFOR
      timer->destroy
      ENDCASE

   'Quit': Widget_Control, event.top, /Destroy

ENDCASE

END


PRO Example_Cleanup, tlb

   ; Cleanup routine when TLB widget dies. Be sure
   ; to destroy Show Progress objects.

Widget_Control, tlb, Get_UValue=info
Obj_Destroy, info[0]
Obj_Destroy, info[1]
END


PRO Example
Device, Decomposed=0
TVLCT, 255, 0, 0, 20
tlb = Widget_Base(Column=1, Xoffset=200, Yoffset=200)

   ; Create an AutoUpDate object. Store in UValue of Button.

autoTimer = Obj_New("ShowProgress", tlb, Color=20, Steps=20, Delay=5, /AutoUpdate)
button = Widget_Button(tlb, Value='Automatic Mode', UValue=autoTimer)

   ; Create a Manual Show Progress object. Store in UValue of Button.

progressTimer = Obj_New("ShowProgress", tlb, Color=20, /CancelButton)
button = Widget_Button(tlb, Value='Manual Mode', UValue=progressTimer)

quiter = Widget_Button(tlb, Value='Quit', UValue='QUIT')

Widget_Control, tlb, /Realize, Set_UValue=[autoTimer, progressTimer]
XManager, 'example', tlb, /No_Block, Cleanup='example_cleanup'
END