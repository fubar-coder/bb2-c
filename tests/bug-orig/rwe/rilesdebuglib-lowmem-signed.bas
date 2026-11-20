; rilesdebuglib-lowmem-signed (#717) - the commands that show memory test
; "an address below $420" with a signed compare (CMP.l / BLT): every
; address from $80000000 counts as low memory.  GetDataItem gives 0 there
; and D_HexWord writes "****".  On a machine with 24 address lines (the
; A500) $80000004 is the long at 4 (ExecBase) and $FFFFFFF0 lies in the
; Kickstart ROM.
; Correct: GetDataItem($80000004,4) is the long at 4; D_HexWord shows the
; hex digits of the word at $FFFFFFF0.
; @fixed #717
; @rwe rilesdebuglib
; @machine a500
DEFTYPE .l
If GetDataItem($80000004,4)=Peek.l(4) Then NPrint "long read" Else NPrint "taken as low memory"
b=AllocMem_(64,$10001)
If b=0 Then NPrint "no memory" : End
D_HexWord $FFFFFFF0,b,1
If Peek.b(b)=42 Then NPrint "stars" Else NPrint "digits"
FreeMem_ b,64
End
