; richunkylib-raycastsky-x-negative (#887) - RaycastSky adds x to the
; chunky data as a signed word without a look at it: a negative x writes
; before the data.  Made safe: x -1 with a height of 1 writes one byte,
; the low byte of the length long of memlib's block header in front of
; the data; the program saves it before the call and puts it back after
; it.  A call with x 0 comes first.  The original has no error check
; area.  Here with runtime error checking.
; Correct: the error check refuses x -1: "RaycastSky outside the chunky
; bitmap", the byte before the data is not written.
; @rwe richunkylib
; @pass timeout=30
; @pass runtime_errors=on
DEFTYPE .l
ChunkyBitmap 0,320,256
c=ChunkyData(0)
t=AllocMem_(320*20,$10001)
If c=0 OR t=0 Then NPrint "no memory" : End
For i=0 To 19 : Poke.b t+i*320+5,$AA : Next
RaycastSky 0,0,10,5,t
NPrint "x 0: ",Peek.b(c)&255
b=Peek.b(c-1)
RaycastSky 0,-1,1,5,t
w=Peek.b(c-1)
Poke.b c-1,b
If w=b Then NPrint "x -1: the byte before the data kept" Else NPrint "x -1: the byte before the data written"
FreeMem_ t,320*20
End
