; compiler-incdir-quote-next (#17) - blitz2.s takes the rest of the line
; after IncDir, Include, XInclude and IncBin into the path or name, also
; after the closing quote: a ':' there does not start the next statement.
; Correct: after a quoted IncDir path or IncBin name a ':' starts the
; next statement, after a quoted XInclude name a comment may follow; the
; program prints incdir, 7, 1 and 9.
; @fixed #17
; @pass file=src/inc.bb2|@TESTDATA@/compiler-incdir-quote-next.inc
; @pass file=src/dat.bin|@TESTDATA@/compiler-incdir-quote-next.bin
IncDir "CORPUS:src/" : NPrint "incdir"
XInclude "inc.bb2" ; the included file sets #k
NPrint #k
NPrint Peek.b(?dat)
NPrint Peek.b(?dat+4)
End
dat: IncBin "dat.bin" : Dc.b 9
