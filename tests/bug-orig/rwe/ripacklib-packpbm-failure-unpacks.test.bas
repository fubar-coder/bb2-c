; ripacklib-packpbm-failure-unpacks (#772) - PackPBM's failure path (the
; packed data would not be smaller than the source) sets d0 = -1 and has
; no RTS: it runs into UnPackPBM, which unpacks from address -1 (then 0,
; 1, ...) into dest, as far as the caller's d3 + dest allows, before -1
; is returned.  Here two different bytes (1, 2) do not pack: PackPBM
; writes the literal count 1 and the byte 1 into dest, then fails.
; Correct: -1, and dest holds what PackPBM wrote (1, 1), the bytes behind
; it unchanged ($AA).
; ripacklib needs RIAmosFuncLib (LoadIFF calls its NextBank, BLoad, ...).
DEFTYPE .l

!TestCase{packing_fails}
  m=AllocMem_(64,$10001) : d=AllocMem_(64,$10001)
  !Require{m<>0 AND d<>0,"memory"}
  For i=0 To 63 : Poke.b d+i,$AA : Next
  Poke.b m,1 : Poke.b m+1,2
  r=PackPBM(m,d,2)
  a$=""
  For i=0 To 7
    If i>0 Then a$=a$+" "
    a$=a$+Right$(Hex$(Peek.b(d+i) AND 255),2)
  Next
  FreeMem_ d,64 : FreeMem_ m,64
  !CheckEq{r,-1,"result"}
  !CheckStrEq{a$,"01 01 AA AA AA AA AA AA","dest"}
!EndTestCase
