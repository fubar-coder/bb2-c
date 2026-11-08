; compiler-incdir-quote-else (#17) - a quoted IncDir path ends at its
; closing quote, and Else behind it ends the statement as ':' does
; (blitz2.s takes the Else into the path).  CNIF/CSIF act below all
; statements (getchar): in a false block IncDir and Include are skipped
; as any other text, the path stays and nothing is included.
; Correct: the program prints else, celse and 7.
; @fixed #17
; @pass file=src/inc.bb2|@TESTDATA@/compiler-incdir-quote-next.inc
a=0
IncDir "CORPUS:src/"
If a Then IncDir "CORPUS:src/" Else NPrint "else"
CNIF 1=0
Include "missing.bb2"
IncDir "CORPUS:nosuch/"
CELSE
NPrint "celse"
CEND
XInclude "inc.bb2"
NPrint #k
End
