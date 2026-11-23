; rilesdebuglib-lowmem-signed (#717) - the commands that show memory test
; "an address below $420" with a signed compare (CMP.l / BLT): every
; address from $80000000 counts as low memory.  GetDataItem gives 0 there
; and D_HexWord writes "****".  On a machine with 24 address lines (the
; A500) $80000004 is the long at 4 (ExecBase) and $FFFFFFF0 lies in the
; Kickstart ROM.
; Correct: GetDataItem($80000004,4) is the long at 4; D_HexWord shows the
; hex digits of the word at $FFFFFFF0.
DEFTYPE .l

!TestCase{high_addresses}
  e4=Peek.l(4)
  g=GetDataItem($80000004,4)
  b=AllocMem_(64,$10001)
  !Require{b<>0,"allocated"}
  D_HexWord $FFFFFFF0,b,1
  c0=Peek.b(b)
  FreeMem_ b,64
  !CheckEq{g,e4,"long_read"}
  !Check{c0<>42,"hex_digits"}
!EndTestCase
