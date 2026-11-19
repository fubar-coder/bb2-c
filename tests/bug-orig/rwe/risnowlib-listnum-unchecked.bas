; risnowlib-listnum-unchecked (#913) - the commands find the snow field
; by listnum * 16 (ASL.w #4) in the table of 4 x 16 bytes (listpointers)
; without a range check: InitSnowDrop with listnum 4 writes its four
; list pointers into the 16 bytes behind the table (the address of the
; table is InitSnowDrop's result).  What lies there is code of the
; program (in the port the next library's, used by Peek): exec's CopyMem
; saves the 16 bytes before the call, copies them after it and restores
; them before anything else runs.  listnum is an argument the programmer
; controls; the original has no error check area.  Here with runtime
; error checking.
; Correct: the error check refuses listnum 4: "Snow list number must be
; 0 to 3", nothing is written behind the table.
; @rwe risnowlib
; @pass runtime_errors=on
DEFTYPE .l
BitMap 0,320,256,3
s=AllocMem_(32,$10001)
If s=0 Then NPrint "no memory" : End
lt=InitSnowDrop(0,10,0,10240)
CopyMem_ lt+64,s,16
NPrint "before"
x=InitSnowDrop(4,10,0,10240)
CopyMem_ lt+64,s+16,16
CopyMem_ s,lt+64,16
k=0
For i=0 To 12 Step 4
  If Peek.l(s+i)<>Peek.l(s+16+i) Then k+1
Next
FreeMem_ s,32
If k=0 Then NPrint "behind the table kept" Else NPrint "behind the table overwritten"
End
