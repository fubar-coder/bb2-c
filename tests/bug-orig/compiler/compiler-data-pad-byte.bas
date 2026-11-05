; compiler-data-pad-byte (#969) - a string literal (litstring, data1) and
; a Data$ string (dodata, data2) of odd length are followed by a pad byte
; to the next even address; the original only moves the pointer, the byte
; is never written. With Compile/Run (Make Smallest Code off, the default)
; the object buffer is reused by every compile of the session: the pad
; byte is what the previous compile left at that offset. tedstub runs a
; filler first (a literal of 16 'E's in data1, Data.w $EEEE in data2),
; then this program; it finds both copies of "JKL" (the Data$ entry
; through Restore/Read, the literal by a search below data2) and prints
; the byte behind each. (Create Executable compiles into a cleared
; buffer: there the pads are 0.) Every loadxtra allocates the buffer anew
; (cleared), so no job may load options: TED loads them only with a
; source, Compile/Run after Compile/Run does not. The directory at the
; path of tedstub's defaults makes its savextra fail - no defaults, every
; job compiles with the options blitz2 started with ("XTRA none"; runtime
; error checking on: a Data entry begins with its type word, the pad of
; "JKL" is 9 bytes behind the start of data2).
; Correct: both pad bytes are 0.
; @pass dir=out/defaults.xtra
; @pass file=src/filler.bb2|@TESTDATA@/compiler-data-pad-byte.filler
; @pass jobline=run=r001|CORPUS:src/filler.bb2|RAM:con
; @pass jobline=run=r002|CORPUS:src/compiler-data-pad-byte.bb2|RAM:con
p.l=0
q.l=0
i.l=0
f.w=0
s$="JKL"
Restore
PutReg d0,p
Restore a
Read s$
PutReg a3,q
NPrint "data2 pad ",Hex$(Peek.b(q-1)&255)," off ",Hex$(q-1-p)
i=8192
While f=0 AND i>7
  If Peek.w(p-i)=$4A4B AND Peek.b(p-i+2)=$4C Then f=1 Else i=i-1
Wend
NPrint "data1 pad ",Hex$(Peek.b(p-i+3)&255)," found ",f
End
a: Data$ "JKL"
