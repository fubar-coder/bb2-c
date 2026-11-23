; compiler-exe-pad-word (#1184) - Create Executable writes the code hunk
; rounded up to a long straight from the object buffer, which is rounded to
; words only: for a program with an odd number of code words the last word
; of the hunk (the pad) is read from the 2 bytes behind the buffer. inject
; guard puts its pattern there (never 0). Jobs j001 to j004 compile p0
; (NPrint, End) and p1 (the same with one NOP more) twice each; one of the
; two has an odd number of code words (which one depends on the
; libraries). p1's hunk is one long longer than p0's if p1 is the odd one,
; else they have the same size. The case finds the odd one, ORs its pad
; words (both compiles) and compares the two compiles of each source.
; Correct: the pad word is 0, the same source gives the same executable.
DEFTYPE .l

; rdfile{f$,p}: reads the file f$ (at most 12000 bytes) to p, returns its
; length (0: not read)
Function.l rdfile{f$,p}
  l=0
  If ReadFile(0,f$)
    l=Lof(0)
    If l>12000 Then l=12000
    ReadMem 0,p,l
    CloseFile 0
  EndIf
  Function Return l
End Function

!TestCase{pad_word}
  Dim a.l(3000),b.l(3000),c.l(3000),d.l(3000)
  la=rdfile{"CORPUS:out/p0a",&a(0)}
  lb=rdfile{"CORPUS:out/p1a",&b(0)}
  lc=rdfile{"CORPUS:out/p0b",&c(0)}
  ld=rdfile{"CORPUS:out/p1b",&d(0)}
  n=0
  If la>0 Then n+1
  If lb>0 Then n+1
  If lc>0 Then n+1
  If ld>0 Then n+1
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
  !CheckEq{n,4,"files_read"}
  !CheckEq{sz,1,"sizes"}
  !CheckEq{pw,0,"pad_word"}
  !CheckEq{s0,1,"same_p0"}
  !CheckEq{s1,1,"same_p1"}
!EndTestCase
