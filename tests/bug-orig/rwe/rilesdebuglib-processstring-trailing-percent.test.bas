; rilesdebuglib-processstring-trailing-percent (#720) - ProcessString
; takes the character after a '%' as the entry name; for a '%' at the
; end that is the terminating 0, which matches no entry, and the routine
; goes on behind it.  The parser's buffer first holds "ABCDEFGH", then
; "100%": behind its 0 follow "FGH" of the longer text.
; Correct: ProcessString stops at the end of the text.
DEFTYPE .l

!TestCase{percent_at_end}
  b=AllocMem_(64,$10001)
  !Require{b<>0,"allocated"}
  Poke$ b,"ABCDEFGH"
  D_InitParse b
  Poke$ b,"100%"
  D_InitParse b
  a$=ProcessString
  FreeMem_ b,64
  k=Instr(a$,"FGH")
  !CheckEq{k,0,"read_behind_end"}
!EndTestCase
