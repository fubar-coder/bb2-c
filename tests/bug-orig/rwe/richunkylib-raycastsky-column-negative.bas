; richunkylib-raycastsky-column-negative (#887) - RaycastSky adds the
; column to the texture as a signed word without a look at it: a negative
; column reads before the texture.  Made safe: the texture given is 320
; bytes into an allocation, so the byte before it is still the program's
; (marked $55).  A call with column 0 comes first.  The original has no
; error check area.  Here with runtime error checking.
; Correct: the error check refuses column -1: "RaycastSky column below
; 0", the byte before the texture is not copied.
; @rwe richunkylib
; @pass timeout=30
; @pass runtime_errors=on
DEFTYPE .l
ChunkyBitmap 0,320,256
c=ChunkyData(0)
m=AllocMem_(320*21,$10001)
If c=0 OR m=0 Then NPrint "no memory" : End
t=m+320
Poke.b t,$AA : Poke.b t-1,$55
RaycastSky 0,10,1,0,t
NPrint "column 0: ",Peek.b(c+10)&255
RaycastSky 0,11,1,-1,t
NPrint "column -1: ",Peek.b(c+11)&255
FreeMem_ m,320*21
End
