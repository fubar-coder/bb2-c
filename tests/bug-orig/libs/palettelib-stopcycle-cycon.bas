; palettelib-stopcycle-cycon (#228) - Cycle installs the cycling
; interrupt only while the flag cycon is 0 and then sets it; StopCycle
; removes the interrupt but leaves cycon set, so a later Cycle installs
; nothing and the colours no longer cycle.
; Here: a screen, a palette whose colours 1-3 cycle one step per frame;
; Cycle, StopCycle, Cycle; after each command colour 1 of the screen
; (Red, Green, Blue) is watched for 8 frames: changed at all or not
; (only that, the frame of a step is timing).
; Correct: colour 1 changes after both Cycle commands and stays after
; StopCycle.
; @fixed #228
; @configs clib
; @pass timeout=120
DEFTYPE .l
Screen 0,0,0,320,200,3,0,"cycle",1,2
PalRGB 0,0,0,0,0 : PalRGB 0,1,15,0,0 : PalRGB 0,2,0,15,0 : PalRGB 0,3,0,0,15
SetCycle 0,0,1,3,1
ShowPalette 0
Cycle 0 : Gosub watch : NPrint "first Cycle: changed ",ch
StopCycle : VWait : Gosub watch : NPrint "StopCycle: changed ",ch
Cycle 0 : Gosub watch : NPrint "second Cycle: changed ",ch
StopCycle
CloseScreen 0
End

watch:
  c0=Red(1)*256+Green(1)*16+Blue(1) : ch=0
  For k=1 To 8
    VWait
    If Red(1)*256+Green(1)*16+Blue(1)<>c0 Then ch=1
  Next
Return
