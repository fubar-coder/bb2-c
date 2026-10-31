; bb_blitlib-depth0-dbra (#429) - the blit routines count the planes with
; SUBQ.w #1 / DBRA on the shape's (or bitmap's) depth without a check: a
; shape of depth 0 makes TBlock blit 65536 planes through the "plane
; pointers" behind the bitmap object.  A shape that was never defined
; also has its data and cookie at address 0: the blits write the bits of
; the exception vectors to whatever addresses lie behind the bitmap
; object, different in every session (sometimes a hang, sometimes not).
; Here shape 5 stays of depth 0, but its data and cookie are a zeroed
; block of chip memory: with the cookie 0 every blit writes back what it
; reads (D = C), the 65536 blits change nothing and only take their
; time (more than a second; measured in frames with CIA-A's TOD).
; Correct: a shape of depth 0 draws nothing (no time), the program goes
; on.
; @rwe bb_blitlib
DEFTYPE .l
Function tod{}
  h=Peek.b($BFEA01)&255
  m=Peek.b($BFE901)&255
  l=Peek.b($BFE801)&255
  Function Return h*65536+m*256+l
End Function
BitMap 0,320,64,1
z=AllocMem_(64,$10002)
If z=0 Then NPrint "no memory" : End
s=Addr Shape(5)
Poke.l s+14,z
Poke.l s+18,z
NPrint "before"
t=tod{}
OwnBlitter_
TBlock 5,160,32
WaitBlit_
DisownBlitter_
t=tod{}-t
If t<10 Then NPrint "no blits" Else NPrint "65536 blits"
FreeMem_ z,64
NPrint "after"
End
