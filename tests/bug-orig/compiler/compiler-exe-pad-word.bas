; compiler-exe-pad-word (#1184) - Create Executable writes the code hunk
; rounded up to a long straight from the object buffer, which is rounded to
; words only: for a program with an odd number of code words the last word
; of the hunk (the pad) is read from the 2 bytes behind the buffer. inject
; guard puts its pattern there (never 0). Jobs j001 to j004 compile p0
; (NPrint, End) and p1 (the same with one NOP more) twice each; one of the
; two has an odd number of code words (which one depends on the
; libraries). p1's hunk is one long longer than p0's if p1 is the odd one,
; else they have the same size. The program finds the odd one, ORs its pad
; words (both compiles) and compares the two compiles of each source.
; Correct: the pad word is 0, the same source gives the same executable.
; @fixed #1184
; @pass wrap=@INJECT@|guard
; @pass file=p0.bb2|@TESTDATA@/compiler-exe-pad-word.p0
; @pass file=p1.bb2|@TESTDATA@/compiler-exe-pad-word.p1
; @pass jobline=job=j001|CORPUS:p0.bb2|CORPUS:out/p0a
; @pass jobline=job=j002|CORPUS:p1.bb2|CORPUS:out/p1a
; @pass jobline=job=j003|CORPUS:p0.bb2|CORPUS:out/p0b
; @pass jobline=job=j004|CORPUS:p1.bb2|CORPUS:out/p1b
DEFTYPE .l
Dim a.l(3000),b.l(3000),c.l(3000),d.l(3000)
f$="CORPUS:out/p0a" : p=&a(0) : Gosub rd : la=l
f$="CORPUS:out/p1a" : p=&b(0) : Gosub rd : lb=l
f$="CORPUS:out/p0b" : p=&c(0) : Gosub rd : lc=l
f$="CORPUS:out/p1b" : p=&d(0) : Gosub rd : ld=l
; the size of the code hunk in longs (the header: 5 longs, then it)
sa=Peek.l(&a(0)+20) : sb=Peek.l(&b(0)+20)
If sb>sa
  po=&b(0)+32+sb*4-2 : pq=&d(0)+32+sb*4-2
Else
  po=&a(0)+32+sa*4-2 : pq=&c(0)+32+sa*4-2
EndIf
pw=(Peek.w(po)&$FFFF) OR (Peek.w(pq)&$FFFF)
If pw Then pw=1
s0=1 : s1=1
If la<>lc Then s0=0
If lb<>ld Then s1=0
For i=0 To 3000
  If a(i)<>c(i) Then s0=0
  If b(i)<>d(i) Then s1=0
Next
sz=0
If sa>0 AND (sb=sa OR sb=sa+1) Then sz=1
NPrint "read ",n," sizes ",sz
NPrint "pad ",pw
NPrint "same p0 ",s0," p1 ",s1
End
rd:
l=0
If ReadFile(0,f$)
  l=Lof(0)
  If l>12000 Then l=12000
  ReadMem 0,p,l
  CloseFile 0
  n=n+1
EndIf
Return
