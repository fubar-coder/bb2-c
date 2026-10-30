; t93_freq - freqlib FileRequest$ (the Blitz file requester, modal),
; cancelled by reqclose (compilerlauf/tedstub, run with --reqtrap
; .../reqclose; ENV:REQCLOSE says what to answer)
; Own 320x256 screen: the requester window is screen height - 40 = 216
; high (16 list lines); on screens of 645 lines or more (the emulator's
; 1050-line Workbench) the original writes its line gadgets past its
; data block and the program crashes after returning (BUG(orig) (#160), see
; libs-c/libs/freqlib.c).
If WriteFile(0,"ENV:REQCLOSE")
  FileOutput 0
  NPrint "cancel BB2 file requester 2"
  NPrint "ok BB2 file requester"
  NPrint "cancel *"
  CloseFile 0
  DefaultOutput
EndIf
Screen 0,0,0,320,256,2,0,"t93",1,2
Use Screen 0
MaxLen p$=160
MaxLen f$=64
p$="RAM:T"
f$="x"
r$=FileRequest$("BB2 file requester 2",p$,f$)
NPrint "cancel [",r$,"] ",Len(r$)," [",p$,"] [",f$,"]"
KillFile "ENV:REQCLOSE"
End
