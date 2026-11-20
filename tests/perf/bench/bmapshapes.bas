; bmapshapes - perf benchmark: the RWE userlib rimapshapeslib, whose
; BlitMapShape is a map editor's call per tile.  BlitMapShape of a 16 x 16
; and a 32 x 32 map shape of 2 planes, BlitMapShapeSize of 16 x 8 of the
; 16 x 16 one, and BlitGroup of a group of 4 16 x 16 members, all into a
; window on the Workbench screen; the value per call.
; Blitz mode owns the blitter, and graphics.library's BltBitMapRastPort
; waits for it: the blitter is given back (DisownBlitter_) for the loops.
; @rwe rimapshapeslib
DEFTYPE .l
Dim t(5)
BitMap 0,320,64,2
Boxf 0,0,15,15,1
Boxf 16,0,47,31,2
Boxf 20,4,40,24,3
GetaShape 0,0,0,16,16
GetaShape 1,16,0,32,32
SetNumMapShapes 10
ShapeToMapShape 0,0
ShapeToMapShape 1,1
g=ResetGroup(16,16)
AddGroupMember 0,0,0
AddGroupMember 0,1,0
AddGroupMember 0,0,1
AddGroupMember 0,1,1
WbToScreen 0
Window 0,0,20,320,100,$000E,"bmapshapes",1,2
DefaultOutput
VWait 5
BLITZ
DisownBlitter_
Poke.b $bfde00,0 : Poke.b $bfdf00,0
Poke.b $bfd400,99 : Poke.b $bfd500,0
Poke.b $bfd600,$ff : Poke.b $bfd700,$ff
Poke.b $bfdf00,$51 : Poke.b $bfde00,$11
VWait 2
Gosub tod : t0=tt
For i=1 To 20000
Next
Gosub tod : t(0)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 150000
Next
Gosub tod : t(1)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
BlitMapShape 0,0,180,20
Next
Gosub tod : t(2)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
BlitMapShape 0,1,200,20
Next
Gosub tod : t(3)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
BlitMapShapeSize 0,0,240,20,16,8
Next
Gosub tod : t(4)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
BlitGroup 0,160,20
Next
Gosub tod : t(5)=(tt-t0)&65535
VWait 2
OwnBlitter_
AMIGA
NPrint "empty ",t(0)," 20000 1"
NPrint "emptylong ",t(1)," 150000 1"
NPrint "blitms16 ",t(2)," 2000 1"
NPrint "blitms32 ",t(3)," 2000 1"
NPrint "blitsize ",t(4)," 2000 1"
NPrint "blitgroup4 ",t(5)," 2000 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return
