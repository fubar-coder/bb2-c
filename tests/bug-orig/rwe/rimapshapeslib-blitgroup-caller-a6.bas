; rimapshapeslib-blitgroup-caller-a6 (#902) - BlitGroup blits its members
; with JMP _BltBitmapRastPort(a6) as BlitMapShape does, but its !libs has
; no #graphicslib,#la6: a6 is whatever the program's code left there, and
; the call goes to offset -606 of that "library".
; Made safe: right before BlitGroup the program loads a6 (GetReg) with the
; base of a jump table of its own, whose entry at -606 is an RTS; a6 is
; not touched by the code of BlitGroup's arguments.  The group has one
; member, a 16x16 map shape of colour 1, blitted at 20,20 of a window on
; the Workbench screen, whose pixel there is read before and after.
; Correct: BlitGroup calls graphics.library: the pixel becomes colour 1.
; @rwe rimapshapeslib
DEFTYPE .l
BitMap 0,320,64,1
Boxf 0,0,15,15,1
GetaShape 0,0,0,16,16
SetNumMapShapes 10
ShapeToMapShape 0,0
g=ResetGroup(16,16)
AddGroupMember 0,0,0
jt=AllocMem_(1024,$10001)
If jt=0 Then NPrint "no memory" : End
fb=jt+800
Poke.w fb-606,$4E75
CacheClearU_
WbToScreen 0
Window 0,20,30,200,80,$000E,"BlitGroup",1,2
DefaultOutput
rp=RastPort(0)
NPrint "pixel before ",ReadPixel_(rp,20,20)
GetReg a6,fb
BlitGroup 0,20,20
NPrint "pixel after ",ReadPixel_(rp,20,20)
FreeMem_ jt,1024
End
