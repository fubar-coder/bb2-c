; compiler-fold-mod-relocs (#1049) - a constant expression is folded:
; its code is run at compile time and replaced by its value, and the
; relocations added since it began are freed.  A constant Mod as the
; first use of modlib links modlib in during the fold (uselib3), and the
; cleanup frees modlib's relocations too: modlib's calls of ldivlib and
; lmullib keep their compile-time addresses in the executable (JSR to the
; compiler's memory).  A Long Mod at run time then jumps there.
; Here: 7 Mod 3 (folded, the first Mod), then a Long Mod of variables;
; the program walks its own segment list (pr_CLI, cli_Module) and counts
; the JSR abs.l instructions ($4EB9) of its hunks whose target lies in
; none of its hunks.  It prints the counts, not the results of the Long
; Mod (which work as long as the compiler's memory is untouched).
; ($4EB9 is computed at run time, so that the comparison holds no
; constant that reads as such a JSR.)
; Correct: every JSR abs.l of the program points into its own hunks.
; @fixed #1049
; @configs clib
DEFTYPE .l
Dim lo.l(15),hi.l(15)
a.w=7 Mod 3
x=17 : y=5
b=x Mod y
t=Peek.l(Peek.l(4)+276)
cl=Peek.l(t+172)*4
s=Peek.l(cl+60)*4
n=0
While s<>0 AND n<16
  lo(n)=s-4 : hi(n)=s-4+Peek.l(s-4)
  n=n+1
  s=Peek.l(s)*4
Wend
NPrint "hunks ",n
bad=0 : jw=$4EB8 : jw=jw+1
For h=0 To n-1
  For p=lo(h)+8 To hi(h)-6 Step 2
    If (Peek.w(p) AND $FFFF)=jw
      d=Peek.l(p+2) : ok=0
      For k=0 To n-1
        If d>=lo(k) AND d<hi(k) Then ok=1
      Next
      If ok=0 Then bad=bad+1
    EndIf
  Next
Next
NPrint "jsr outside the hunks ",bad
End
