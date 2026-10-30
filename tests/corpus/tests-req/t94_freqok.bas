; t94_freqok - freqlib FileRequest$ answered with OK by reqclose (click
; on its OK gadget); path$/file$ get the new contents but keep their old
; lengths (BUG(orig) (#153)).
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
Screen 0,0,0,320,256,2,0,"t94",1,2
Use Screen 0
MaxLen p$=160
MaxLen f$=64
p$="RAM:"
f$="bb2test.txt"
r$=FileRequest$("BB2 file requester",p$,f$)
NPrint "ok ",Len(r$)
NPrint "[",r$,"]"
NPrint "[",p$,"] [",f$,"]"
KillFile "ENV:REQCLOSE"
End
