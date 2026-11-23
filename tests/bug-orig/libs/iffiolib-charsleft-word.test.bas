; iffiolib-charsleft-word (#177) - readbuff keeps only the low word of the
; dos Read result in charsleft: a read error (-1) counts as 65535 bytes
; read, and the old contents of the 2048 byte buffer are taken as the
; file's data.
; Here: LoadShape of an ILBM (16x4, 1 plane, BODY bytes 1..8) built in
; memory and written to CORPUS:: fills the buffer; then LoadShape of PRT:,
; where Read returns -1 (port-handler, no printer).  The original loads
; the old ILBM from the buffer as shape 1 and the program goes on.
; Correct: the read error is an error (or the end of the file): there is
; no ILBM, LoadShape stops with a runtime error.
DEFTYPE .l
Statement pb{v.l}
  SHARED m,p
  Poke.b m+p,v : p=p+1
End Statement
Statement pw{v.l}
  pb{(v LSR 8) AND 255} : pb{v AND 255}
End Statement
Statement pl{v.l}
  pw{(v LSR 16) AND $FFFF} : pw{v AND $FFFF}
End Statement

!TestCase{read_error}
  m=AllocMem(256,65536) : p=0
  pl{$464F524D} : pl{0} : pl{$494C424D}
  pl{$424D4844} : pl{20}
  pw{16} : pw{4} : pw{0} : pw{0} : pb{1} : pb{0} : pb{0} : pb{0}
  pw{0} : pb{10} : pb{11} : pw{320} : pw{200}
  pl{$424F4459} : pl{8}
  For i=1 To 8 : pb{i} : Next
  Poke.l m+4,p-8
  ok=WriteFile(0,"CORPUS:bo177.iff")
  !Require{ok,"file_written"}
  WriteMem 0,m,p
  CloseFile 0
  FreeMem m,256
  LoadShape 0,"CORPUS:bo177.iff"
  a=Addr Shape(0)
  !CheckEq{Peek.w(a),16,"shape0_width"}
  !CheckEq{Peek.w(a+2),4,"shape0_height"}
  !CheckEq{Peek.b(Peek.l(a+14)) AND 255,1,"shape0_data"}
  KillFile "CORPUS:bo177.iff"
  LoadShape 1,"PRT:"
  !Check{0,"not_refused"}
!EndTestCase
