; ridisplaylib-allocmem-unchecked (#888) - InitView does not test the
; result of AllocMem: when it fails, the eight words of the copper list
; go to addresses 0-15 (over ExecBase at 4), and finit later calls
; FreeMem(0,size).  The size is computed as a word: a height of 4096
; makes it negative, so the allocation always fails.  Made safe: the 16
; bytes at 0 are read before and after the call with interrupts disabled
; and restored at once.
; Correct: InitView returns 0 and writes nothing; nothing is freed at the
; end.
; @rwe ridisplaylib
DEFTYPE .l
Dim o.l(3),n.l(3)
Disable_
For i=0 To 3 : o(i)=Peek.l(i*4) : Next
r=InitView(4096,8,8,256)
For i=0 To 3 : n(i)=Peek.l(i*4) : Poke.l i*4,o(i) : Next
Enable_
k=0
For i=0 To 3
  If n(i)<>o(i) Then k=1
Next
If k Then NPrint "low memory written" Else NPrint "low memory kept"
NPrint r
End
