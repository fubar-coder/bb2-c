; screenslib-displayid-lowword (#250) - the long form of Screen writes the
; viewmode with MOVE d1,newmodes+2 only as a word into the value of the
; tag SA_DisplayID: the upper word (the monitor part of a display ID)
; stays 0 and the screen opens on the default monitor.
; Here: Screen with PAL_MONITOR_ID|HIRES_KEY ($29000), then the display ID
; of its ViewPort (graphics GetVPModeID).
; Correct: the screen has the display ID $00029000.
; @fixed #250
; @configs clib
DEFTYPE .l
Screen 0,0,0,640,256,2,$29000,"bo250",1,2
id=GetVPModeID_(Peek.l(Addr Screen(0))+44)
CloseScreen 0
NPrint "id ",Hex$(id)
End
