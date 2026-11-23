; compiler-incdir-quote-next (#17) - blitz2.s takes the rest of the line
; after IncDir, Include, XInclude and IncBin into the path or name, also
; after the closing quote: a ':' there does not start the next statement.
; Correct: after a quoted IncDir path or IncBin name a ':' starts the
; next statement, after a quoted XInclude name a comment may follow: the
; statement behind IncDir runs, #k is 7, the first byte of the binary is 1
; and the Dc.b behind IncBin is 9 (the binary is 4 bytes long).
!TestCase{incdir_next}
  s1.l=0
  IncDir "CORPUS:src/" : s1=1
  XInclude "inc.bb2" ; the included file sets #k
  v.l=#k
  b0.l=Peek.b(?dat)
  b4.l=Peek.b(?dat+4)
  !CheckEq{s1,1,"incdir_next_statement"}
  !CheckEq{v,7,"constant"}
  !CheckEq{b0,1,"incbin_first"}
  !CheckEq{b4,9,"dc_after_incbin"}
  Goto pastdat
.dat
  IncBin "dat.bin" : Dc.b 9
  Even
.pastdat
!EndTestCase
