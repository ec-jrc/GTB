; Copyright (c) 1993-2015, Exelis Visual Information Solutions, Inc. All
;       rights reserved. Unauthorized reproduction is prohibited.

; **********************************************************************
;
; Description:
;   This pro code example allows you to connect to an OGC WCS server
;     and download a file.
;   Invocation:
;

; MODIFICATION HISTORY:
;   LFG, RSI, June. 2007  Original version.

;;----------------------------------------------------------------------------
pro idlneturl_widget_ListClick_event, ev
  compile_opt idl2, hidden

  catch, errorStatus            ; catch all errors and display an error dialog
  if (errorStatus ne 0) then begin
    catch,/cancel
    print, !error_state.msg
    r = dialog_message(!error_state.msg, title='File List Error', dialog_parent=ev.top, /error)
    return
  endif

end

;;----------------------------------------------------------------------------
pro idlneturl_widget_btnBrowse_event, ev
  compile_opt idl2, hidden

  catch, errorStatus            ; catch all errors and display an error dialog
  if (errorStatus ne 0) then begin
    catch,/cancel
    print, !error_state.msg
    r = dialog_message(!error_state.msg, title='Browse Error', dialog_parent=ev.top, /error)
    return
  endif

  ; Get the path already placed in the download text box
  widget_control, ev.top, get_uvalue = sState
  widget_control, sState.wDownloadDir, get_value = filePath

  ; Display a file browse dialog
  filePath = dialog_pickfile(dialog_parent=ev.top, path = filePath, /directory)

  ; Paste the browse dialog selection into the download dir text box
  widget_control, sState.wDownloadDir, set_value = filePath

end

;;----------------------------------------------------------------------------
pro idlneturl_widget_btnDownload_event, ev
  compile_opt idl2, hidden

  catch, errorStatus            ; catch all errors and display an error dialog
  if (errorStatus ne 0) then begin
    catch,/cancel
    print, !error_state.msg
    r = dialog_message(!error_state.msg, title='Download Error', dialog_parent=ev.top, /error)
    return
  endif

  widget_control, ev.top, get_uvalue = sState

  ; Retrieve the URL from the URL text widget
  widget_control, sState.wTxtServer, get_value = sUrl

  ; Retrieve the local save directory from the download text widget
  widget_control, sState.wDownloadDir, get_value = localPath

  ; Retrieve the index of the selected remote file from the server list widget
  fileArrayIndex = widget_info(sState.wServerFile, /LIST_SELECT)

  ; If no file is selected, display an error message
  if (fileArrayIndex eq -1) then begin
    r = dialog_message('Please select a file to download.', $
    title='File Not Selected', /center, dialog_parent=ev.top, /information)
    return
  endif

  ; Pull the remote file name from the file array stored in the list widget
  widget_control, sState.wServerFile, get_uvalue = fileArray
  remoteFileName = fileArray[fileArrayIndex]

  localFullPath = localPath[0] + remoteFileName

  ; Create a new url object
  oUrl = OBJ_NEW('IDLnetUrl')

  ; This is an htp transaction
  oUrl->SetProperty, URL_SCHEME = 'http'

  ; Use the http server string entered in the text box
  oUrl->SetProperty, URL_HOSTNAME = sUrl[0]

  ; name of remote server directory
  oUrl->SetProperty, URL_PATH = '/guidos/GuidosToolbox/' + remoteFileName

  ; Set the appropriate username and password
  oUrl->SetProperty, URL_USERNAME = ''
  oUrl->SetProperty, URL_PASSWORD = ''

  ; Copy the selected remote file into the specified local directory
  retrievedFilePath = oUrl->Get(FILENAME=localFullPath)

  ; Close the connection to the remote HTTP server, and destroy the object
  oUrl->CloseConnections
  OBJ_DESTROY, oUrl

  ; Display a "success!" pop-up message
  r = dialog_message('Download Complete', title='Download', /center, $
    dialog_parent=ev.top, /information)

end

;;----------------------------------------------------------------------------
pro idlneturl_widget_btnClose_event, ev
  compile_opt idl2, hidden
  on_error, 2

  ;; make the ui go away
  widget_control, ev.top, /destroy
end

;;----------------------------------------------------------------------------
pro idlneturl_widget_btnConnect_event, ev

  compile_opt idl2, hidden

  catch, errorStatus            ; catch all errors and display an error dialog
  if (errorStatus ne 0) then begin
    catch,/cancel
    print, !error_state.msg
    r = dialog_message(!error_state.msg, title='Connect Error', dialog_parent=ev.top, /error)
    return
  endif

  ; Get the URL string entered into the URL text box
  widget_control, ev.top, get_uvalue = sState
  widget_control, sState.wTxtServer, get_value = sUrl

  ; If no string is entered, generate an error
  if (sUrl eq '') then begin
    r = dialog_message('Enter Server URL before pressing Connect.', title='URL Error', dialog_parent=ev.top, /error)
    return
  endif

  ; From here on out, create an IDLnetURL object, connect to the
  ; HTTP server, and display the directory in the list box

  ; create a new url object
  oUrl = OBJ_NEW('IDLnetUrl')

  ; This is an http transaction
  oUrl->SetProperty, URL_SCHEME = 'http'

  ; Use the http server string entered in the text box
  oUrl->SetProperty, URL_HOSTNAME = sUrl[0]

  ; name of dir to get directory listing for on the $
  ; remote http server
  ; This default path is valid on the HTTP server
  oUrl->SetProperty, URL_PATH = '/guidos/GuidosToolbox/'

  ; Set the appropriate username and password
  oUrl->SetProperty, URL_USERNAME = ''
  oUrl->SetProperty, URL_PASSWORD = ''

  ; Request the dir listing from the http server
  ;dirList = oUrl->GetFtpDirList(/SHORT)
  dirlist = 'GWS2015.04.zip'
  ; Save the directory list as an array in the widget uvalue
  ; field. This array will be consulted later to select a file
  ; name, when given an index into the array
  widget_control, sState.wServerFile, set_uvalue = dirList

  ; Display the dir listing in the list box
  widget_control, sState.wServerFile, set_value = dirList

  ; we are done so we release the url object
  OBJ_DESTROY, oUrl
 end

