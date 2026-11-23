; ripacklib-unpackpbm-limits (#778) - UnPackPBM tests its limits after
; the access: one byte at dest + destlength is written.  The packed data
; (4 bytes: a run of 128 and a run of 72 bytes of $58) unpack to 200
; bytes, into a buffer of destlength 100 (filled with $AA).
; Correct: the bytes 0-99 are $58, the byte at 100 stays $AA.
; ripacklib needs RIAmosFuncLib (LoadIFF calls its NextBank, BLoad, ...).
DEFTYPE .l

!TestCase{destlength_100}
  m=AllocMem_(64,$10001) : d=AllocMem_(256,$10001)
  !Require{m<>0 AND d<>0,"memory"}
  For i=0 To 255 : Poke.b d+i,$AA : Next
  Poke.b m,$81 : Poke.b m+1,$58 : Poke.b m+2,$B9 : Poke.b m+3,$58
  UnPackPBM m,d,4,100
  a=Peek.b(d) AND 255 : b=Peek.b(d+99) AND 255 : c=Peek.b(d+100) AND 255
  FreeMem_ d,256 : FreeMem_ m,64
  !CheckEq{a,$58,"byte_0"}
  !CheckEq{b,$58,"byte_99"}
  !CheckEq{c,$AA,"byte_100"}
!EndTestCase
