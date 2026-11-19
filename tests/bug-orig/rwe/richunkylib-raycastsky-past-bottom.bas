; richunkylib-raycastsky-past-bottom (#887) - RaycastSky copies height
; bytes 320 apart from the chunky data + x without a look at the chunky
; bitmap's size: a column that runs past the last row writes behind the
; data.  Made safe: the chunky bitmap is 320 x 256, and its object is
; made to say 100 rows (32000 bytes) by a Poke of the height word, so
; the row the column runs into (row 100) still lies inside the data.  A
; call that fits (100 rows) comes first.  The original has no error check
; area.  Here with runtime error checking.
; Correct: the error check refuses the 101 rows: "RaycastSky outside the
; chunky bitmap", row 100 is not written.
; @fixed #887
; @rwe richunkylib
; @pass timeout=30
; @pass runtime_errors=on
DEFTYPE .l
ChunkyBitmap 0,320,256
c=ChunkyData(0)
t=AllocMem_(320*110,$10001)
If c=0 OR t=0 Then NPrint "no memory" : End
For i=0 To 109 : Poke.b t+i*320+5,$AA : Next
o=Addr ChunkyBitmap(0)
Poke.w o+2,100
RaycastSky 0,10,100,5,t
NPrint "100 rows: row 99 ",Peek.b(c+320*99+10)&255
RaycastSky 0,10,101,5,t
NPrint "101 rows: row 100 ",Peek.b(c+320*100+10)&255
Poke.w o+2,256
FreeMem_ t,320*110
End