;;----------------------------------------------------------------------------

pro idlneturl_widget
  compile_opt idl2
  on_error, 2

  textWidth = 60
  textHeight = 10

  ; This URL points to the HTTP server, which contains several test
  ; files for download.
  defaultURL = 'geohub.jrc.ec.europa.eu'
  tmpIDLDir = GETENV('IDL_TMPDIR')

  ; the errors caught in the compound widget's main init routine bubble up to this level.
  ; if there is an error it is displayed and this dialog exits

  catch, errorStatus
  if (errorStatus ne 0) then begin
    catch,/cancel
    r = dialog_message(!error_state.msg, title='IDL HTTP Error', dialog_parent=rwBase, /error)
    return
  endif

  ; root level container widget for this ui
  rwBase = widget_base(row=7, /ALIGN_LEFT, TITLE='IDL HTTP Server Downloader', uname='ui_http_downloader', tlb_frame_attr=1)
  
  ; Label for server URL input text box widget
  wTxtServerLabel = widget_label(rwBase, value='HTTP Server URL', uname='server_url_label')  
  
  ; Container widget for the URL input text box and Connect button widgets
  wBaseURL =  widget_base(rwBase, column=2, uname='url_container')
  
  ; URL input text box widget
  wTxtServer = widget_text(wBaseURL, /EDITABLE, uname='server_url', value = defaultURL, xsize = textWidth)
  ; Connect button widget
  wConnect = widget_button(wBaseURL, value='Connect', uname='Connect', event_pro = 'idlneturl_widget_btnConnect_event')
  
  ; Label for server file list widget
  wServerFileLabel = widget_label(rwBase, value='Server Files', uname='server_files_label')
  
  ; Server file list widget
  wServerFile = widget_list(rwBase, uname='server_list', xsize = textWidth, ysize = textHeight, event_pro = 'idlneturl_widget_ListClick_event')
  
  ; Label for download directory widget
  wDownloadDirLabel = widget_label(rwBase, value='Download Directory', uname='download_dir_label')
  
  ;Container widget for directory and browse widgets
  wBaseDirectory =  widget_base(rwBase, column=2, /ALIGN_LEFT, uname='directory_container')
  
  ; Download directory and browse button for the wBaseDirectory base widget
  wDownloadDir = widget_text(wBaseDirectory, /EDITABLE, uname='download_dir', value = tmpIDLDir, xsize = textWidth)
  
  wBrowse = widget_button(wBaseDirectory, value='Browse', uname='browse', event_pro = 'idlneturl_widget_btnBrowse_event')
  
  ; Container widget for the Download and Quit buttons
  wBaseDownload =  widget_base(rwBase, column=2, uname='download_container')
  
  ; Download and quit button widgets
  wDownload = widget_button(wBaseDownload, value='Download', uname='Download', event_pro = 'idlneturl_widget_btnDownload_event')
  wQuit = widget_button(wBaseDownload, value='Quit', uname='Quit', event_pro = 'idlneturl_widget_btnClose_event')
  
  ; Create a state structure that holds all the widget IDs
  ; This structure will be stored in the top-level widget
  sState = { $
    wTxtServerLabel:wTxtServerLabel, $
    wTxtServer:wTxtServer, $
    wConnect:wConnect, $
    wServerFileLabel:wServerFileLabel, $
    wServerFile:wServerFile, $
    wDownloadDirLabel:wDownloadDirLabel, $
    wBaseDirectory:wBaseDirectory, $
    wDownloadDir:wDownloadDir, $
    wBrowse:wBrowse, $
    wBaseDownload:wBaseDownload, $
    wDownload:wDownload, $
    wQuit:wQuit $
    }
    
  ; Add the state structure to the top-level widget
  widget_control, rwBase, set_uvalue = sState
    
  ; draw the ui
  widget_control, rwBase, /real
  

  ;; The XMANAGER procedure provides the main event loop and
  ;; management for widgets created using IDL.  Calling XMANAGER
  ;; "registers" a widget program with the XMANAGER event handler,
  ;; XMANAGER takes control of event processing until all widgets have
  ;; been destroyed.

  ;; NO BLOCK needs to be set to 0 in order for the build query events to fire
  
  XMANAGER,'idlneturl_widget', rwBase, GROUP_LEADER=rwBase, NO_BLOCK=0
end