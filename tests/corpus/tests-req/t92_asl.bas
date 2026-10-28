; t92_asl - asllib ASL requesters (modal), answered by reqclose
; (compilerlauf/tedstub, run with --reqtrap .../reqclose): the lines in
; ENV:REQCLOSE say ok/cancel per window title; results and variables
If WriteFile(0,"ENV:REQCLOSE")
  FileOutput 0
  NPrint "ok BB2 ASL ok"
  NPrint "cancel BB2 ASL*"
  NPrint "cancel *"
  CloseFile 0
  DefaultOutput
EndIf
WbToScreen 0
MaxLen p$=160
MaxLen f$=64
p$="RAM:"
f$="file.txt"
r$=ASLFileRequest$("BB2 ASL",p$,f$)
NPrint "file [",r$,"] ",Len(r$)," [",p$,"] [",f$,"]"
r$=ASLFileRequest$("BB2 ASL pattern",p$,f$,"#?.bb2")
NPrint "pattern [",r$,"] ",Len(r$)
r$=ASLPathRequest$("BB2 ASL path",p$,20,20,300,200)
NPrint "path [",r$,"] ",Len(r$)," [",p$,"]"
a.l=ASLFontRequest(1)
NPrint "font ",a
a.l=ASLScreenRequest(3)
NPrint "screen ",a
r$=ASLFileRequest$("BB2 ASL ok",p$,f$)
NPrint "ok [",r$,"] ",Len(r$)," [",p$,"] ",Len(p$)," [",f$,"] ",Len(f$)
KillFile "ENV:REQCLOSE"
End
