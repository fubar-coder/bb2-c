; rilesdebuglib-gethexadr-odd (#1168) - GetHexAdr$ reads the word or long
; at the address with one MOVE.w or MOVE.l: at an odd address an address
; error on a 68000 (the session ends in a timeout).
; Correct: the word and the long at an odd address in hex, read byte by
; byte as GetDataItem does.
DEFTYPE .l

!TestCase{odd_address}
  b=AllocMem_(16,$10001)
  !Require{b<>0,"allocated"}
  Poke.b b+1,$11 : Poke.b b+2,$22 : Poke.b b+3,$33 : Poke.b b+4,$44
  w$=GetHexAdr$(b+1,2)
  l$=GetHexAdr$(b+1,3)
  FreeMem_ b,16
  !CheckStrEq{w$,"1122","word"}
  !CheckStrEq{l$,"11223344","long"}
!EndTestCase
