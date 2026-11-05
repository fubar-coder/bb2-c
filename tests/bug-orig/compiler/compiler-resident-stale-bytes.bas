; compiler-resident-stale-bytes (#970) - Create Resident writes bytes the
; compiler never set. Every node has an odd length here (type abc 15, field
; x 17, type def 15, field y 17, macro m 15, constant a 21; the text of m 3
; bytes): writeitem and its kin round the length up and write one byte
; behind the allocation (inject guard: a pattern byte, never 0). The
; constant's data (+12) and procnum (+16) keep what the allocation held
; (inject fill=a5: $a5). A field's type keeps the low word of the type
; pointer (writeoffs puts the type index into the high word only) and a
; type's next is the raw pointer (restypes). The program reads the
; resident and ORs these bytes per kind; it prints 1 for a kind whose
; bytes are not 0 (their values depend on the memory).
; Correct: all of them 0 - the same source always gives the same file.
; @pass wrap=@INJECT@|guard fill=a5
; @pass file=r.bb2|@TESTDATA@/compiler-resident-stale-bytes.res
; @pass jobline=resident=r|CORPUS:r.bb2|CORPUS:out/r.res
DEFTYPE .l
Dim m.l(256)
If ReadFile(0,"CORPUS:out/r.res")
  ReadMem 0,&m(0),Lof(0)
  CloseFile 0
EndIf
p=&m(0)+32
; types: the fields of each (the last one with next 0), then the type node
nt=Peek.w(p) : p=p+2
For i=1 To nt
  Repeat
    n=Peek.l(p) : l=Peek.b(p+14)&255
    lw=lw OR (Peek.w(p+12)&$FFFF)
    If l&1 Then pf=pf OR (Peek.b(p+l)&255)
    p=p+((l+1)&$FFFE)
  Until n=0
  tn=tn OR Peek.l(p) : l=Peek.b(p+10)&255
  If l&1 Then pt=pt OR (Peek.b(p+l)&255)
  p=p+((l+1)&$FFFE)
Next
; macros: the node, then its text
nm=Peek.w(p) : p=p+2
For i=1 To nm
  tl=Peek.w(p+8)&$FFFF : l=Peek.b(p+12)&255
  If l&1 Then pm=pm OR (Peek.b(p+l)&255)
  p=p+((l+1)&$FFFE)
  If tl&1 Then px=px OR (Peek.b(p+tl)&255)
  p=p+((tl+1)&$FFFE)
Next
; constants
nc=Peek.w(p) : p=p+2
For i=1 To nc
  cd=cd OR Peek.l(p+12) : cp=cp OR (Peek.w(p+16)&$FFFF)
  l=Peek.b(p+18)&255
  If l&1 Then pc=pc OR (Peek.b(p+l)&255)
  p=p+((l+1)&$FFFE)
Next
If pf Then pf=1
If pt Then pt=1
If pm Then pm=1
If px Then px=1
If pc Then pc=1
If cd Then cd=1
If cp Then cp=1
If lw Then lw=1
If tn Then tn=1
NPrint "types ",nt," macros ",nm," constants ",nc," text ",tl
NPrint "pad: field ",pf," type ",pt," macro ",pm," text ",px," constant ",pc
NPrint "constant: data ",cd," procnum ",cp
NPrint "field type low word ",lw," type next ",tn
End
